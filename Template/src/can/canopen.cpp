#include "canopen.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <string.h>

#include "Context.h"
#include "Operational.h"
#include "PreOperational.h"
#include "StopState.h"
#include "app_interface.h"
#include "can_bus.h"
#include "state_events.h"

extern Context *motor_state;

static atomic_t pending_nmt = ATOMIC_INIT(0);

static int32_t get_i32_le(const uint8_t *d)
{
  uint32_t v = (uint32_t)d[0]
             | ((uint32_t)d[1] << 8)
             | ((uint32_t)d[2] << 16)
             | ((uint32_t)d[3] << 24);
  return (int32_t)v;
}

static void put_u32_le(uint8_t *d, uint32_t v)
{
  d[0] = v & 0xFF;
  d[1] = (v >> 8) & 0xFF;
  d[2] = (v >> 16) & 0xFF;
  d[3] = (v >> 24) & 0xFF;
}

static void put_i16_le(uint8_t *d, int16_t value)
{
  uint16_t v = (uint16_t)value;
  d[0] = v & 0xFF;
  d[1] = (v >> 8) & 0xFF;
}

static void handle_nmt(const CanMsg &msg)
{
  if (msg.dlc < 2) {
    return;
  }

  uint8_t command = msg.data[0];
  uint8_t node    = msg.data[1];
  if (node != 0 && node != 1) {
    return;
  }

  atomic_set(&pending_nmt, command);
  state_event_notify();
}

static void send_sdo_response(uint8_t command, uint16_t index, uint8_t subindex,
                              uint32_t value)
{
  CanMsg response = {};
  response.id = 0x581;
  response.dlc = 8;
  response.data[0] = command;
  response.data[1] = index & 0xFF;
  response.data[2] = (index >> 8) & 0xFF;
  response.data[3] = subindex;
  put_u32_le(&response.data[4], value);
  can_bus_send(response);
}

static void handle_sdo(const CanMsg &msg)
{
  if (msg.dlc < 8) {
    return;
  }

  uint8_t cmd       = msg.data[0];
  uint16_t index    = msg.data[1] | ((uint16_t)msg.data[2] << 8);
  uint8_t subindex  = msg.data[3];

  // SDO upload (read)
  if (cmd == 0x40) {
    uint32_t val = 0;
    uint8_t reply_cmd = 0x43; // 4 bytes
    switch (index) {
      case 0x1000: val = 0x00020192; break;
      case 0x2000: val = app_get_controller_type(); reply_cmd = 0x4F; break;
      case 0x2001: { float f = app_get_kp(); memcpy(&val, &f, 4); break; }
      case 0x2002: { float f = app_get_ti(); memcpy(&val, &f, 4); break; }
      case 0x2010: val = (uint16_t)app_get_duty(); reply_cmd = 0x4B; break;
      case 0x606C: val = (uint32_t)app_get_actual_speed(); break;
      case 0x6083: val = app_get_accel_limit(); break;
      case 0x6084: val = app_get_dec_limit(); break;
      case 0x60FF: val = (uint32_t)app_get_target_speed(); break;
      default:
        send_sdo_response(0x80, index, subindex, 0x06020000);
        return;
    }
    send_sdo_response(reply_cmd, index, subindex, val);
    return;
  }

  // SDO download (write)
  if (cmd == 0x23 || cmd == 0x2B || cmd == 0x2F) {
    // Configuration objects can only be written in Pre-Operational
    if (index == 0x2000 || index == 0x2001 || index == 0x2002 ||
        index == 0x6083 || index == 0x6084) {
      if (motor_state->get_state() != &pre_op_state) {
        send_sdo_response(0x80, index, subindex, 0x08000022);
        return;
      }
    }

    uint32_t raw = (uint32_t)get_i32_le(&msg.data[4]);
    switch (index) {
      case 0x2000: app_set_controller_type((uint8_t)raw); break;
      case 0x2001: { float f; memcpy(&f, &raw, 4); app_set_kp(f); break; }
      case 0x2002: { float f; memcpy(&f, &raw, 4); app_set_ti(f); break; }
      case 0x6083: app_set_accel_limit(raw); break;
      case 0x6084: app_set_dec_limit(raw); break;
      case 0x60FF: app_set_target_speed((int32_t)raw); break;
      default:
        send_sdo_response(0x80, index, subindex, 0x06020000);
        return;
    }
    send_sdo_response(0x60, index, subindex, 0);
  }
}

static void handle_rpdo1(const CanMsg &msg)
{
  if (motor_state->get_state() != &operational_state || msg.dlc < 4) {
    return;
  }
  app_set_target_speed(get_i32_le(msg.data));
}

void canopen_handle(const CanMsg &msg)
{
  if (msg.id == 0x000) {
    handle_nmt(msg);
  } else if (msg.id == 0x601 && motor_state->get_state() != &stop_state) {
    handle_sdo(msg);
  } else if (msg.id == 0x201) {
    handle_rpdo1(msg);
  } else if (msg.id == 0x080 && motor_state->get_state() == &operational_state) {
    canopen_send_tpdo1();
  }
}

void canopen_process_pending_nmt()
{
  uint8_t command = (uint8_t)atomic_set(&pending_nmt, 0);
  switch (command) {
    case 0x01: motor_state->on_operate();     break;
    case 0x02: motor_state->on_fault();       break;
    case 0x80: motor_state->on_pre_operate(); break;
    case 0x81:
    case 0x82: motor_state->reset();          break;
    default: break;
  }
}

void canopen_send_bootup() {
  CanMsg msg = {};
  msg.id = 0x701;
  msg.dlc = 1;
  msg.data[0] = 0x00;
  can_bus_send(msg);
}

void canopen_send_tpdo1() {
  CanMsg msg = {};
  msg.id  = 0x181;
  msg.dlc = 6;
  put_u32_le(&msg.data[0], (uint32_t)app_get_actual_speed());
  put_i16_le(&msg.data[4], app_get_duty());
  can_bus_send(msg);
}

void canopen_send_emcy() {
  CanMsg msg = {};
  msg.id  = 0x081;
  msg.dlc = 8;
  msg.data[0] = 0x00;
  msg.data[1] = 0x10;
  msg.data[2] = 0x01;
  can_bus_send(msg);
}