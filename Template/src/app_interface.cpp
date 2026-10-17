#include "app_interface.h"
#include <stdint.h>
#include "controller.h"
#include "P_controller.h"
#include "PI_controller.h"

extern int16_t target_speed;
extern int16_t currspeed;
extern int16_t ctrlOP; 
static const int32_t max_rpm = 100;

extern controller*   ctrl;
extern P_controller  Pctrl;
extern PI_controller PIctrl;

extern double Kp_inp_P;
extern double Kp_inp_PI;
extern double Ti_inp;
static uint32_t profile_acc = 0;
static uint32_t profile_dec = 0;


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

int32_t app_get_target_speed(){
    return target_speed/100;
}

int32_t app_get_actual_speed(){
    return currspeed/100; // currspeed is rpm*100
}   

int16_t app_get_duty(){
    // int32_t so ctrlOP*1000 doesn't overflow 16 bits
    return (int16_t)((int32_t)ctrlOP * 1000 / 32640);  // ±32640 -> ±1000 (0.1 %)
}


uint8_t app_get_controller_type(){
    if (ctrl == &Pctrl){
        return 0;
    }
    if (ctrl == &PIctrl){
        return 1;
    }
    return 0;
}

bool app_set_controller_type(uint8_t type){
    if (type==0){
        ctrl = &Pctrl;
    }
    else if (type==1){
        ctrl = &PIctrl;
    }
    else{
        return false;
    }
    return true;
}

float app_get_kp(){
    if (ctrl==&Pctrl){
        return Kp_inp_P;
    }
    else if (ctrl==&PIctrl){
        return Kp_inp_PI;
    }
    return 0;
}

bool app_set_kp(float kp){
    if (kp <= 0.0f || kp >= 128.0f) return false;

    if (ctrl == &PIctrl) {
        Kp_inp_PI = kp;
        PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
    } else {
        Kp_inp_P = kp;
        Pctrl = P_controller(Kp_inp_P);
    }
    return true;
}

float app_get_ti(){
    return Ti_inp;
}

bool app_set_ti(float ti){
    if (ti <= 0.0f) return false;

    Ti_inp = ti;
    PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
    return true;
}

uint32_t app_get_accel_limit(){
    return profile_acc;
}

bool app_set_accel_limit(uint32_t accel_limit){
    profile_acc = accel_limit;
    return true;
}

uint32_t app_get_dec_limit(){
    return profile_dec;
}

bool app_set_dec_limit(uint32_t dec_limit){
    profile_dec = dec_limit;
    return true;
}


