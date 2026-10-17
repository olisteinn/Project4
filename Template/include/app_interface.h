// What the CANopen layer is allowed to ask of the application.
#pragma once
#include <stdint.h>

void app_set_target_speed(int32_t rpm);  // target velocity in RPM
int32_t app_get_actual_speed();   // actual velocity in RPM
int16_t app_get_duty();           //output duty cycle in 0.1 %, -1000..1000