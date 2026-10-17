#include "Initialization.h"
#include "Operational.h"
#include "Context.h"
#include "control_task.h"
#include "StopState.h"
#include "PreOperational.h"
#include "status_task.h"
#include "app_interface.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(operational, LOG_LEVEL_INF);

void Operational::on_entry()
{
  LOG_INF("Operational");
  app_set_nmt_state(AppNmtState::Operational);
  control_set_enabled(true);
  status_task_set_mode(StatusMode::Operational);
}

void Operational::on_exit()
{
  control_set_enabled(false);
  LOG_INF("Leaving Operational");
}

void Operational::reset()
{
  this->context_->transition_to(&init_state);
}

void Operational::on_operate()
{
}

void Operational::on_pre_operate()
{
  this->context_->transition_to(&pre_op_state);
}

void Operational::on_fault()
{
  this->context_->transition_to(&stop_state);
}
