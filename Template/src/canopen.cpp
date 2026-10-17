#include "canopen.h"
#include "Context.h"
#include "uart.h"
#include "Operational.h"
#include "app_interface.h"

extern Context *motor_state;

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
                default: serial_println("Unknown NMT command");
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


static void handle_SDO(const CanMsg &msg){

}

static void handle_RPDO1(const CanMsg &msg){
  if (motor_state->get_state() != &operational_state || msg.dlc <4)
    return; // should only be accepted in operational state and needs atleast 4 bytes for an Integer32
  
  int32_t target_rpm = get_i32_le(msg.data);
  app_set_target_speed(target_rpm);
}



void canopen_handle(const CanMsg &msg)
{
  if (msg.id == 0x000) {
    handle_nmt(msg);
  }
  else if (msg.id == 0x601){
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