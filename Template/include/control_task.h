#pragma once

#include <stdint.h>

struct ControlTelemetry {
  int16_t actual_speed;
  int16_t output;
};

bool control_task_start();
void control_set_enabled(bool enabled);
void control_request_fault_stop();
void control_set_target_speed(int16_t target);
int16_t control_get_target_speed();
ControlTelemetry control_get_telemetry();
