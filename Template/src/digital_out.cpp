#include "digital_out.h"

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
      break;
  }
}

void Digital_out::init()
{
  if (!gpio_is_ready_dt(&dt_spec)) {
    return;
  }
  gpio_pin_configure_dt(&dt_spec, GPIO_OUTPUT_INACTIVE);
}

void Digital_out::set_hi()
{
  gpio_pin_set_dt(&dt_spec, 1);
}

void Digital_out::set_lo()
{
  gpio_pin_set_dt(&dt_spec, 0);
}

void Digital_out::toggle()
{
  gpio_pin_toggle_dt(&dt_spec);
}
