#include "drive.h"
#include <zephyr/sys/printk.h>

static const struct pwm_dt_spec ain1 = PWM_DT_SPEC_GET(DT_ALIAS(ain1));
static const struct pwm_dt_spec ain2 = PWM_DT_SPEC_GET(DT_ALIAS(ain2));

Drive::Drive(uint8_t Pslp_arg) : Pslp(Pslp_arg) {
    Pfwd = &ain1;
    Prev = &ain2;
}

void Drive::set_duty(const struct pwm_dt_spec *ch, uint8_t duty) {
    uint32_t pulse = (uint32_t)((uint64_t)ch->period * duty / 255);
    pwm_set_pulse_dt(ch, pulse); // wants pulse width in ns.
}

void Drive::init() {
    Pslp.init();
    if (!pwm_is_ready_dt(Pfwd) || !pwm_is_ready_dt(Prev)) {
        printk("PWM device not ready\n");
        return;
    }
    stop();
}

void Drive::fwd(uint8_t speed) {
    set_duty(Prev, 0);
    set_duty(Pfwd, speed);
}

void Drive::rev(uint8_t speed) {
    set_duty(Pfwd, 0);
    set_duty(Prev, speed);
}

void Drive::run(int16_t speed) {
    if (speed == 0) {
        stop();
    } else if (speed > 0) {
        fwd((uint8_t)(speed >> 7));
    } else {
        rev((uint8_t)(-speed >> 7));
    }
}

void Drive::stop(void) {
    set_duty(Pfwd, 0);
    set_duty(Prev, 0);
}

void Drive::sleep() {
    stop();
    Pslp.set_lo();
}

void Drive::wake() {
    stop();
    Pslp.set_hi();
}