#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 在 e2 studio 生成工程后，用 FSP API 实现本文件中的函数（见 src/board_hal_fsp.c.example） */

void     board_init(void);
uint32_t board_millis(void);
void     board_delay_ms(uint32_t ms);

/* 电机：duty ∈ [-1000, 1000]，负值为反转 */
void board_motor_set_duty(int16_t left_duty, int16_t right_duty);

/* 编码器累计脉冲（可正可负） */
int32_t board_encoder_left(void);
int32_t board_encoder_right(void);

/* 灰度：返回 0~1000，越大越“黑”（可按传感器极性在实现里翻转） */
void board_line_read(uint16_t out[8]);

/* 超声波：毫米；失败返回 0xFFFF */
uint16_t board_ultrasonic_front_mm(void);
uint16_t board_ultrasonic_left_mm(void);
uint16_t board_ultrasonic_right_mm(void);

/* 舵机：angle_deg 0~180（或按舵机手册映射到脉宽） */
void board_servo_set_deg(uint8_t joint, float angle_deg);

/* 与 OrangePi 通信的 UART */
void board_host_uart_write(const uint8_t *data, uint16_t len);
int  board_host_uart_read(uint8_t *data, uint16_t max_len); /* 返回读到的字节数 */

/* SYN6288 所在 UART（可与 host 分开） */
void board_voice_uart_write(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
