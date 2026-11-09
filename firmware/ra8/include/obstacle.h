#pragma once

#include <stdint.h>
#include "protocol.h"

typedef enum {
    OBS_OK = 0,
    OBS_SLOW = 1,
    OBS_AVOID = 2,
    OBS_STOP = 3
} obs_action_t;

void obstacle_init(void);
void obstacle_update(void);
void obstacle_get_status(proto_obstacle_t *out);
obs_action_t obstacle_action(void);

/* 对循迹输出做限速 / 叠加绕行偏转 */
void obstacle_apply(int16_t *linear_mm_s, int16_t *angular_mrad_s);
