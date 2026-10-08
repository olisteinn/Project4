#pragma once
#include "digital_out.h"
#include <stdint.h>


class Drive {
    private:
        volatile uint8_t *Pfwd;
        volatile uint8_t *Prev;
        volatile uint8_t *CTRLreg;
        uint8_t TN;
        Digital_out Pslp;
        uint8_t speed;
    public:
        Drive(uint8_t TN_arg, uint8_t Pslp_arg);
        void init(void);
        void fwd(uint8_t speed);
        void rev(uint8_t speed);
        void run(int16_t speed);
        void stop(void); // .stop() stoppar snúning
        void sleep(void); // .sleep() stoppar snúning og slekkur á driver/brú
        void wake(void); // .wake() kveikir á driver/brú
};