#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "digital_out.h"
#include "status_task.h"
#include "app_interface.h"

LOG_MODULE_REGISTER(status, LOG_LEVEL_INF);

extern Digital_out led;

K_SEM_DEFINE(status_tick, 0, 1);

static atomic_t status_mode = ATOMIC_INIT(
    static_cast<atomic_val_t>(StatusMode::Initializing));
static K_THREAD_STACK_DEFINE(status_stack, 1536);
static struct k_thread status_thread;
static struct k_timer status_timer;

static void status_timer_expired(struct k_timer *)
{
  k_sem_give(&status_tick);
}

static void status_thread_entry(void *, void *, void *)
{
  uint16_t elapsed_ms = 0;
  StatusMode previous_mode = StatusMode::Initializing;

  while (true) {
    k_sem_take(&status_tick, K_FOREVER);
    const auto mode = static_cast<StatusMode>(atomic_get(&status_mode));
    if (mode != previous_mode) {
      elapsed_ms = 0;
      previous_mode = mode;
      if (mode == StatusMode::Operational) {
        led.set_hi();
      } else {
        led.set_lo();
      }
    }
    elapsed_ms += 20;

    switch (mode) {
      case StatusMode::Initializing:
        led.set_lo();
        break;
      case StatusMode::PreOperational:
        if (elapsed_ms >= 500) {
          elapsed_ms = 0;
          led.toggle();
        }
        break;
      case StatusMode::Operational: {
        LOG_INF("target=%d actual=%d output=%d",
                app_get_target_speed(),
                app_get_actual_speed(),
                app_get_duty());
        break;
      }
      case StatusMode::Stopped:
        if (elapsed_ms >= 250) {
          elapsed_ms = 0;
          led.toggle();
        }
        break;
    }
  }
}

bool status_task_start()
{
  if (k_thread_create(&status_thread, status_stack,
                      K_THREAD_STACK_SIZEOF(status_stack),
                      status_thread_entry, nullptr, nullptr, nullptr,
                      K_PRIO_PREEMPT(6), 0, K_NO_WAIT) == nullptr) {
    return false;
  }

  k_timer_init(&status_timer, status_timer_expired, nullptr);
  k_timer_start(&status_timer, K_MSEC(20), K_MSEC(20));
  return true;
}

void status_task_set_mode(StatusMode mode)
{
  atomic_set(&status_mode, static_cast<atomic_val_t>(mode));
}
