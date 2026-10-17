#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart.h"
#include "Initialization.h"
#include "Operational.h"
#include "Context.h"
#include "digital_out.h"
#include "drive.h"
#include "encoder.h"
#include "controller.h"
#include "timer.h"
#include "StopState.h"
#include "PreOperational.h"

extern Digital_out led;
extern Drive bridge;
extern Encoder motor;
extern controller* ctrl;
extern int16_t target_speed;
char speed_str[10];
char print_str[100];
int16_t currspeed;
int16_t ctrlOP;
// struct record { // struct for bulk printing recorded values AFTER program run
//   int16_t pwmmem;
//   int16_t speedrecord;
// };

// static const uint16_t recordlength = 200;
// uint16_t recordhead = 0;
// record Record_run[recordlength];
// bool print_flag = false;

void Operational::on_do()
{
  if (loop1 == true){
    loop1 = false;
    currspeed = motor.speed();
    ctrlOP = ctrl->update(target_speed,currspeed); // ctrlOP = controller output, int16_t 
    bridge.run(ctrlOP); // .run(int16_t) les signed 16 bit on þýðir:
    //[-1,-32768] í .rev([0,255]) og [1,32767] í .fwd([0,255])
  }
  if (loop2 == true) {
    loop2 = false;
    sprintf(speed_str,"%4d.%2d",currspeed/100,abs(currspeed)%100);
    sprintf(print_str,"\rTarget speed: %4d.%1d rpm    True speed: %s rpm    PWM: %3d    ",
      target_speed/100,abs(target_speed)%100,speed_str, ctrlOP>>7);
    // sprintf(print_str,"\r%6lu, %4d.%1d, %s, %3d",
    //   time_ms(),target_speed/100,abs(target_speed)%100,speed_str, ctrlOP>>7);
    serial_print(print_str);
  }
  // if (loop2 == true) {
  //   loop2 = false;
  //   Record_run[recordhead].speedrecord = currspeed;
  //   Record_run[recordhead].pwmmem = ctrlOP>>7;
  //   recordhead++;
  //   if (recordhead >= recordlength) {
  //     recordhead=0;
  //     char print_str[80];
  //     char speed_str[10];
  //     char ts_str[32];
  //     sprintf(ts_str,"Target speed: %4d.%02d", target_speed/100,abs(target_speed)%100);
  //     serial_println("-------START-------");
  //     serial_println(ts_str);
  //     for (uint16_t i = 0; i < recordlength; i++) {
  //       sprintf(speed_str, "%4d.%02d", Record_run[i].speedrecord/100, abs(Record_run[i].speedrecord)%100);
  //       sprintf(print_str, "%6d,%7s,%d\n",i*20,speed_str,Record_run[i].pwmmem);
  //       serial_print(print_str);
  //     }
  //     serial_println("--------END--------");
  //   }
  // }
}

void Operational::on_entry()
{
  serial_println("Operational");
  led.set_hi();
  bridge.wake();
}

void Operational::on_exit()
{
  led.set_lo();
  bridge.stop();
  serial_print("Operational -> ");
}

void Operational::reset()
{
  this->context_->transition_to(&init_state);
}

void Operational::on_operate()
{

}

void Operational::on_pre_operate()
{
  this->context_->transition_to(&pre_op_state);
}

void Operational::on_fault()
{
    this->context_->transition_to(&stop_state);
}