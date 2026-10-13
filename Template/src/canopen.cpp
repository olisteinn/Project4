#include "canopen.h"
#include "Context.h"
#include "uart.h"

extern Context *motor_state;
static const uint8_t NODE_ID = 1;

static void handle_nmt(const CanMsg &msg){
    if (msg.dlc >= 2) {
            uint8_t command = msg.data[0];
            uint8_t node    = msg.data[1];
    
            if (node != NODE_ID || node != 0) return; // for motor controller (node = 1) or for everyone (node = 0)
            
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

static void handle_SDO(const CanMsg &msg){

}

static void handle_RPDO1(const CanMsg &msg){

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