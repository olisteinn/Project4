#include <avr/io.h>
#include <avr/interrupt.h>
#include "drive.h"
#include "Fault_handling.h"

extern Drive bridge;

static volatile bool faultFlag = false;

void initFaultPin() {
    DDRD &= ~(1 << DDD3);
    PORTD |= (1 << PORTD3);
    EICRA = (EICRA & ~((1 << ISC11) | (1 << ISC10))) | (1 << ISC11);
    EIMSK |= (1 << INT1);
    if (!(PIND & (1 << PIND3))) {
        faultFlag = true;
    }
}

bool check_fault(){
    if (faultFlag == true){
        faultFlag = false;
        return true;
    }
    else{
        return false;
    }
}

ISR(INT1_vect) {
    bridge.sleep();
    faultFlag = true;
}
