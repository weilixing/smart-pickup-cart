#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef struct {
    uint8_t mode;              /* 0停 1遥控 2循迹 3急停 */
    int16_t cmd_linear_mm_s;
    int16_t cmd_angular_mrad_s;
    uint8_t arm_seq_req;       /* 0xFF=无新请求 */
    uint8_t voice_index_req;   /* 0xFF=无 */
    uint32_t last_rx_ms;
} host_cmd_t;

void comm_init(void);
void comm_poll_rx(void);                 /* 主循环或 UART 中断后调用 */
const host_cmd_t *comm_cmd(void);
void comm_clear_arm_req(void);
void comm_clear_voice_req(void);

void comm_send_odom(const proto_odom_t *o);
void comm_send_line(const proto_line_t *l);
void comm_send_obstacle(const proto_obstacle_t *o);
void comm_send_arm_state(const proto_arm_state_t *a);
void comm_send_voice_done(uint8_t index);
