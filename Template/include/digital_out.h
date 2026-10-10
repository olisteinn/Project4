#pragma once
#include <zephyr/drivers/gpio.h>


#define SLP 1
#define LED 2
#define ENC_OUT 3


class Digital_out {
    private:
        struct gpio_dt_spec dt_spec;
        bool valid = true;
    public:
        Digital_out(int pin) {
            switch (pin)
            {
            case SLP:{
                dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(slp), gpios);
                break;
            }
            case LED:{
                dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
                break;
            }
            case ENC_OUT:{
                dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(enc_out), gpios);
                break;
            }
            default:
                valid = false;
                return;
            }
        }

        void init() {
            if (!valid || !gpio_is_ready_dt(&dt_spec)){
                printk("ERROR: GPIO not ready or invalid pin \n");
                return;
            }
            gpio_pin_configure_dt(&dt_spec, GPIO_OUTPUT_INACTIVE);
        }

        void set_hi() {
            gpio_pin_set_dt(&dt_spec, 1);
        }

        void set_lo() {
            gpio_pin_set_dt(&dt_spec, 0);
        }

        void toggle() {
            gpio_pin_toggle_dt(&dt_spec);
        }
};