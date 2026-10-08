#include "uart.h"
#include <avr/io.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define BRC ((F_CPU / 16 / BAUD) - 1)

void serial_init(void) {
    // 1. Set the Baud Rate
    // UBRR0 is a 16-bit register split into High (H) and Low (L) bytes.
    UBRR0H = (BRC >> 8); 
    UBRR0L = BRC;

    // 2. Enable the Transmitter (TXEN0) and Receiver (RXEN0)
    // This physically connects pins 0 and 1 to the USART hardware.
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    // 3. Set Frame Format: 8 data bits, 1 stop bit, no parity
    // UCSZ01 and UCSZ00 configure the character size to 8 bits.
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void serial_print_char(char c) {
    // Wait for the Transmit Buffer to be empty.
    // The UDRE0 (USART Data Register Empty) bit goes to 1 when it's safe to write.
    while (!(UCSR0A & (1 << UDRE0))) {
        // Do nothing until the buffer clears
    }
    
    // Put the character into the data register. The hardware immediately sends it.
    UDR0 = c;
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
    // RXC0 (Receive Complete) is 1 when an unread byte is in UDR0.
    return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

char serial_read(void) {
    // Wait until data is available to be read
    while (!(UCSR0A & (1 << RXC0))) {
        // Do nothing until a character arrives
    }
    return UDR0;
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

void serial_flush() {
    // Keep reading and throwing away characters as long as there is unread data
    while (UCSR0A & (1 << RXC0)) {
        char dummy = UDR0; 
    }
}