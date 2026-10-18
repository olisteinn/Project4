#include "StopState.h"
#include "Context.h"
#include "Initialization.h"
#include "Operational.h"
#include "PreOperational.h"
#include "status_task.h"
#include "control_task.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stop_state, LOG_LEVEL_INF);

void StopState::on_entry()
{
  LOG_INF("Stopped");
  control_set_enabled(false);
  status_task_set_mode(StatusMode::Stopped);
}

void StopState::on_exit()
{
  LOG_INF("Leaving Stopped");
}

void StopState::reset()
{
  this->context_->transition_to(&init_state);
}

void StopState::on_operate()
{
  this->context_->transition_to(&operational_state);
}

void StopState::on_pre_operate()
{
  this->context_->transition_to(&pre_op_state);
}

void StopState::on_fault()
{
}
