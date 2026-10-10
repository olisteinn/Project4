#include "encoder.h"
#include "timer.h"
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>

extern Encoder motor;
static const struct gpio_dt_spec c1spec = GPIO_DT_SPEC_GET(DT_ALIAS(c1), gpios);

void c1_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    motor.position();
}

Encoder::Encoder(int pin1, int pin2, int pin_out) 
  : P1(pin1), P2(pin2), Pout(pin_out) 
{
}

void Encoder::init() {
    gpio_pin_interrupt_configure_dt(&c1spec, GPIO_INT_EDGE_RISING);
    gpio_init_callback(&cb_data, c1_isr, BIT(c1spec.pin));
    gpio_add_callback(c1spec.port, &cb_data);
    P1.init();
    P2.init();
    Pout.init();
    P1prevstate = P1.is_hi();
    
    counter = 0;
    ext_counter = 0;
    dir = true;
    rpm = 0;
    history_head = 0;
    memset(enc_memory, 0, sizeof(enc_memory));
}

void Encoder::update() {
    bool P1currstate = P1.is_hi();
    if (P1currstate == true) {
        if (P1currstate != P1prevstate) {
            bool P2currstate = P2.is_hi();
            if (P1currstate != P2currstate) {
                counter++;
                ext_counter++;
                dir = true;
            } else {
                counter--;
                ext_counter--;
                dir = false;
            }
            P1prevstate = P1currstate;

            history_head++;
            if (history_head >= history_length) {
                history_head=0;
            }
            enc_memory[history_head].timestamps = time_mus();
            enc_memory[history_head].counter_mem = (int8_t)counter;

        if (ext_counter >= rev_res || -rev_res >= ext_counter) {
            ext_counter = 0;
        }
        }
    } else {
        P1prevstate = P1currstate;
    }
}

bool Encoder::direction() {
    return dir;
}

int16_t Encoder::position() {
    int16_t val = ext_counter;
    return val;
}

int16_t Encoder::speed() {
    uint8_t head = history_head;
    uint8_t tail = head + 1;
    if (tail >= history_length) {
        tail = 0;
    }
        uint32_t new_time = enc_memory[head].timestamps;
        uint32_t old_time = enc_memory[tail].timestamps;
        int8_t delta_pos = enc_memory[head].counter_mem - enc_memory[tail].counter_mem;
    if ((time_mus() - new_time) > timeout) {
        rpm = 0;
        return rpm;
    }
    int32_t delta_time = new_time - old_time;
    if (delta_time == 0) {
        return rpm; 
    }
    rpm = 100 * (60000000L/rev_res) * delta_pos / delta_time;
    return rpm;
}