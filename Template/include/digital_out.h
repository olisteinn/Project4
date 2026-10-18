#pragma once
#include <zephyr/drivers/gpio.h>


#define SLP 1
#define LED 2
#define ENC_OUT 3


class Digital_out {
    private:
        struct gpio_dt_spec dt_spec;
    public:
        Digital_out(int pin);
        void init();
        void set_hi();
        void set_lo();
        void toggle();
};