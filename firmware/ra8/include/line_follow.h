#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

void line_init(void);
void line_update(void);                /* 读传感器、算偏差、输出建议速度 */
void line_get_status(proto_line_t *out);
void line_get_cmd(int16_t *linear_mm_s, int16_t *angular_mrad_s);
bool line_on_line(void);
