#pragma once
#include <stdint.h>
#include <avr/io.h>

// .is_hi() og .is_lo() til að nota. Skila bool: true/false
// Öll þessi #define þýða bara pinnamerkingar yfir í actual númerin á pinnunum
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

class Digital_in {
    private:
        uint8_t pinMask;
        volatile uint8_t *ddrReg;
        volatile uint8_t *portReg;
        volatile uint8_t *pinReg;
    public:
        Digital_in(int pin) {
            // Þetta velur bara rétta pointera fyrir DDR, port og pin
            if (pin <= 7) {
                pinMask = (1 << pin);
                ddrReg = &DDRD;
                portReg = &PORTD;
                pinReg = &PIND;
                if (pin == 2) { // virkir sjálfkrafa INT0 interrupts ef pinninn er D2
                    EICRA |= (1 << ISC00);
                    EICRA &= ~(1 << ISC01);
                    EIMSK |= (1 << INT0);
                } 
                else if (pin == 3) { // virkir sjálfkrafa INT1 interrupts ef pinninn er D3
                    EICRA |= (1 << ISC10);
                    EICRA &= ~(1 << ISC11);
                    EIMSK |= (1 << INT1);
                }
            } else if (pin >= 8 && pin <= 13) {
                pinMask = (1 << (pin - 8));
                ddrReg = &DDRB;
                portReg = &PORTB;
                pinReg = &PINB;
            } else if (pin >= 14 && pin <= 19) {
                pinMask = (1 << (pin - 14));
                ddrReg = &DDRC;
                portReg = &PORTC;
                pinReg = &PINC;
            }
            
        }

        void init() {
            *ddrReg &= ~pinMask;
            *portReg |= pinMask;
        }

        bool is_hi() {
            // skilar true ef hi, false ef lo
            return (*pinReg & pinMask) != 0;
        }

        bool is_lo() {
            // sama nema öfugt. Þannig séð óþarfi
            return (*pinReg & pinMask) == 0;
        }
};