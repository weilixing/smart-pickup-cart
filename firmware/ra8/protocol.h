/*
 * OrangePi <-> RA8 串口协议（与 ros2_ws/.../protocol.py 对应）
 * 文档：docs/serial_protocol.md
 *
 * 帧：[0xAA, 0x55, CMD, LEN, PAYLOAD..., CRC]
 * CRC = 从 CMD 到 PAYLOAD 末尾逐字节 XOR；多字节整数小端。
 */
#pragma once

#include <stdint.h>

#define PROTO_HEADER0 0xAAu
#define PROTO_HEADER1 0x55u

/* 下行 */
#define PROTO_CMD_SET_MODE  0x01u /* uint8: 0停 1遥控 2循迹任务 3急停 */
#define PROTO_CMD_SET_VEL   0x02u /* int16 linear_mm_s + int16 angular_mrad_s */
#define PROTO_CMD_HEARTBEAT 0x03u
#define PROTO_CMD_ARM_SEQ   0x10u /* uint8 seq_id: 0回零 1抓取 2投放 */
#define PROTO_CMD_ARM_JOINT 0x11u /* uint8 joint + uint16 angle_cdeg */
#define PROTO_CMD_VOICE     0x20u /* uint8 index 或 UTF-8 文本 */

/* 上行 */
#define PROTO_CMD_ODOM       0x81u
#define PROTO_CMD_LINE       0x82u
#define PROTO_CMD_OBSTACLE   0x83u
#define PROTO_CMD_ARM_STATE  0x84u
#define PROTO_CMD_VOICE_DONE 0x85u
#define PROTO_CMD_IMU        0x86u

typedef struct __attribute__((packed)) {
    int32_t x_mm;
    int32_t y_mm;
    int32_t theta_mrad;
    int16_t v_mm_s;
    int16_t w_mrad_s;
} proto_odom_t;

typedef struct __attribute__((packed)) {
    int16_t error_mm;
    uint8_t on_line;
    uint8_t station;
} proto_line_t;

typedef struct __attribute__((packed)) {
    uint16_t front_mm;
    uint16_t left_mm;
    uint16_t right_mm;
    uint8_t state; /* 0正常 1减速 2绕行 */
} proto_obstacle_t;

typedef struct __attribute__((packed)) {
    uint8_t busy;
    uint8_t last_seq;
    uint8_t ok;
} proto_arm_state_t;

static inline uint8_t proto_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
    }
    return crc;
}

static inline uint16_t proto_pack(uint8_t cmd, const uint8_t *payload, uint8_t len, uint8_t *out)
{
    out[0] = PROTO_HEADER0;
    out[1] = PROTO_HEADER1;
    out[2] = cmd;
    out[3] = len;
    for (uint8_t i = 0; i < len; i++) {
        out[4 + i] = payload[i];
    }
    out[4 + len] = proto_crc8(&out[2], (uint16_t)(len + 2));
    return (uint16_t)(len + 5);
}
