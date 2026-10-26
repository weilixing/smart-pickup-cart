/*
 * OrangePi <-> RA8 串口协议（与 ros2_ws/src/motor_bridge/motor_bridge/protocol.py 对应）
 * 协议文档：docs/serial_protocol.md
 *
 * 帧格式：[0xAA, 0x55, CMD, LEN, PAYLOAD..., CRC]
 * CRC = 从 CMD 到 PAYLOAD 末尾逐字节 XOR；多字节整数小端。
 */
#pragma once

#include <stdint.h>

#define PROTO_HEADER0 0xAAu
#define PROTO_HEADER1 0x55u

/* 下行（OrangePi -> RA8） */
#define PROTO_CMD_SET_VEL   0x01u /* int16 linear_mm_s + int16 angular_mrad_s */
#define PROTO_CMD_LIFT_CMD  0x02u /* uint8: 0=降, 1=升 */
#define PROTO_CMD_HEARTBEAT 0x03u /* 空；500ms 无任何下行帧则急停 */

/* 上行（RA8 -> OrangePi） */
#define PROTO_CMD_ODOM       0x81u /* proto_odom_t，50Hz */
#define PROTO_CMD_LIFT_STATE 0x82u /* uint8: 0=已降下, 1=已升起, 2=运动中 */
#define PROTO_CMD_RFID       0x84u /* 12B ASCII UID，不足补 0 */
#define PROTO_CMD_IMU        0x85u /* proto_imu_t，100Hz */

typedef struct __attribute__((packed)) {
    int32_t x_mm;
    int32_t y_mm;
    int32_t theta_mrad;
    int16_t v_mm_s;
    int16_t w_mrad_s;
} proto_odom_t;

typedef struct __attribute__((packed)) {
    int16_t ax_mg;    /* 加速度，单位 mg */
    int16_t ay_mg;
    int16_t az_mg;
    int16_t gz_cdps;  /* Z 轴角速度，单位 0.01 dps */
} proto_imu_t;

static inline uint8_t proto_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
    }
    return crc;
}

/* 打包一帧到 out（out 至少 len + 5 字节），返回帧总长度 */
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
