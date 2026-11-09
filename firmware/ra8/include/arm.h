#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

void arm_init(void);
void arm_request_seq(uint8_t seq_id);  /* 0回零 1抓取 2投放 */
void arm_set_joint_cdeg(uint8_t joint, uint16_t angle_cdeg);
void arm_update(void);                 /* 非阻塞推进序列 */
void arm_get_state(proto_arm_state_t *out);
bool arm_busy(void);
