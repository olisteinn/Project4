#include "app_interface.h"
#include <stdint.h>

extern int16_t target_speed;

static const int32_t max_rpm = 100;

void app_set_target_speed(int32_t rpm){
    // To make sure int32 doesn't overflow when converting to int16. since the motor already has a max rpm of 100
    if (rpm > max_rpm){
        rpm = max_rpm;
    }

    if (rpm<-max_rpm){
        rpm = -max_rpm;
    }
    target_speed = (int16_t)(rpm*100);
}