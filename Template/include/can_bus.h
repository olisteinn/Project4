// moves messages on and off the wire

#pragma once
#include <stdint.h>

// Can message framework
struct CanMsg {
  uint32_t id;
  uint8_t  dlc;
  uint8_t  data[8];
};

bool can_bus_init(); //initialize the canbus
bool can_bus_receive(CanMsg &msg); // true if message was waiting
bool can_bus_send(const CanMsg &msg); // true if sent