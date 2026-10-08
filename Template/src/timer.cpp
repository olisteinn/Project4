#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint32_t ms;

volatile bool loop1 = false;
volatile bool loop2 = false;
uint8_t loop1_ms=0;
uint8_t loop1_cnt=0;
uint16_t loop2_ms=0;
uint16_t loop2_cnt=0;

void time_init() {
    cli();
    ms = 0;
    TCCR1A = 0; // Timer 1 control register A (set to 0)
    TCCR1B = 0; // T1 control register B
    TCNT1 = 0; // T1 counter = 0 
    TCCR1B |= (1 << WGM12); // clears timer on compare match
    TIMSK1 |= (1 << OCIE1A); // set interrupt on compare match
    // Prescaler: 
    // TCCR1B |= CS12 | CS11 | CS10
    // 0 0 0 = stopp
    // 0 0 1 = enginn prescaler -> 0.0625 microsek tick eða 1/16 af microsek
    // 0 1 0 = 1/8 eða 0.5 microsek
    // 0 1 1 = 1/64 eða 4 microsek
    // 1 0 0 = 1/256 eða 16 microsek
    // 1 0 1 = 1/1024 eða 64 microsek
    TCCR1B |= (1 << CS11) | (1 << CS10); // 1/64 prescaler
    // Þetta stillir hversu hratt interruptið keyrir, þarf að vera rétt miðað við prescaler:
    OCR1A = 249UL; // telur líka 0 þannig hvert 250-sta tick (millisek með 1/64 prescaler)

    loop1_cnt = 0;
    loop2_cnt = 0;
    loop1 = false;
    loop2 = false;

}

uint32_t time_ms() {
    uint8_t oldSREG = SREG; // interrupt state save, betra vs sei() ef þetta er keyrt milli cli(); og sei(); annarstaðar
    cli(); // disable interrupts
        uint32_t current_ms = ms;
    SREG = oldSREG; // interrupt state restore
    return current_ms;
}

uint32_t time_mus() {
    // Skilar mícrosek með að lesa beint af TCNT1 og bæta við ms
    // nákvæmni 4 míkrósek útaf prescaler samt
    uint8_t oldSREG = SREG; // interrupt state save
    cli(); // disable interrupts
        uint32_t current_ms = ms;
        uint16_t tcnt1_read = TCNT1; // les beint af TCNT1 (sem resettar hverja ms)
        if ((TIFR1 & (1 << OCF1A)) && (tcnt1_read < OCR1A)) {
            // checkar hvort TIFR1 sé on í OCF1A, sem gerist þegar TCNT1 == OCR1A 
            // og hvort TCNT1 hafi rúllað yfir 
            // Gerist stundum og þá á ms eftir að ++ en TCNT1 er búinn að resetta í 0
            // og þá les tíminn einni millisek of lítið
            current_ms++;
        }
    SREG = oldSREG; // interrupt state restore
    return current_ms*1000UL + tcnt1_read*4UL; // millisek * 1000 + TCNT1 * 4 = mícrósek
}

void set_loop_ms(uint8_t loop1_arg, uint16_t loop2_arg) {
    uint8_t oldSREG = SREG;
    cli(); // disable interrupts
    loop1_ms = loop1_arg;
    loop2_ms = loop2_arg;
    SREG = oldSREG;
}

ISR(TIMER1_COMPA_vect) { // Keyrir þegar TCNT1 == OCR1A, COMPB væri TCNT1 == OCR1B
    // Þetta er interrupt á timer 1 og telur millisek og passar loop control bool
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





