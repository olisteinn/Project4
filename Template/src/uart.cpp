#include "uart.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static int peeked = -1;   // one-character buffer, -1 = empty

void serial_init(void) {
    if (!device_is_ready(uart_dev)) {
            printk("Console UART not ready\n");
        }
}

void serial_print_char(char c) {
    uart_poll_out(uart_dev, c); // waits until the UART can take the byte, then sends it
}

void serial_print(const char* str) {
    // Loop through the character array until we hit the null terminator ('\0')
    while (*str) {
        serial_print_char(*str); // Print the current character
        str++;                   // Move to the next character memory address
    }
}

void serial_println(const char* str) {
    serial_print(str);
    serial_print_char('\r'); // Carriage return
    serial_print_char('\n'); // Line feed
}

uint8_t serial_available(void) {
    if (peeked >= 0) return 1;           // already have one waiting
    unsigned char c;
    if (uart_poll_in(uart_dev, &c) == 0) {   // 0 = got a byte, -1 = nothing there
        peeked = c;
        return 1;
    }
    return 0;
}

char serial_read(void) {
    while (!serial_available()) {
        k_yield();  // let other threads run while waiting
    }
    char c = (char)peeked;
    peeked = -1;
    return c;
}

void serial_read_string(char* buffer, uint8_t max_length) {
    uint8_t index = 0;
    while (index < max_length - 1) {
        char c = serial_read();
        
        // Stop reading if we hit a newline or carriage return
        if (c == '\n' || c == '\r') {
            break; 
        }
        
        buffer[index] = c;
        index++;
    }
    // Always add a null terminator so C knows where the string ends
    buffer[index] = '\0'; 
}

void serial_flush(void) {
    unsigned char c;
    peeked = -1;
    while (uart_poll_in(uart_dev, &c) == 0) { }   // throw away anything waiting
}