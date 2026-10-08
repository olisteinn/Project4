#include "uart.h"
#include <Initialization.h>
#include <Operational.h>
#include <Context.h>
#include <stdlib.h>
#include "encoder.h"
#include "drive.h"
#include "digital_out.h"
#include "timer.h"
#include <avr/interrupt.h>
#include "Fault_handling.h"
#include "PreOperational.h"
#include "StopState.h"
#include "controller.h"
#include "P_controller.h"
#include "PI_controller.h"


extern Drive bridge;
extern Digital_out led;
static uint32_t led_ms;

extern controller* ctrl;
extern P_controller Pctrl;
extern PI_controller PIctrl;

extern double Kp_inp_P;
extern double Kp_inp_PI;
extern double Ti_inp;

void PreOperational::on_do()
{
    if ((time_ms()-led_ms) >= 500) {
        led_ms = time_ms();
        led.toggle();
    }
}

void PreOperational::on_entry()
{
  serial_println("Pre Operational");
  bridge.stop();
  led_ms = time_ms();
  serial_println("Select controller type with ""P"" or ""PI""");
  serial_println("Set controller parameters with ""Kp"" ""Ti"" ");
}

void PreOperational::on_exit()
{
  led.set_lo();
  serial_print("Pre Operational -> ");
}

void PreOperational::reset()
{
  this->context_->transition_to(&init_state);
}

void PreOperational::on_operate()
{
  this->context_->transition_to(&operational_state);
}

void PreOperational::on_pre_operate()
{

}

void PreOperational::on_fault()
{
    this->context_->transition_to(&stop_state);

}

void PreOperational::controller_selector(char* inp)
{
  if (inp[0] == 'P') {
    if (inp[1] == '\0') {
      ctrl = &Pctrl;
      serial_println("P controller selected");
    } else if (inp[1] == 'I' && inp[2] == '\0') {
      ctrl = &PIctrl;
      serial_println("PI controller selected");
    }
  }
}

void PreOperational::set_parameters(char* inp)
{
  if (inp[0] == 'K' && inp[1] == 'p' && inp[2] == '\0') {
    serial_flush();
    serial_print("Enter a value for Kp: ");
    char Kp_str[7];
    serial_read_string(Kp_str,7);
    if (ctrl == &Pctrl) { // Kp sett fyrir þann controller sem er valinn
      Kp_inp_P = atof(Kp_str);
      Pctrl = P_controller(Kp_inp_P);
    } else {
      Kp_inp_PI = atof(Kp_str);
      PIctrl = PI_controller(Kp_inp_PI,Ti_inp);
    }
    serial_print("\nKp set to: ");
    serial_println(Kp_str);
  } else if (inp[0] == 'T' && inp[1] == 'i' && inp[2] == '\0') {
    serial_flush();
    serial_print("Enter a value for Ti: ");
    char Ti_str[7];
    serial_read_string(Ti_str,7);
    Ti_inp = atof(Ti_str);
    PIctrl = PI_controller(Kp_inp_PI,Ti_inp);
    serial_print("\nTi set to: ");
    serial_println(Ti_str);
  }
}


