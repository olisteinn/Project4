#pragma once
#include <zephyr/drivers/gpio.h>

#define C1 1
#define C2 2

class Digital_in {
    private:
        struct gpio_dt_spec dt_spec;
        int pin;
    public:
        Digital_in(int pin_arg):pin{pin_arg} {
            switch (pin) {
                case C1: {
                    dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(c1),gpios);
                    break;
                }
                case C2: {
                    dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(c2),gpios);
                    break;
                }
                default:
                    return;
            }
        }

        void init() {
            if (!gpio_is_ready_dt(&dt_spec)) {
                return;
            }
            gpio_pin_configure_dt(&dt_spec, GPIO_INPUT);
        }

        bool is_hi() {
            return gpio_pin_get_dt(&dt_spec) == 1;
        }

        bool is_lo() {
            return gpio_pin_get_dt(&dt_spec) == 0;
        }
};