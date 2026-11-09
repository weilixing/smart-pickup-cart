#include "motor.h"
#include "app_config.h"
#include "board_hal.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float target_l;
    float target_r;
    float cmd_l;
    float cmd_r;
    float integ_l;
    float integ_r;
    int32_t last_enc_l;
    int32_t last_enc_r;
    float x_mm;
    float y_mm;
    float theta_rad;
    float v_mm_s;
    float w_rad_s;
    bool estop;
} motor_t;

static motor_t g;

void motor_init(void)
{
    g = (motor_t){0};
    g.last_enc_l = board_encoder_left();
    g.last_enc_r = board_encoder_right();
}

void motor_estop(void)
{
    g.estop = true;
    g.target_l = g.target_r = 0;
    g.cmd_l = g.cmd_r = 0;
    g.integ_l = g.integ_r = 0;
    board_motor_set_duty(0, 0);
}

void motor_clear_estop(void)
{
    g.estop = false;
}

void motor_set_target(int16_t linear_mm_s, int16_t angular_mrad_s)
{
    if (g.estop) {
        return;
    }
    if (linear_mm_s > MAX_LINEAR_MM_S) linear_mm_s = MAX_LINEAR_MM_S;
    if (linear_mm_s < -MAX_LINEAR_MM_S) linear_mm_s = (int16_t)-MAX_LINEAR_MM_S;
    if (angular_mrad_s > MAX_ANGULAR_MRAD_S) angular_mrad_s = MAX_ANGULAR_MRAD_S;
    if (angular_mrad_s < -MAX_ANGULAR_MRAD_S) angular_mrad_s = (int16_t)-MAX_ANGULAR_MRAD_S;

    float v = (float)linear_mm_s;
    float w = (float)angular_mrad_s / 1000.0f; /* rad/s */
    float half = WHEEL_TRACK_MM * 0.5f;
    g.target_l = v - w * half;
    g.target_r = v + w * half;
}

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static float ramp_toward(float cur, float tgt, float max_delta)
{
    float d = tgt - cur;
    if (d > max_delta) d = max_delta;
    if (d < -max_delta) d = -max_delta;
    return cur + d;
}

void motor_update_1khz(void)
{
    const float dt = 1.0f / (float)MOTOR_CTRL_HZ;
    int32_t el = board_encoder_left();
    int32_t er = board_encoder_right();
    int32_t dl = el - g.last_enc_l;
    int32_t dr = er - g.last_enc_r;
    g.last_enc_l = el;
    g.last_enc_r = er;

    float dist_l = ((float)dl / TICKS_PER_METER) * 1000.0f; /* mm */
    float dist_r = ((float)dr / TICKS_PER_METER) * 1000.0f;
    float dist = 0.5f * (dist_l + dist_r);
    float dtheta = (dist_r - dist_l) / WHEEL_TRACK_MM;

    g.theta_rad += dtheta;
    g.x_mm += dist * cosf(g.theta_rad);
    g.y_mm += dist * sinf(g.theta_rad);
    g.v_mm_s = dist / dt;
    g.w_rad_s = dtheta / dt;

    if (g.estop) {
        board_motor_set_duty(0, 0);
        return;
    }

    float max_dv = ACCEL_MM_S2 * dt;
    g.cmd_l = ramp_toward(g.cmd_l, g.target_l, max_dv);
    g.cmd_r = ramp_toward(g.cmd_r, g.target_r, max_dv);

    float meas_l = dist_l / dt;
    float meas_r = dist_r / dt;
    const float kp = 0.8f;
    const float ki = 0.15f;

    float err_l = g.cmd_l - meas_l;
    float err_r = g.cmd_r - meas_r;
    g.integ_l = clampf(g.integ_l + err_l * dt, -500.0f, 500.0f);
    g.integ_r = clampf(g.integ_r + err_r * dt, -500.0f, 500.0f);

    float duty_l = kp * err_l + ki * g.integ_l;
    float duty_r = kp * err_r + ki * g.integ_r;
    /* 开环偏置：目标速度映射到占空比，闭环修正 */
    duty_l += g.cmd_l * 1.5f;
    duty_r += g.cmd_r * 1.5f;
    duty_l = clampf(duty_l, -1000.0f, 1000.0f);
    duty_r = clampf(duty_r, -1000.0f, 1000.0f);
    board_motor_set_duty((int16_t)duty_l, (int16_t)duty_r);
}

void motor_get_odom(proto_odom_t *out)
{
    out->x_mm = (int32_t)g.x_mm;
    out->y_mm = (int32_t)g.y_mm;
    out->theta_mrad = (int32_t)(g.theta_rad * 1000.0f);
    out->v_mm_s = (int16_t)g.v_mm_s;
    out->w_mrad_s = (int16_t)(g.w_rad_s * 1000.0f);
}
