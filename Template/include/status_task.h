#pragma once

enum class StatusMode {
  Initializing,
  PreOperational,
  Operational,
  Stopped
};

bool status_task_start();
void status_task_set_mode(StatusMode mode);
