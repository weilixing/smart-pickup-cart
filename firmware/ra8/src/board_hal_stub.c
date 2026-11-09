#include "board_hal.h"

/*
 * 仅用于本地编译检查 / 无板仿真。
 * 上板时请改用 board_hal_fsp.c（由本 stub 改写成 FSP 调用），并排除本文件。
 */

static uint32_t g_ms;
static int32_t g_enc_l;
static int32_t g_enc_r;

void board_init(void)
{
    g_ms = 0;
}

uint32_t board_millis(void)
{
    return g_ms++;
}

void board_delay_ms(uint32_t ms)
{
    g_ms += ms;
}

void board_motor_set_duty(int16_t left_duty, int16_t right_duty)
{
    (void)left_duty;
    (void)right_duty;
    /* stub：假编码器随 duty 走动，便于无板冒烟 */
    g_enc_l += left_duty / 100;
    g_enc_r += right_duty / 100;
}

int32_t board_encoder_left(void)
{
    return g_enc_l;
}

int32_t board_encoder_right(void)
{
    return g_enc_r;
}

void board_line_read(uint16_t out[8])
{
    for (int i = 0; i < 8; i++) {
        out[i] = (i == 3 || i == 4) ? 800u : 100u;
    }
}

uint16_t board_ultrasonic_front_mm(void) { return 800; }
uint16_t board_ultrasonic_left_mm(void) { return 800; }
uint16_t board_ultrasonic_right_mm(void) { return 800; }

void board_servo_set_deg(uint8_t joint, float angle_deg)
{
    (void)joint;
    (void)angle_deg;
}

void board_host_uart_write(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
}

int board_host_uart_read(uint8_t *data, uint16_t max_len)
{
    (void)data;
    (void)max_len;
    return 0;
}

void board_voice_uart_write(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
}
