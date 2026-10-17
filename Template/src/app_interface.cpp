#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <math.h>
#include <limits.h>

#include "app_interface.h"
#include "P_controller.h"
#include "PI_controller.h"
#include "control_task.h"
#include "controller.h"

extern controller *ctrl;
extern P_controller Pctrl;
extern PI_controller PIctrl;
extern double Kp_inp_P;
extern double Kp_inp_PI;
extern double Ti_inp;

static constexpr int32_t kMaxSpeedRpm = 100;
static atomic_t nmt_state = ATOMIC_INIT(
    static_cast<atomic_val_t>(AppNmtState::Initializing));
K_MUTEX_DEFINE(app_config_mutex);

static uint32_t profile_accel = 0;
static uint32_t profile_decel = 0;

void app_set_nmt_state(AppNmtState state)
{
  atomic_set(&nmt_state, static_cast<atomic_val_t>(state));
}

AppNmtState app_get_nmt_state()
{
  return static_cast<AppNmtState>(atomic_get(&nmt_state));
}

void app_set_target_speed(int32_t rpm)
{
  if (rpm > kMaxSpeedRpm) {
    rpm = kMaxSpeedRpm;
  } else if (rpm < -kMaxSpeedRpm) {
    rpm = -kMaxSpeedRpm;
  }
  control_set_target_speed(static_cast<int16_t>(rpm * 100));
}

int32_t app_get_target_speed()
{
  return control_get_target_speed() / 100;
}

int32_t app_get_actual_speed()
{
  return control_get_telemetry().actual_speed / 100;
}

int16_t app_get_duty()
{
  const int32_t output = control_get_telemetry().output;
  return static_cast<int16_t>(output * 1000 / (255 * 128));
}

uint8_t app_get_controller_type()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const uint8_t type = ctrl == &PIctrl ? 1 : 0;
  k_mutex_unlock(&app_config_mutex);
  return type;
}

bool app_set_controller_type(uint8_t type)
{
  if (type > 1) {
    return false;
  }

  k_mutex_lock(&app_config_mutex, K_FOREVER);
  ctrl = type == 0 ? static_cast<controller *>(&Pctrl)
                   : static_cast<controller *>(&PIctrl);
  k_mutex_unlock(&app_config_mutex);
  return true;
}

float app_get_kp()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const float kp = ctrl == &PIctrl ? static_cast<float>(Kp_inp_PI)
                                    : static_cast<float>(Kp_inp_P);
  k_mutex_unlock(&app_config_mutex);
  return kp;
}

bool app_set_kp(float kp)
{
  if (!isfinite(kp) || kp <= 0.0f ||
      kp > static_cast<float>(INT16_MAX) / 256.0f) {
    return false;
  }

  k_mutex_lock(&app_config_mutex, K_FOREVER);
  if (ctrl == &PIctrl) {
    if ((256.0 * kp / Ti_inp) > INT16_MAX) {
      k_mutex_unlock(&app_config_mutex);
      return false;
    }
    Kp_inp_PI = kp;
    PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
  } else {
    Kp_inp_P = kp;
    Pctrl = P_controller(Kp_inp_P);
  }
  k_mutex_unlock(&app_config_mutex);
  return true;
}

float app_get_ti()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const float ti = static_cast<float>(Ti_inp);
  k_mutex_unlock(&app_config_mutex);
  return ti;
}

bool app_set_ti(float ti)
{
  if (!isfinite(ti) || ti <= 0.0f) {
    return false;
  }

  k_mutex_lock(&app_config_mutex, K_FOREVER);
  if ((256.0 * Kp_inp_PI / ti) > INT16_MAX) {
    k_mutex_unlock(&app_config_mutex);
    return false;
  }
  Ti_inp = ti;
  PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
  k_mutex_unlock(&app_config_mutex);
  return true;
}

uint32_t app_get_accel_limit()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const uint32_t limit = profile_accel;
  k_mutex_unlock(&app_config_mutex);
  return limit;
}

bool app_set_accel_limit(uint32_t accel_limit)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  profile_accel = accel_limit;
  k_mutex_unlock(&app_config_mutex);
  return true;
}

uint32_t app_get_dec_limit()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const uint32_t limit = profile_decel;
  k_mutex_unlock(&app_config_mutex);
  return limit;
}

bool app_set_dec_limit(uint32_t decel_limit)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  profile_decel = decel_limit;
  k_mutex_unlock(&app_config_mutex);
  return true;
}

int16_t app_controller_update(int16_t reference, int16_t actual)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  const int16_t output = ctrl->update(reference, actual);
  k_mutex_unlock(&app_config_mutex);
  return output;
}
