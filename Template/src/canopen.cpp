#include "canopen.h"
#include "Context.h"
#include "app_interface.h"
#include "Operational.h"
#include "PreOperational.h"
#include "StopState.h" 

extern Context *motor_state;

static const uint32_t DEVICE_TYPE = 0x00020192;

static const uint32_t SDO_ABORT_WRITE_PREOP = 0x08000022;
static const uint32_t SDO_ABORT_OBJECT_NO_EXIST = 0x06020000;

enum Access { RO, RW, RW_PREOP }; //ReadOnly, ReadWrite, ReadWrite in PreOperational only

//Object dictionary
struct OdEntry {
  uint16_t index;   // which object (e.g. 0x2001 for Kp)
  uint8_t  size;    // how many bytes (1 for int8,2 for int16 or 4 for int32/float)
  Access   access;  // RO,RW,RW_PREOP
};

static const OdEntry od[] = {
  {0x1000, 4, RO},        // Device type
  {0x2000, 1, RW_PREOP},  // Controller type (0 = P, 1 = PI)
  {0x2001, 4, RW_PREOP},  // Kp (REAL32)
  {0x2002, 4, RW_PREOP},  // Ti (REAL32)
  {0x2010, 2, RO},        // Output duty cycle (0.1 %)
  {0x606C, 4, RO},        // Velocity actual value (RPM)
  {0x6083, 4, RW_PREOP},  // Profile acceleration
  {0x6084, 4, RW_PREOP},  // Profile deceleration
  {0x60FF, 4, RW},        // Target velocity (RPM)
};

// * => returns a pointer
static const OdEntry *od_find(uint16_t index)
{
  for (const OdEntry &e : od) { // : means in
    if (e.index == index) return &e;
  }
  return nullptr;  // object does not exist
}

static uint32_t od_read(uint16_t index)
{
  switch (index) {
    case 0x1000: return DEVICE_TYPE;
    case 0x2000: return app_get_controller_type();
    case 0x2001: return (uint32_t)(app_get_kp() * 1024.0f + 0.5f);  // +0.5 rounds
    case 0x2002: return (uint32_t)(app_get_ti() * 1024.0f + 0.5f);
    case 0x2010: return (uint16_t)app_get_duty();
    case 0x606C: return (uint32_t)app_get_actual_speed();
    case 0x6083: return app_get_accel_limit();
    case 0x6084: return app_get_dec_limit();
    case 0x60FF: return (uint32_t)app_get_target_speed();
  }
  return 0;
}

static bool od_write(uint16_t index, uint32_t raw)
{
  switch (index) {
    case 0x2000: return app_set_controller_type((uint8_t)raw);
    case 0x2001: return app_set_kp(raw / 1024.0f);
    case 0x2002: return app_set_ti(raw / 1024.0f);
    case 0x6083: return app_set_accel_limit(raw);
    case 0x6084: return app_set_dec_limit(raw);
    case 0x60FF: app_set_target_speed((int32_t)raw); return true;
  }
  return false;
}

// helper functions 
static void handle_nmt(const CanMsg &msg){
    if (msg.dlc >= 2) {
            uint8_t command = msg.data[0];
            uint8_t node    = msg.data[1];
    
            if (node != 1 && node != 0) return; // for motor controller (node = 1) or for everyone (node = 0)
            
            switch (command) {
                case 0x01: motor_state->on_operate();     break;
                case 0x02: motor_state->on_fault();       break;
                case 0x80: motor_state->on_pre_operate(); break;
                case 0x81: motor_state->reset();          break;
                case 0x82: motor_state->reset();          break;
                }
            }
}

// 
static int32_t get_i32_le(const uint8_t *d)
{
  // d is a pointer to the data sequence
  uint32_t v = (uint32_t)d[0] // first data point f.e. 0x34 becomes 0x00000034
  | (uint32_t)d[1] << 8 // second data point f.e. 0x12 becomes 0x00000012 then shifted left 8 bits -> 0x00001200
  | (uint32_t)d[2] << 16 
  | (uint32_t)d[3] << 24;
  return (int32_t)v; //makes it signed
}

static void put_i32_le(uint8_t *d, int32_t value)
{
  uint32_t v = (uint32_t)value;
  d[0] = v & 0xFF;          // lowest byte first
  d[1] = (v >> 8)  & 0xFF;
  d[2] = (v >> 16) & 0xFF;
  d[3] = (v >> 24) & 0xFF;
}

static void put_i16_le(uint8_t *d, int16_t value)
{
  uint16_t v = (uint16_t)value;
  d[0] = v & 0xFF;
  d[1] = (v >> 8) & 0xFF;
}

static void sdo_send(uint8_t cmd, uint16_t index, uint8_t sub, uint32_t data)
{
  CanMsg r = {};
  r.id  = 0x581;
  r.dlc = 8;                          // SDO is always 8 bytes
  r.data[0] = cmd;
  r.data[1] = index & 0xFF;           // index, little-endian
  r.data[2] = (index >> 8) & 0xFF;
  r.data[3] = sub;
  put_i32_le(&r.data[4], (int32_t)data);
  can_bus_send(r);
}

static void sdo_abort(uint16_t index, uint8_t sub, uint32_t code)
{
  sdo_send(0x80, index, sub, code);
}




static void handle_SDO(const CanMsg &msg){
  if (msg.dlc != 8) return;

  uint8_t  cmd   = msg.data[0];
  uint16_t index = msg.data[1] | (msg.data[2] << 8);
  uint8_t  sub   = msg.data[3];
  uint8_t  ccs   = cmd >> 5;  // 1 = download (write), 2 = upload (read)

  if (ccs != 1 && ccs != 2) return;  // unknown command: ignore

  const OdEntry *e = od_find(index);
  if (e == nullptr) {
    sdo_abort(index, sub, SDO_ABORT_OBJECT_NO_EXIST);
    return;
  }

  // ---- Upload (read) ----
  if (ccs == 2) {
    uint8_t resp = 0x43 | ((4 - e->size) << 2);  // 4 bytes -> 43, 2 -> 4B, 1 -> 4F
    sdo_send(resp, index, sub, od_read(index));
    return;
  }

  // ---- Download (write) ----
  if (e->access == RW_PREOP && motor_state->get_state() != &pre_op_state) {
    sdo_abort(index, sub, SDO_ABORT_WRITE_PREOP);
    return;
  }

  uint32_t raw = (uint32_t)get_i32_le(&msg.data[4]); // reads byte 4,5,6,7 and puts in le 
  if (e->size == 1) raw &= 0xFF;     // ignore unused bytes
  if (e->size == 2) raw &= 0xFFFF;

  if (!od_write(index, raw)) return;  // read-only object or invalid value: ignored, no reply

  sdo_send(0x60, index, sub, 0);      // write accepted

}

static void handle_RPDO1(const CanMsg &msg){
  if (motor_state->get_state() != &operational_state || msg.dlc <4)
    return; // should only be accepted in operational state and needs atleast 4 bytes for an Integer32
  
  int32_t target_rpm = get_i32_le(msg.data);
  app_set_target_speed(target_rpm);
}


// Called functions
void canopen_handle(const CanMsg &msg)
{
  if (msg.id == 0x000) {
    handle_nmt(msg);
  }
  else if (msg.id == 0x601 && motor_state->get_state() != &stop_state){
    handle_SDO(msg);
  }
  else if (msg.id == 0x201){
    handle_RPDO1(msg);
  }
}

void canopen_send_bootup(){
  CanMsg msg = {};
  msg.id = 0x701;
  msg.dlc = 1;
  msg.data[0] = 0x00; 
  can_bus_send(msg);
}


void canopen_send_tpdo1(){
  CanMsg msg = {};
  msg.id  = 0x181;
  msg.dlc = 6;
  put_i32_le(&msg.data[0], app_get_actual_speed());  // bytes 0-3 (int32)
  put_i16_le(&msg.data[4], app_get_duty());          // bytes 4-5 (int16)
  can_bus_send(msg);
}