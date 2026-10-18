#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include "control_task.h"
#include "app_interface.h"
#include "canopen.h"
#include "drive.h"
#include "encoder.h"
#include "setpoint_generator.h"

extern Encoder motor;
extern Drive bridge;

K_SEM_DEFINE(control_tick, 0, 1);
K_MUTEX_DEFINE(telemetry_mutex);

static atomic_t control_enabled = ATOMIC_INIT(0);
static atomic_t fault_stop_pending = ATOMIC_INIT(0);
static atomic_t target_reference = ATOMIC_INIT(5000);
static ControlTelemetry telemetry = {};

static K_THREAD_STACK_DEFINE(control_stack, 2048);
static struct k_thread control_thread;
static struct k_timer control_timer;
static SetpointGenerator setpoint;
static constexpr uint32_t kControlPeriodMs = 5;
// static constexpr uint16_t kTpdo1EveryUpdates = 20;

static void control_timer_expired(struct k_timer *)
{
  k_sem_give(&control_tick);
}

static void control_thread_entry(void *, void *, void *)
{
  bool motor_enabled = false;
  // uint16_t updates_in_operational = 0;

  while (true) {
    k_sem_take(&control_tick, K_FOREVER);

    if (atomic_get(&control_enabled) == 0) {
      // updates_in_operational = 0;
      if (atomic_cas(&fault_stop_pending, 1, 0)) {
        bridge.sleep();
        motor_enabled = false;
      } else if (motor_enabled) {
        bridge.stop();
        motor_enabled = false;
      }
      continue;
    }

    motor.toggle_timing_probe();

    if (!motor_enabled) {
      bridge.wake();
      motor_enabled = true;
    }

    int16_t actual = motor.speed();
    int16_t reference = setpoint.update(
        (int16_t)atomic_get(&target_reference),
        app_get_accel_limit(), app_get_dec_limit(), kControlPeriodMs);
    int16_t output = app_controller_update(reference, actual);
    bridge.run(output);

    k_mutex_lock(&telemetry_mutex, K_FOREVER);
    telemetry.actual_speed = actual;
    telemetry.output = output;
    k_mutex_unlock(&telemetry_mutex);

    // if (++updates_in_operational >= kTpdo1EveryUpdates) {
    //   updates_in_operational = 0;
    //   canopen_send_tpdo1();
    // } // tekið út fyrir 3.3 optional SYNC
  }
}

void control_task_start()
{
  // Control (2) preempts CAN (3) and the main thread (5).
  k_thread_create(&control_thread, control_stack,
                  K_THREAD_STACK_SIZEOF(control_stack),
                  control_thread_entry, nullptr, nullptr, nullptr,
                  K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

  k_timer_init(&control_timer, control_timer_expired, nullptr);
  k_timer_start(&control_timer, K_MSEC(kControlPeriodMs),
                K_MSEC(kControlPeriodMs));
}

void control_set_enabled(bool enabled)
{
  atomic_set(&control_enabled, enabled ? 1 : 0);
  if (!enabled) {
    k_sem_give(&control_tick);
  }
}

void control_request_fault_stop()
{
  atomic_set(&control_enabled, 0);
  atomic_set(&fault_stop_pending, 1);
  k_sem_give(&control_tick);
}

void control_set_target_speed(int16_t target)
{
  atomic_set(&target_reference, target);
}

int16_t control_get_target_speed()
{
  return (int16_t)atomic_get(&target_reference);
}

ControlTelemetry control_get_telemetry()
{
  ControlTelemetry snapshot;
  k_mutex_lock(&telemetry_mutex, K_FOREVER);
  snapshot = telemetry;
  k_mutex_unlock(&telemetry_mutex);
  return snapshot;
}
