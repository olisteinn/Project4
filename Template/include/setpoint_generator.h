#pragma once

#include <stdint.h>

class SetpointGenerator {
 public:
  explicit SetpointGenerator(int16_t initial_reference = 0);

  int16_t update(int16_t target, uint32_t acceleration_rpm_per_second,
                 uint32_t deceleration_rpm_per_second, uint32_t elapsed_ms);

  int16_t reference() const;
  void reset(int16_t reference = 0);

 private:
  int16_t reference_;
  uint8_t acceleration_remainder_;
  uint8_t deceleration_remainder_;
};
