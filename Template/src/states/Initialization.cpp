#include <Initialization.h>
#include <Operational.h>
#include <Context.h>
#include "encoder.h"
#include "drive.h"
#include "digital_out.h"
#include "Fault_handling.h"
#include "PreOperational.h"
#include "P_controller.h"
#include "PI_controller.h"
#include "canopen.h"
#include <zephyr/logging/log.h>
#include "status_task.h"

LOG_MODULE_REGISTER(initialization, LOG_LEVEL_INF);

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

void Initialization::on_entry()
{
  LOG_INF("Initialization");
  status_task_set_mode(StatusMode::Initializing);
  motor.init();  // initalize encoder
  bridge.init();  // initalize motor driver
  led.init();
  initFaultPin();

  unsigned int key = irq_lock();
  Pctrl  = P_controller(Kp_init_P);
  PIctrl = PI_controller(Kp_init_PI, Ti_init_PI);
  Kp_inp_P  = Kp_init_P;
  Kp_inp_PI = Kp_init_PI;
  Ti_inp    = Ti_init_PI;
  ctrl   = &Pctrl;           

  irq_unlock(key);
  canopen_send_bootup();
  this->context_->transition_to(&pre_op_state);
}

void Initialization::on_exit()
{
  LOG_INF("Boot up complete");
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
