#pragma once

#include <stdint.h>

void app_set_target_speed(int32_t rpm);
int32_t app_get_target_speed();
int32_t app_get_actual_speed();
int16_t app_get_duty();

uint8_t app_get_controller_type();
void app_set_controller_type(uint8_t type);
float app_get_kp();
void app_set_kp(float kp);
float app_get_ti();
void app_set_ti(float ti);

uint32_t app_get_accel_limit();
void app_set_accel_limit(uint32_t accel_limit);
uint32_t app_get_dec_limit();
void app_set_dec_limit(uint32_t decel_limit);

int16_t app_controller_update(int16_t reference, int16_t actual);
