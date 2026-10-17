#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include "Fault_handling.h"
#include "control_task.h"
#include "state_events.h"

LOG_MODULE_REGISTER(fault_handling, LOG_LEVEL_INF);

static atomic_t fault_pending = ATOMIC_INIT(0);

/* Get the fault pin configuration from the device tree alias "flt" */
static const struct gpio_dt_spec fault_pin = GPIO_DT_SPEC_GET(DT_ALIAS(flt), gpios);

/* Callback structure for the GPIO interrupt */
static struct gpio_callback fault_cb_data;

static void fault_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    atomic_set(&fault_pending, 1);
    control_request_fault_stop();
    state_event_notify();
}

void initFaultPin() {
    if (!device_is_ready(fault_pin.port)) {
        LOG_ERR("Fault GPIO device is not ready");
        return;
    }

    int err = gpio_pin_configure_dt(&fault_pin, GPIO_INPUT);
    if (err != 0) {
        LOG_ERR("Failed to configure fault GPIO (%d)", err);
        return;
    }

    gpio_init_callback(&fault_cb_data, fault_isr, BIT(fault_pin.pin));
    err = gpio_add_callback(fault_pin.port, &fault_cb_data);
    if (err != 0) {
        LOG_ERR("Failed to register fault GPIO callback (%d)", err);
        return;
    }

    err = gpio_pin_interrupt_configure_dt(&fault_pin, GPIO_INT_EDGE_TO_ACTIVE);
    if (err != 0) {
        LOG_ERR("Failed to configure fault GPIO interrupt (%d)", err);
        return;
    }

    const int pin_state = gpio_pin_get_dt(&fault_pin);
    if (pin_state > 0) {
        atomic_set(&fault_pending, 1);
        control_request_fault_stop();
        state_event_notify();
    } else if (pin_state < 0) {
        LOG_ERR("Failed to read fault GPIO state");
    }
}

bool check_fault(){
    return atomic_cas(&fault_pending, 1, 0);
}
