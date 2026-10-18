#include <zephyr/kernel.h>

#include "digital_in.h"
#include "digital_out.h"
#include "encoder.h"
#include "drive.h"
#include "P_controller.h"
#include "PI_controller.h"
#include "controller.h"
#include "Context.h"
#include "Initialization.h"
#include "Operational.h"
#include "PreOperational.h"
#include "StopState.h"
#include "Fault_handling.h"
#include "canopen.h"
#include "can_bus.h"
#include "control_task.h"
#include "state_events.h"
#include "status_task.h"

static K_THREAD_STACK_DEFINE(can_thread_stack, 2048);
static struct k_thread can_thread;

Initialization init_state;
Operational operational_state;
PreOperational pre_op_state;
StopState stop_state;
Context *motor_state;

Encoder motor(C1, C2, ENC_OUT);
Drive bridge(SLP);
Digital_out led(LED);

double Kp_init_P = 8.21;
double Kp_init_PI = 7.39;
double Ti_init_PI = 0.08517;
double Kp_inp_P = Kp_init_P;
double Kp_inp_PI = Kp_init_PI;
double Ti_inp = Ti_init_PI;
P_controller Pctrl(Kp_init_P);
PI_controller PIctrl(Kp_init_PI, Ti_init_PI);
controller *ctrl = &Pctrl;

static void can_thread_entry(void *, void *, void *)
{
  while (true) {
    CanMsg msg;
    if (can_bus_receive_wait(msg)) {
      canopen_handle(msg);
    }
  }
}

int main()
{
  if (!can_bus_init()) {
    return -1;
  }

  Context ctx(&init_state);
  motor_state = &ctx;

  control_task_start();
  status_task_start();

  k_thread_create(&can_thread, can_thread_stack,
                  K_THREAD_STACK_SIZEOF(can_thread_stack),
                  can_thread_entry, nullptr, nullptr, nullptr,
                  K_PRIO_PREEMPT(3), 0, K_NO_WAIT);

  while (true) {
    state_event_wait();
    canopen_process_pending_nmt();
    if (check_fault()) {
      motor_state->on_fault();
    }
  }
}
