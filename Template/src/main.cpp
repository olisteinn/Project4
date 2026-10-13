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
  
  Context ctx(&init_state);
  motor_state = &ctx;
  char c[10];
  
  while (true) {
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
