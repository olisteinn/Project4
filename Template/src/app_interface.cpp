#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_interface.h"
#include "P_controller.h"
#include "PI_controller.h"
#include "control_task.h"
#include "controller.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

extern controller *ctrl;
extern P_controller Pctrl;
extern PI_controller PIctrl;
extern double Kp_inp_P;
extern double Kp_inp_PI;
extern double Ti_inp;

static const int32_t max_rpm = 100;
K_MUTEX_DEFINE(app_config_mutex);

static uint32_t profile_accel = 0;
static uint32_t profile_decel = 0;

void app_set_target_speed(int32_t rpm)
{
  if (rpm > max_rpm) {
    rpm = max_rpm;
  } else if (rpm < -max_rpm) {
    rpm = -max_rpm;
  }
  control_set_target_speed((int16_t)(rpm * 100));
  LOG_INF("Target speed set to %d RPM", (int)rpm);
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
  int32_t ctrlOP = control_get_telemetry().output;
  return (int16_t)(ctrlOP * 1000 / 32640); // ±32640 -> ±1000 (0.1 %)
}

uint8_t app_get_controller_type()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  uint8_t type = (ctrl == &PIctrl) ? 1 : 0;
  k_mutex_unlock(&app_config_mutex);
  return type;
}

void app_set_controller_type(uint8_t type)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  ctrl = (type == 0) ? (controller *)&Pctrl : (controller *)&PIctrl;
  k_mutex_unlock(&app_config_mutex);
  LOG_INF("Controller set to %s", (type == 0) ? "P" : "PI");
}

float app_get_kp()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  float kp = (ctrl == &PIctrl) ? (float)Kp_inp_PI : (float)Kp_inp_P;
  k_mutex_unlock(&app_config_mutex);
  return kp;
}

void app_set_kp(float kp)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  if (ctrl == &PIctrl) {
    Kp_inp_PI = kp;
    PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
  } else {
    Kp_inp_P = kp;
    Pctrl = P_controller(Kp_inp_P);
  }
  k_mutex_unlock(&app_config_mutex);
  LOG_INF("Kp set to %.4f", (double)kp);
}

float app_get_ti()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  float ti = (float)Ti_inp;
  k_mutex_unlock(&app_config_mutex);
  return ti;
}

void app_set_ti(float ti)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  Ti_inp = ti;
  PIctrl = PI_controller(Kp_inp_PI, Ti_inp);
  k_mutex_unlock(&app_config_mutex);
  LOG_INF("Ti set to %.4f", (double)ti);
}

uint32_t app_get_accel_limit()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  uint32_t limit = profile_accel;
  k_mutex_unlock(&app_config_mutex);
  return limit;
}

void app_set_accel_limit(uint32_t accel_limit)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  profile_accel = accel_limit;
  k_mutex_unlock(&app_config_mutex);
  LOG_INF("Accel limit set to %u RPM/s", accel_limit);
}

uint32_t app_get_dec_limit()
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  uint32_t limit = profile_decel;
  k_mutex_unlock(&app_config_mutex);
  return limit;
}

void app_set_dec_limit(uint32_t decel_limit)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  profile_decel = decel_limit;
  k_mutex_unlock(&app_config_mutex);
  LOG_INF("Decel limit set to %u RPM/s", decel_limit);
}

int16_t app_controller_update(int16_t reference, int16_t actual)
{
  k_mutex_lock(&app_config_mutex, K_FOREVER);
  int16_t output = ctrl->update(reference, actual);
  k_mutex_unlock(&app_config_mutex);
  return output;
}
