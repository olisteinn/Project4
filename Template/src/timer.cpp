#include "timer.h"
#include <zephyr/kernel.h>
#include <stdint.h>

volatile uint32_t ms;
volatile bool loop1 = false;
volatile bool loop2 = false;
uint8_t loop1_ms=0;
uint8_t loop1_cnt=0;
uint16_t loop2_ms=0;
uint16_t loop2_cnt=0;

void time_init() {
    ms = 0;
    loop1_cnt = 0;
    loop2_cnt = 0;
    loop1 = false;
    loop2 = false;
    
    // Start the Zephyr timer: first expiry in 1ms, repeating every 1ms
    k_timer_start(&tick_timer, K_MSEC(1), K_MSEC(1));
}

uint32_t time_ms() {
    // Replaces AVR register reading with the tracked millisecond counter
    return ms;
}
uint32_t time_mus() {
    // Replaces raw TCNT1 register math by combining ms with Zephyr cycle counts
    uint32_t current_ms = ms;
    uint32_t us_in_ms = k_cyc_to_us_floor32(k_cycle_get_32()) % 1000;
    return (current_ms * 1000UL) + us_in_ms;
}

void set_loop_ms(uint8_t loop1_arg, uint16_t loop2_arg) {
    loop1_ms = loop1_arg;
    loop2_ms = loop2_arg;
}

// Replaces the AVR ISR (TIMER1_COMPA_vect)
static void timer_expiry_cb(struct k_timer *timer) {
    ms++;
    
    if (loop1_ms > 0) {
        loop1_cnt++;
        if (loop1_cnt >= loop1_ms) {
            loop1 = true;
            loop1_cnt = 0;
        }
    }
    
    if (loop2_ms > 0) {
        loop2_cnt++;
        if (loop2_cnt >= loop2_ms) {
            loop2 = true;
            loop2_cnt = 0;
        }
    }
}





