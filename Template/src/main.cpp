#include <zephyr/kernel.h>
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
Initialization init_state;
Operational operational_state;
StopState stop_state;
Context *motor_state;



Encoder motor(C1, C2, ENC_OUT);
Drive bridge(SLP);

Digital_out led(LED);


double Kp_init = 7.39; // 8.21 fyrir P controller, 7.39 fyrir PI
int16_t target_speed = 5000;  // Target speed = rpm*100
P_controller Pctrl(Kp_init);
controller* ctrl = &Pctrl;

void main(void) {
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
      } else if (c[0] == 's' && c[1] == '\0') {
        motor_state->on_fault();
      }
      serial_flush();
    }

    if (check_fault())
      {
        motor_state->on_fault();
      }
    motor_state->do_work();
  }
}
