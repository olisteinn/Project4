#pragma once
#include <stdint.h>
void time_init();

uint32_t time_ms();

uint32_t time_mus();

void set_loop_ms(uint8_t loop1_arg, uint16_t loop2_arg);

extern volatile bool loop1;
extern volatile bool loop2;