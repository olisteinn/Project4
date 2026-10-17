#include "canopen.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "Context.h"
#include "Operational.h"
#include "PreOperational.h"
#include "StopState.h"
#include "app_interface.h"
#include "can_bus.h"
#include "state_events.h"

LOG_MODULE_REGISTER(canopen, LOG_LEVEL_INF);

extern Context *motor_state;

static constexpr uint32_t kDeviceType = 0x00020192;
static constexpr uint32_t kAbortUnsupportedAccess = 0x06010002;
static constexpr uint32_t kAbortObjectMissing = 0x06020000;
static constexpr uint32_t kAbortValueRange = 0x06090030;
static constexpr uint32_t kAbortSubindexMissing = 0x06090011;
static constexpr uint32_t kAbortWriteInWrongState = 0x08000022;
static constexpr uint32_t kAbortInvalidCommand = 0x05040001;

enum class NmtRequest : uint8_t {
  Operate,
  Stop,
  PreOperational,
  Reset
};

enum class OdAccess : uint8_t {
  ReadOnly,
  ReadWrite,
  ReadWritePreOperational
};

struct OdEntry {
  uint16_t index;
  uint8_t size;
  OdAccess access;
};

static constexpr OdEntry kObjectDictionary[] = {
    {0x1000, 4, OdAccess::ReadOnly},
    {0x2000, 1, OdAccess::ReadWritePreOperational},
    {0x2001, 4, OdAccess::ReadWritePreOperational},
    {0x2002, 4, OdAccess::ReadWritePreOperational},
    {0x2010, 2, OdAccess::ReadOnly},
    {0x606C, 4, OdAccess::ReadOnly},
    {0x6083, 4, OdAccess::ReadWritePreOperational},
    {0x6084, 4, OdAccess::ReadWritePreOperational},
    {0x60FF, 4, OdAccess::ReadWrite},
};

K_MSGQ_DEFINE(nmt_request_queue, sizeof(NmtRequest), 10, alignof(NmtRequest));

static const OdEntry *find_od_entry(uint16_t index)
{
  for (const OdEntry &entry : kObjectDictionary) {
    if (entry.index == index) {
      return &entry;
    }
  }
  return nullptr;
}

static uint32_t get_u32_le(const uint8_t *data)
{
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

static int32_t get_i32_le(const uint8_t *data)
{
  return static_cast<int32_t>(get_u32_le(data));
}

static void put_u32_le(uint8_t *data, uint32_t value)
{
  data[0] = static_cast<uint8_t>(value);
  data[1] = static_cast<uint8_t>(value >> 8);
  data[2] = static_cast<uint8_t>(value >> 16);
  data[3] = static_cast<uint8_t>(value >> 24);
}

static void put_i16_le(uint8_t *data, int16_t value)
{
  const uint16_t bits = static_cast<uint16_t>(value);
  data[0] = static_cast<uint8_t>(bits);
  data[1] = static_cast<uint8_t>(bits >> 8);
}

static uint32_t float_to_u32(float value)
{
  uint32_t bits;
  static_assert(sizeof(bits) == sizeof(value), "OD float must be 32 bits");
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

static float u32_to_float(uint32_t bits)
{
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static uint32_t read_od_value(uint16_t index)
{
  switch (index) {
    case 0x1000: return kDeviceType;
    case 0x2000: return app_get_controller_type();
    case 0x2001: return float_to_u32(app_get_kp());
    case 0x2002: return float_to_u32(app_get_ti());
    case 0x2010: return static_cast<uint16_t>(app_get_duty());
    case 0x606C: return static_cast<uint32_t>(app_get_actual_speed());
    case 0x6083: return app_get_accel_limit();
    case 0x6084: return app_get_dec_limit();
    case 0x60FF: return static_cast<uint32_t>(app_get_target_speed());
    default: return 0;
  }
}

static bool write_od_value(uint16_t index, uint32_t raw)
{
  switch (index) {
    case 0x2000:
      return app_set_controller_type(static_cast<uint8_t>(raw));
    case 0x2001:
      return app_set_kp(u32_to_float(raw));
    case 0x2002:
      return app_set_ti(u32_to_float(raw));
    case 0x6083:
      return app_set_accel_limit(raw);
    case 0x6084:
      return app_set_dec_limit(raw);
    case 0x60FF:
      app_set_target_speed(static_cast<int32_t>(raw));
      return true;
    default:
      return false;
  }
}

static bool queue_nmt_request(NmtRequest request)
{
  if (k_msgq_put(&nmt_request_queue, &request, K_NO_WAIT) != 0) {
    LOG_WRN("NMT request queue full");
    return false;
  }
  state_event_notify();
  return true;
}

static void handle_nmt(const CanMsg &msg)
{
  if (msg.dlc < 2) {
    LOG_WRN("Ignoring short NMT frame");
    return;
  }

  const uint8_t command = msg.data[0];
  const uint8_t node = msg.data[1];
  if (node != 0 && node != 1) {
    return;
  }

  switch (command) {
    case 0x01: queue_nmt_request(NmtRequest::Operate); break;
    case 0x02: queue_nmt_request(NmtRequest::Stop); break;
    case 0x80: queue_nmt_request(NmtRequest::PreOperational); break;
    case 0x81:
    case 0x82: queue_nmt_request(NmtRequest::Reset); break;
    default: LOG_WRN("Unknown NMT command 0x%02x", command); break;
  }
}

static void send_sdo_response(uint8_t command, uint16_t index, uint8_t subindex,
                              uint32_t value)
{
  CanMsg response = {};
  response.id = 0x581;
  response.dlc = 8;
  response.data[0] = command;
  response.data[1] = static_cast<uint8_t>(index);
  response.data[2] = static_cast<uint8_t>(index >> 8);
  response.data[3] = subindex;
  put_u32_le(&response.data[4], value);
  if (!can_bus_send(response)) {
    LOG_ERR("Failed to send SDO response for 0x%04x", index);
  }
}

static void send_sdo_abort(uint16_t index, uint8_t subindex, uint32_t code)
{
  send_sdo_response(0x80, index, subindex, code);
}

static void handle_sdo(const CanMsg &msg)
{
  if (msg.dlc != 8) {
    LOG_WRN("Ignoring SDO frame with DLC %u", msg.dlc);
    return;
  }

  const uint8_t command = msg.data[0];
  const uint16_t index = static_cast<uint16_t>(msg.data[1]) |
                         (static_cast<uint16_t>(msg.data[2]) << 8);
  const uint8_t subindex = msg.data[3];
  const OdEntry *entry = find_od_entry(index);
  if (entry == nullptr) {
    send_sdo_abort(index, subindex, kAbortObjectMissing);
    return;
  }
  if (subindex != 0) {
    send_sdo_abort(index, subindex, kAbortSubindexMissing);
    return;
  }

  if (command == 0x40) {
    const uint8_t response_command =
        static_cast<uint8_t>(0x43 | ((4 - entry->size) << 2));
    send_sdo_response(response_command, index, subindex,
                      read_od_value(index));
    return;
  }

  const uint8_t expected_download_command =
      entry->size == 1 ? 0x2F : (entry->size == 2 ? 0x2B : 0x23);
  if (command != expected_download_command) {
    send_sdo_abort(index, subindex, kAbortInvalidCommand);
    return;
  }
  if (entry->access == OdAccess::ReadOnly) {
    send_sdo_abort(index, subindex, kAbortUnsupportedAccess);
    return;
  }
  if (entry->access == OdAccess::ReadWritePreOperational &&
      app_get_nmt_state() != AppNmtState::PreOperational) {
    send_sdo_abort(index, subindex, kAbortWriteInWrongState);
    return;
  }

  uint32_t raw = get_u32_le(&msg.data[4]);
  if (entry->size == 1) {
    raw &= 0xFF;
  } else if (entry->size == 2) {
    raw &= 0xFFFF;
  }
  if (!write_od_value(index, raw)) {
    send_sdo_abort(index, subindex, kAbortValueRange);
    return;
  }

  send_sdo_response(0x60, index, subindex, 0);
}

static void handle_rpdo1(const CanMsg &msg)
{
  if (msg.dlc != 4) {
    LOG_WRN("Ignoring RPDO1 frame with DLC %u", msg.dlc);
    return;
  }
  if (app_get_nmt_state() != AppNmtState::Operational) {
    return;
  }
  app_set_target_speed(get_i32_le(msg.data));
}

void canopen_handle(const CanMsg &msg)
{
  if (msg.id == 0x000) {
    handle_nmt(msg);
  } else if (msg.id == 0x601 &&
             app_get_nmt_state() != AppNmtState::Stopped) {
    handle_sdo(msg);
  } else if (msg.id == 0x201) {
    handle_rpdo1(msg);
  }
}

void canopen_process_pending_nmt()
{
  NmtRequest request;
  while (k_msgq_get(&nmt_request_queue, &request, K_NO_WAIT) == 0) {
    switch (request) {
      case NmtRequest::Operate: motor_state->on_operate(); break;
      case NmtRequest::Stop: motor_state->on_fault(); break;
      case NmtRequest::PreOperational: motor_state->on_pre_operate(); break;
      case NmtRequest::Reset: motor_state->reset(); break;
    }
  }
}

void canopen_send_bootup()
{
  CanMsg message = {};
  message.id = 0x701;
  message.dlc = 1;
  message.data[0] = 0x00;
  if (!can_bus_send(message)) {
    LOG_ERR("Failed to send CANopen boot-up message");
  }
}

void canopen_send_tpdo1()
{
  CanMsg message = {};
  message.id = 0x181;
  message.dlc = 6;
  put_u32_le(&message.data[0],
             static_cast<uint32_t>(app_get_actual_speed()));
  put_i16_le(&message.data[4], app_get_duty());
  can_bus_send_async(message);
}
