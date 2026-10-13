// moves messages on and off the wire

#pragma once
#include <stdint.h>

// Can message framework
struct CanMsg {
  uint32_t id;
  uint8_t  dlc;
  uint8_t  data[8];
};

bool can_bus_init();