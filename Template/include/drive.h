#pragma once
#include "digital_out.h"
#include <stdint.h>
#include <zephyr/drivers/pwm.h>


class Drive {
    private:
        const struct pwm_dt_spec *Pfwd;
        const struct pwm_dt_spec *Prev;
        Digital_out Pslp(SLP);
        void set_duty(const struct pwm_dt_spec *ch, uint8_t duty);
    public:
        Drive(uint8_t Pslp_arg);
        void init(void);
        void fwd(uint8_t speed);
        void rev(uint8_t speed);
        void run(int16_t speed);
        void stop(void); // .stop() stoppar snúning
        void sleep(void); // .sleep() stoppar snúning og slekkur á driver/brú
        void wake(void); // .wake() kveikir á driver/brú
};