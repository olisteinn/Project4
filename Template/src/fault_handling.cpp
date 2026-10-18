#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>
#include "Fault_handling.h"
#include "control_task.h"
#include "state_events.h"

static atomic_t fault_pending = ATOMIC_INIT(0);

static const struct gpio_dt_spec fault_pin = GPIO_DT_SPEC_GET(DT_ALIAS(flt), gpios);

static struct gpio_callback fault_cb_data;

static void fault_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    atomic_set(&fault_pending, 1);
    control_request_fault_stop();
    state_event_notify();
}

void initFaultPin() {
    if (!device_is_ready(fault_pin.port)) {
        return;
    }
    gpio_pin_configure_dt(&fault_pin, GPIO_INPUT);
    gpio_init_callback(&fault_cb_data, fault_isr, BIT(fault_pin.pin));
    gpio_add_callback(fault_pin.port, &fault_cb_data);
    gpio_pin_interrupt_configure_dt(&fault_pin, GPIO_INT_EDGE_TO_ACTIVE);
}

bool check_fault() {
    return atomic_cas(&fault_pending, 1, 0);
}
