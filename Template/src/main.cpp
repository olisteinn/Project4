#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <stdint.h>
#include <stdlib.h>
#include "digital_in.h"
#include "digital_out.h"
#include "encoder.h"
#include "drive.h"
#include "P_controller.h"
#include "Context.h"
#include "uart.h"
#include "Initialization.h"
#include "Operational.h"
#include "State.h"
#include "Fault_handling.h"
#include "StopState.h"
#include "PI_controller.h"
#include "PreOperational.h"

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
CAN_MSGQ_DEFINE(rx_msgq, 10);  // queue for can that can hold 10 messages 


Initialization init_state;
Operational operational_state;
PreOperational pre_op_state;
StopState stop_state;
Context *motor_state;



Encoder motor(C1, C2, ENC_OUT);
Drive bridge(SLP);

Digital_out led(LED);


double Kp_init_P  = 8.21;     // optimal for P controller
double Kp_init_PI = 7.39;     // optimal for PI controller
double Ti_init_PI = 0.08517;  // optimal for PI controller
double Kp_inp_P   = Kp_init_P;
double Kp_inp_PI  = Kp_init_PI;
double Ti_inp     = Ti_init_PI;
int16_t target_speed = 5000;  // Target speed = rpm*100
P_controller  Pctrl(Kp_init_P);
PI_controller PIctrl(Kp_init_PI, Ti_init_PI);
controller* ctrl = &Pctrl;

int main(void) {
  serial_init();
  
  const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);
  if (!device_is_ready(dev))
  {
    printk("CAN device not ready\n");
    return 0;
  }
  
  int err = can_set_bitrate(dev, 125000);
  
  if (err != 0)
  {
    printk("Error setting CAN bitrate (err %d)\n", err);
    return 0;
  }
  
  err = can_start(dev);
  
  if (err != 0)
  {
    printk("Error starting CAN controller (err %d)\n", err);
    return 0;
  }

  struct can_filter nmt_filter = {};
  nmt_filter.id    = 0x000; // NMT id 
  nmt_filter.mask  = CAN_STD_ID_MASK; // all bits must match to get through
  nmt_filter.flags = 0; // 0 means normal can messages with 11 bits id

  can_add_rx_filter_msgq(dev, &rx_msgq, &nmt_filter); // put every message that matches nmt_filter into the mailbox rx_msgq


  Context ctx(&init_state);
  motor_state = &ctx;
  char c[10];
  
  while (true) {

    struct can_frame rx; // an empty can message
    while (k_msgq_get(&rx_msgq, &rx, K_NO_WAIT) == 0) // takes the oldest message from the mailbox and copys it to the rx, returns 0 if it got a message, K_NO_WAIT => if mailbox is empty return immediately
     {
      printk("CAN rx id 0x%03X [%d]", rx.id, rx.dlc); // prints id and number of bytes
      // Print each byte as 2digit hex
      for (int i = 0; i < rx.dlc; i++) {
        printk(" %02X", rx.data[i]);
      }
      printk("\n");
    }



    if (serial_available()) {
      serial_read_string(c,10);
      serial_print("\r\n");
      serial_print("I received: ");
      serial_print(c);
      serial_print("\r\n");

      if (c[0] == 'r' && c[1] == '\0') {
        motor_state->reset();
      } else if (c[0] == 'o' && c[1] == '\0') {
        motor_state->on_operate();
        }
        else if (c[0] == 'p' && c[1] == '\0') {
        motor_state->on_pre_operate();
        } 
        else if (c[0] == 's' && c[1] == '\0') {
        motor_state->on_fault();
        }
        else if (motor_state->get_state() == &pre_op_state) {
        if (c[0] == 'P') {
          motor_state->controller_selector(c);
        } else {
          motor_state->set_parameters(c);
        }
      }
      serial_flush();
    }

    if (check_fault())
      {
        motor_state->on_fault();
      }
    motor_state->do_work();
  }

  return 0;
}
