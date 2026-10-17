// What the CANopen layer is allowed to ask of the application.
#pragma once
#include <stdint.h>

void app_set_target_speed(int32_t rpm);  // target velocity in RPM
int32_t app_get_target_speed();
int32_t app_get_actual_speed();   // actual velocity in RPM
int16_t app_get_duty();           //output duty cycle in 0.1 %, -1000..1000


// SDO write read:

uint8_t app_get_controller_type();
bool app_set_controller_type(uint8_t type);
float app_get_kp();
bool app_set_kp(float kp);
float app_get_ti();
bool app_set_ti(float ti);
uint32_t app_get_accel_limit(); // RPM/s, 0 = no limit
bool app_set_accel_limit(uint32_t accel_limit);
uint32_t app_get_dec_limit(); // RPM/s, 0 = no limit
bool app_set_dec_limit(uint32_t accel_limit);



