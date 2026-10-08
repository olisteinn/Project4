#include "drive.h"
#include "digital_out.h"
#include <avr/io.h>

Drive::Drive(uint8_t timer_circuit_no, uint8_t sleep_pin) : Pslp(sleep_pin) {
    TN = timer_circuit_no;
    if (TN == 0) { //timer 0 (D5 & D6)
        Pfwd = &OCR0B; // Pin D5
        Prev = &OCR0A; // Pin D6
    } else if (TN == 1) { //timer 1 (d9 & d10)
        Pfwd = &OCR1AL; //d9
        Prev = &OCR1BL; //d10, -L f. 8 bit er annars 16 bit (pwm er þá 0-65535)
    } else if (TN == 2) { //timer 1 (d9 & d10)
        Pfwd = &OCR2B; //d3
        Prev = &OCR2A; //d11
    }
}

void Drive::init() {
    Pslp.init();
    if (TN == 0) {
        DDRD |= (1 << 5) | (1 << 6); // setur pinna d5 & d6 sem output
        TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00); // setur timer X í fast PWM mode
        TCCR0B = (1 << CS01) | (1 << CS00); // (er á 7.8kHz) prescaler CS02 CS01 CS00, 001=16MHz/(1*255), 010=16MHz/(8*255), 011=16MHz/(64*255) etc
    } else if (TN == 1) {
        DDRB |= (1 << 1) | (1 << 2); //9&11 eru á B, 1 er D9 & 2 er D10
        TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10); // setur timer X í fast PWM mode (og 8-bit)
        TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
    } else if (TN == 2) {
        DDRD |= (1 << 3); //D3
        DDRB |= (1 << 3); //D11 er 3 á B
        TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << CS21); // (er á 976Hz) prescaler CS22 CS21 CS20
    }
    *Pfwd = 0;
    *Prev = 0;
}

void Drive::fwd(uint8_t speed) {
    *Prev = 0;
    *Pfwd = speed;
}

void Drive::rev(uint8_t speed) {
    *Pfwd = 0;
    *Prev = speed;
}

void Drive::run(int16_t speed) {
    if (speed == 0) {
        *Pfwd = 0;
        *Prev = 0;
    } else if (speed > 0) {
        *Prev = 0;
        *Pfwd = (uint8_t)(speed >> 7);
    } else {
        *Pfwd = 0;
        *Prev = (uint8_t)(-speed >> 7);
    }
}

void Drive::stop(void) {
    *Pfwd = 0;
    *Prev = 0;
}

void Drive::sleep() {
    *Pfwd = 0;
    *Prev = 0;
    Pslp.set_lo();
}

void Drive::wake() {
    *Pfwd = 0;
    *Prev = 0;
    Pslp.set_hi();
}