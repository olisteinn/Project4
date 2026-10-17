#include <unity.h>

#include "setpoint_generator.h"

void test_acceleration_ramps_and_reaches_target()
{
  SetpointGenerator ramp;
  for (int update = 0; update < 20; ++update) {
    ramp.update(1000, 100, 100, 5);
  }
  TEST_ASSERT_EQUAL_INT16(1000, ramp.reference());
}

void test_deceleration_uses_deceleration_limit()
{
  SetpointGenerator ramp(1000);
  TEST_ASSERT_EQUAL_INT16(900, ramp.update(0, 100, 200, 5));
}

void test_zero_limit_applies_step_immediately()
{
  SetpointGenerator ramp(1000);
  TEST_ASSERT_EQUAL_INT16(-2500, ramp.update(-2500, 0, 0, 5));
}

void test_reversing_ramps_down_through_zero_before_accelerating()
{
  SetpointGenerator ramp(1000);
  TEST_ASSERT_EQUAL_INT16(500, ramp.update(-1000, 1000, 1000, 5));
  TEST_ASSERT_EQUAL_INT16(0, ramp.update(-1000, 1000, 1000, 5));
  TEST_ASSERT_EQUAL_INT16(-500, ramp.update(-1000, 1000, 1000, 5));
}

void test_target_change_during_ramp_moves_toward_new_target()
{
  SetpointGenerator ramp;
  TEST_ASSERT_EQUAL_INT16(50, ramp.update(1000, 100, 100, 5));
  TEST_ASSERT_EQUAL_INT16(0, ramp.update(-1000, 100, 100, 5));
}

void test_fractional_step_budget_accumulates()
{
  SetpointGenerator ramp;
  ramp.update(100, 1, 1, 5);
  TEST_ASSERT_EQUAL_INT16(0, ramp.reference());
  ramp.update(100, 1, 1, 5);
  TEST_ASSERT_EQUAL_INT16(1, ramp.reference());
}

void test_zero_elapsed_time_does_not_advance_reference()
{
  SetpointGenerator ramp;
  TEST_ASSERT_EQUAL_INT16(0, ramp.update(1000, 100, 100, 0));
}

void test_zero_acceleration_limit_does_not_disable_deceleration_limit()
{
  SetpointGenerator ramp;
  TEST_ASSERT_EQUAL_INT16(1000, ramp.update(1000, 0, 10, 5));
  TEST_ASSERT_EQUAL_INT16(995, ramp.update(0, 0, 10, 5));
}

void test_reset_clears_fractional_step_remainder()
{
  SetpointGenerator ramp;
  ramp.update(100, 1, 1, 5);
  ramp.reset();
  TEST_ASSERT_EQUAL_INT16(0, ramp.update(100, 1, 1, 5));
}

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_acceleration_ramps_and_reaches_target);
  RUN_TEST(test_deceleration_uses_deceleration_limit);
  RUN_TEST(test_zero_limit_applies_step_immediately);
  RUN_TEST(test_reversing_ramps_down_through_zero_before_accelerating);
  RUN_TEST(test_target_change_during_ramp_moves_toward_new_target);
  RUN_TEST(test_fractional_step_budget_accumulates);
  RUN_TEST(test_zero_elapsed_time_does_not_advance_reference);
  RUN_TEST(test_zero_acceleration_limit_does_not_disable_deceleration_limit);
  RUN_TEST(test_reset_clears_fractional_step_remainder);
  return UNITY_END();
}