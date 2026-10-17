// What the CANopen layer is allowed to ask of the application.
#pragma once
#include <stdint.h>

void app_set_target_speed(int32_t rpm);  // target velocity in RPM