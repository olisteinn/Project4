#include "digital_out.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(digital_out, LOG_LEVEL_INF);

Digital_out::Digital_out(int pin)
{
  switch (pin) {
    case SLP:
      dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(slp), gpios);
      break;
    case LED:
      dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
      break;
    case ENC_OUT:
      dt_spec = GPIO_DT_SPEC_GET(DT_ALIAS(enc_out), gpios);
      break;
    default:
      valid = false;
      break;
  }
}

void Digital_out::init()
{
  if (!valid || !gpio_is_ready_dt(&dt_spec)) {
    LOG_ERR("GPIO output is invalid or device is not ready");
    return;
  }

  const int err = gpio_pin_configure_dt(&dt_spec, GPIO_OUTPUT_INACTIVE);
  if (err != 0) {
    LOG_ERR("Failed to configure GPIO output (%d)", err);
  }
}

void Digital_out::set_hi()
{
  const int err = gpio_pin_set_dt(&dt_spec, 1);
  if (err != 0) {
    LOG_ERR("Failed to set GPIO high (%d)", err);
  }
}

void Digital_out::set_lo()
{
  const int err = gpio_pin_set_dt(&dt_spec, 0);
  if (err != 0) {
    LOG_ERR("Failed to set GPIO low (%d)", err);
  }
}

void Digital_out::toggle()
{
  const int err = gpio_pin_toggle_dt(&dt_spec);
  if (err != 0) {
    LOG_ERR("Failed to toggle GPIO (%d)", err);
  }
}
