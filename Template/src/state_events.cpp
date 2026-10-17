#include <zephyr/kernel.h>

#include "state_events.h"

K_SEM_DEFINE(state_event, 0, 1);

void state_event_notify()
{
  k_sem_give(&state_event);
}

void state_event_wait()
{
  k_sem_take(&state_event, K_FOREVER);
}
