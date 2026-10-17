#include "canopen.h"
#include "Context.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "state_events.h"

LOG_MODULE_REGISTER(canopen, LOG_LEVEL_INF);

extern Context *motor_state;

enum class NmtRequest : uint8_t {
    Operate,
    Stop,
    PreOperational,
    Reset
};

K_MSGQ_DEFINE(nmt_request_queue, sizeof(NmtRequest), 10, alignof(NmtRequest));

static void handle_nmt(const CanMsg &msg){
    if (msg.dlc >= 2) {
            uint8_t command = msg.data[0];
            uint8_t node    = msg.data[1];
    
            if (node != 1 && node != 0) return; // for motor controller (node = 1) or for everyone (node = 0)
            
            switch (command) {
                case 0x01: {
                    const NmtRequest request = NmtRequest::Operate;
                    if (k_msgq_put(&nmt_request_queue, &request, K_NO_WAIT) != 0) {
                        LOG_WRN("NMT request queue full");
                    } else {
                        state_event_notify();
                    }
                    break;
                }
                case 0x02: {
                    const NmtRequest request = NmtRequest::Stop;
                    if (k_msgq_put(&nmt_request_queue, &request, K_NO_WAIT) != 0) {
                        LOG_WRN("NMT request queue full");
                    } else {
                        state_event_notify();
                    }
                    break;
                }
                case 0x80: {
                    const NmtRequest request = NmtRequest::PreOperational;
                    if (k_msgq_put(&nmt_request_queue, &request, K_NO_WAIT) != 0) {
                        LOG_WRN("NMT request queue full");
                    } else {
                        state_event_notify();
                    }
                    break;
                }
                case 0x81:
                case 0x82: {
                    const NmtRequest request = NmtRequest::Reset;
                    if (k_msgq_put(&nmt_request_queue, &request, K_NO_WAIT) != 0) {
                        LOG_WRN("NMT request queue full");
                    } else {
                        state_event_notify();
                    }
                    break;
                }
                default: LOG_WRN("Unknown NMT command: 0x%02x", command);
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

void canopen_process_pending_nmt()
{
  NmtRequest request;
  while (k_msgq_get(&nmt_request_queue, &request, K_NO_WAIT) == 0) {
    switch (request) {
      case NmtRequest::Operate:       motor_state->on_operate();     break;
      case NmtRequest::Stop:          motor_state->on_fault();       break;
      case NmtRequest::PreOperational: motor_state->on_pre_operate(); break;
      case NmtRequest::Reset:         motor_state->reset();          break;
    }
  }
}

void canopen_send_bootup(){
  CanMsg msg = {};
  msg.id = 0x701;
  msg.dlc = 1;
  msg.data[0] = 0x00; 
  can_bus_send(msg);
}
