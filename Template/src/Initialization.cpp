#include "uart.h"
#include <Initialization.h>
#include <Operational.h>
#include <Context.h>
#include "encoder.h"
#include "drive.h"
#include "digital_out.h"
#include "timer.h"
#include <avr/interrupt.h>
#include "Fault_handling.h"
#include "PreOperational.h"
#include "P_controller.h"
#include "PI_controller.h"


extern Encoder motor;
extern Drive bridge;

extern Digital_out led;

extern double Kp_init_P;
extern double Kp_init_PI;
extern double Ti_init_PI;
extern double Kp_inp_P;
extern double Kp_inp_PI;
extern double Ti_inp;
extern P_controller Pctrl;
extern PI_controller PIctrl;
extern controller* ctrl;

void Initialization::on_do()
{

}

void Initialization::on_entry()
{
  serial_println("Initialization");
  cli();
  time_init(); // initalize timer 1 for time tracking use
  motor.init();  // initalize encoder
  bridge.init();  // initalize motor driver
  set_loop_ms(5,20); // sets loop durations in ms, one for controller loop other for serial print
  led.init();
  initFaultPin();

  Pctrl  = P_controller(Kp_init_P);
  PIctrl = PI_controller(Kp_init_PI, Ti_init_PI);
  Kp_inp_P  = Kp_init_P;
  Kp_inp_PI = Kp_init_PI;
  Ti_inp    = Ti_init_PI;
  ctrl   = &Pctrl;           

  sei();
  serial_println("Initialization complete");
  this->context_->transition_to(&pre_op_state);
}

void Initialization::on_exit()
{
  serial_println("Boot up complete");
  serial_print("Initialization -> ");
}

void Initialization::reset()
{

}

void Initialization::on_operate()
{
  
}

void Initialization::on_pre_operate()
{

}

void Initialization::on_fault()
{
}


