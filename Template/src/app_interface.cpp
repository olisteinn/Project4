#include "app_interface.h"
#include <stdint.h>

extern int16_t target_speed;
extern int16_t currspeed;
extern int16_t ctrlOP; 
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

int32_t app_get_actual_speed(){
    return currspeed/100; // currspeed is rpm*100
}   

int16_t app_get_duty(){
    // int32_t so ctrlOP*1000 doesn't overflow 16 bits
    return (int16_t)((int32_t)ctrlOP * 1000 / 32640);  // ±32640 -> ±1000 (0.1 %)
}