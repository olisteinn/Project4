#include <Initialization.h>
#include <Operational.h>
#include <Context.h>
#include "PreOperational.h"
#include "StopState.h"
#include "control_task.h"
#include "status_task.h"
#include "app_interface.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pre_operational, LOG_LEVEL_INF);

void PreOperational::on_entry()
{
  LOG_INF("Pre-Operational");
  app_set_nmt_state(AppNmtState::PreOperational);
  control_set_enabled(false);
  status_task_set_mode(StatusMode::PreOperational);
}

void PreOperational::on_exit()
{
  LOG_INF("Leaving Pre-Operational");
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
