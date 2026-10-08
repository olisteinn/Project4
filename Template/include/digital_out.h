#pragma once
#include <stdint.h>
#include <avr/io.h>

// allt nokkuð sjálfsagt
// mjög líkt og digital in
// .set_hi() .set_lo() og .toggle() til að nota

#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
#define D13 13

#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19

class Digital_out {
    private:
        uint8_t pinMask;
        volatile uint8_t *ddrReg;
        volatile uint8_t *portReg;
    public:
        Digital_out(int pin) {
            if (pin <= 7) {
                pinMask = (1<< pin);
                ddrReg = &DDRD;
                portReg = &PORTD;
            } else if (pin >= 8 && pin <= 13) {
                pinMask = (1<< (pin-8));
                ddrReg = &DDRB;
                portReg = &PORTB;
            } else if (pin >= 14 && pin <= 19) {
                pinMask = (1<< (pin-14));
                ddrReg = &DDRC;
                portReg = &PORTC;
            }   
        }

        void init() {
            *ddrReg |= pinMask;
            *portReg &= ~pinMask;
        }

        void set_hi() {
            *portReg |= pinMask;
        }

        void set_lo() {
            *portReg &= ~pinMask;
        }

        void toggle() {
            *portReg ^= pinMask;
        }
};