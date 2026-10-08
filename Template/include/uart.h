#pragma once
#include <stdint.h>


void serial_init(void);
void serial_print_char(char c);
void serial_print(const char* str);
void serial_println(const char* str);

uint8_t serial_available(void);   // 1 if a received byte is waiting, else 0
char serial_read(void);        // returns the received byte
void serial_read_string(char* buffer, uint8_t max_length);
void serial_flush(void);