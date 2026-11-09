#pragma once

#include <stdint.h>
#include "protocol.h"

void motor_init(void);
void motor_estop(void);
void motor_clear_estop(void);
void motor_set_target(int16_t linear_mm_s, int16_t angular_mrad_s);
void motor_update_1khz(void);          /* 速度环 + 斜坡 */
void motor_get_odom(proto_odom_t *out);
