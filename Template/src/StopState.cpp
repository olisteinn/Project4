#include "StopState.h"
#include "Context.h"          
#include "Initialization.h"   
#include "Operational.h"      
#include "drive.h"
#include "digital_out.h"      
#include "uart.h"
#include "timer.h"
#include <stdint.h>
#include "PreOperational.h"

extern Drive bridge;
extern Digital_out led;
static uint32_t led_ms;

void StopState::on_do()
{
    if ((time_ms()-led_ms) >= 250) {
        led_ms = time_ms();
        led.toggle();
    }
}

void StopState::on_entry()
{
    serial_println("Stopped");
    led_ms = time_ms();
}

void StopState::on_exit()
{
    led.set_lo();
    serial_print("Stopped -> ");
}

void StopState::reset()
{
    this->context_->transition_to(&init_state);
}

void StopState::on_operate()
{
    this->context_->transition_to(&operational_state);
}

void StopState::on_pre_operate()
{
    this->context_->transition_to(&pre_op_state);
}

void StopState::on_fault()
{
}