#pragma once
#include <avr/io.h>
#include <stdint.h>
#include "digital_in.h"
#include "digital_out.h"

class Encoder {
    private:
        Digital_in P1;
        Digital_in P2;
        Digital_out Pout;
        volatile int32_t counter; // bara position nema loopar ekki
        const int16_t rev_res = 1050; // Steps í einum hring á mótor
        volatile int16_t ext_counter; // position sem loopar
        volatile bool P1prevstate;
        volatile bool dir; // true fyrir áfram false fyrir afturábak. Fer samt eftir hvernig maður tengir hvort áttin er hvað
        struct History {
            // Þetta er til að reikna hraðann, hægt að averagea yfir seinustu X mörg counter steps
            volatile int8_t counter_mem;
            volatile uint32_t timestamps;
        };
        static const uint8_t history_length = 40; // Averagear hraðann yfir þetta mörg steps, max 254 eða 255, annaðhvort idk
        History enc_memory[history_length];
        volatile uint8_t history_head = 0; 
        const uint32_t timeout = 100000; // microsek, fyrir .speed()
        int16_t rpm; // Reiknuð rpm
    public:
        Encoder(int pin1, int pin2, int pin_out); 
        void init();
        void update(); // .update() bara til að lesa af encoder, reiknar hraðann í leiðinni
        int16_t position(); // .position() skilar counter, ekki búinn að setja inn þannig að hann resettar eftir hring
        bool direction(); // .direction() skilar "bool dir" sem er true/false
        int16_t speed(); // skilar rpm sem 16-bit int þar sem seinustu tveir stafir eru eftir kommu (skilar 10099 sem þýðir 100.99rpm)
};