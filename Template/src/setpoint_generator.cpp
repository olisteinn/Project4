#include "setpoint_generator.h"

static int32_t magnitude(int32_t value)
{
  return value < 0 ? -value : value;
}

SetpointGenerator::SetpointGenerator(int16_t initial_reference)
    : reference_(initial_reference),
      acceleration_remainder_(0),
      deceleration_remainder_(0)
{
}

int16_t SetpointGenerator::update(int16_t target,
                                  uint32_t acceleration_rpm_per_second,
                                  uint32_t deceleration_rpm_per_second,
                                  uint32_t elapsed_ms)
{
  if (target == reference_) {
    acceleration_remainder_ = 0;
    deceleration_remainder_ = 0;
    return reference_;
  }

  int32_t intermediate_target = target;
  const bool reversing =
      reference_ != 0 && target != 0 &&
      ((reference_ < 0) != (target < 0));

  const bool accelerating =
      !reversing &&
      magnitude(target) > magnitude(reference_);
  uint32_t limit = accelerating ? acceleration_rpm_per_second
                                : deceleration_rpm_per_second;
  uint8_t &remainder = accelerating ? acceleration_remainder_
                                    : deceleration_remainder_;

  if (reversing) {
    intermediate_target = 0;
  }

  if (limit == 0) {
    reference_ = target;
    acceleration_remainder_ = 0;
    deceleration_remainder_ = 0;
    return reference_;
  }

  const uint64_t budget = static_cast<uint64_t>(limit) * elapsed_ms + remainder;
  const uint64_t maximum_step = budget / 10;
  remainder = static_cast<uint8_t>(budget % 10);

  const int32_t difference = intermediate_target - reference_;
  const int32_t distance = magnitude(difference);
  if (maximum_step >= static_cast<uint64_t>(distance)) {
    reference_ = static_cast<int16_t>(intermediate_target);
    remainder = 0;
    if (reversing && reference_ == 0) {
      acceleration_remainder_ = 0;
      deceleration_remainder_ = 0;
    }
  } else if (maximum_step > 0) {
    reference_ = static_cast<int16_t>(
        reference_ + (difference > 0 ? static_cast<int32_t>(maximum_step)
                                     : -static_cast<int32_t>(maximum_step)));
  }

  if (reference_ == target) {
    acceleration_remainder_ = 0;
    deceleration_remainder_ = 0;
  }
  return reference_;
}

int16_t SetpointGenerator::reference() const
{
  return reference_;
}

void SetpointGenerator::reset(int16_t reference)
{
  reference_ = reference;
  acceleration_remainder_ = 0;
  deceleration_remainder_ = 0;
}
