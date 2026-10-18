#include "timer.h"
#include <zephyr/kernel.h>

uint32_t time_mus()
{
  return static_cast<uint32_t>(k_cyc_to_us_floor64(k_cycle_get_64()));
}
