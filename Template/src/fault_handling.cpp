#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "drive.h"
#include "Fault_handling.h"

extern Drive bridge;

static volatile bool faultFlag = false;

/* Get the fault pin configuration from the device tree alias "flt" */
static const struct gpio_dt_spec fault_pin = GPIO_DT_SPEC_GET(DT_ALIAS(flt), gpios);

/* Callback structure for the GPIO interrupt */
static struct gpio_callback fault_cb_data;

static void fault_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    bridge.sleep();
    faultFlag = true;
}

void initFaultPin() {
    // Check if the GPIO controller device is ready
    if (!device_is_ready(fault_pin.port)) {
        return;
    }

    // Configure pin as input
    gpio_pin_configure_dt(&fault_pin, GPIO_INPUT);

    // Initialize and add the callback for the interrupt
    gpio_init_callback(&fault_cb_data, fault_isr, BIT(fault_pin.pin));
    gpio_add_callback(fault_pin.port, &fault_cb_data);

    // Configure interrupt trigger (edge falling)
    gpio_pin_interrupt_configure_dt(&fault_pin, GPIO_INT_EDGE_TO_ACTIVE);

    // 5. Check initial state at startup
    if (gpio_pin_get_dt(&fault_pin) > 0) {
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

