/*
 * 双板串口连通测试（RA8 下位机，e2 studio + FSP 6.6.0）
 *
 * 功能：收到 OrangePi 心跳帧 AA 55 03 00 03 → 回 ACK AA 55 90 01 03 92，并闪 LED3。
 * 上位机：scripts/uart_ping_test.py /dev/ttyAMA2
 *
 * ── FSP 配置要点 ──────────────────────────────────────────────────────────
 * 1. 器件 RA8P1（R7JA8P1KRLSAJ），Flat（非 TrustZone）。
 * 2. Stacks → "UART (r_sci_b_uart)"（不是 r_sci_uart）：
 *      - Channel = 1（SCI1）
 *      - Baud 115200, 8N1
 *      - Callback = uart0_callback
 *      - 实例名若是 g_uart0，下面保持 g_uart0_*；若 Generate 成 g_uart1，全文替换。
 * 3. Pins：RXD1 = P706，TXD1 = P707。
 * 4. Clocks：SCICLK 必须 HOCO（勿 Disabled）。
 * 5. 香橙派侧用 UART7：Pin7=TX → P706，Pin29=RX ← P707，GND 共地；设备 /dev/ttyAMA2。
 *    （UART2 / ttyAMA1 在该板上环回不通，不要用。）
 *
 * 本文件替换 call_flat 的 new_thread0_entry.c 后：Build → Debug → Resume(F8)。
 * 上电应先快闪 3 下；收心跳再闪并回 ACK。
 */

#include "new_thread0.h"

#define PROTO_H0   0xAAu
#define PROTO_H1   0x55u
#define CMD_HB     0x03u
#define CMD_ACK    0x90u

#define LED3  BSP_IO_PORT_01_PIN_10

#define RX_BUF_SIZE  128u
static uint8_t           s_rx_buf[RX_BUF_SIZE];
static volatile uint16_t s_rx_wr;
static uint16_t          s_rx_rd;

/* Read() 的目标缓冲必须是静态的，不能用栈上临时变量 */
static uint8_t s_rx_byte;

void uart0_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR) {
        s_rx_buf[s_rx_wr % RX_BUF_SIZE] = (uint8_t)p_args->data;
        s_rx_wr++;
    }
    else if (p_args->event == UART_EVENT_RX_COMPLETE) {
        s_rx_buf[s_rx_wr % RX_BUF_SIZE] = s_rx_byte;
        s_rx_wr++;
        (void)R_SCI_B_UART_Read(&g_uart0_ctrl, &s_rx_byte, 1u);
    }
}

static uint8_t crc8(const uint8_t *d, uint16_t len)
{
    uint8_t c = 0;
    while (len--) {
        c ^= *d++;
    }
    return c;
}

static void send_ack(uint8_t got_cmd)
{
    static uint8_t f[6];
    f[0] = PROTO_H0;
    f[1] = PROTO_H1;
    f[2] = CMD_ACK;
    f[3] = 1u;
    f[4] = got_cmd;
    f[5] = crc8(&f[2], 3u);
    (void)R_SCI_B_UART_Write(&g_uart0_ctrl, f, sizeof(f));
}

static void led_pulse(void)
{
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_LOW);
    vTaskDelay(pdMS_TO_TICKS(80));
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_HIGH);
}

static int parse_frame(uint8_t b, uint8_t *out_cmd)
{
    static uint8_t st = 0, cmd = 0, len = 0, idx = 0, crc = 0;

    switch (st) {
    case 0:
        if (b == PROTO_H0) {
            st = 1;
        }
        break;
    case 1:
        if (b == PROTO_H1) {
            st = 2;
        }
        else {
            st = (b == PROTO_H0) ? 1u : 0u;
        }
        break;
    case 2:
        cmd = b;
        crc = b;
        st = 3;
        break;
    case 3:
        len = b;
        crc ^= b;
        idx = 0;
        st = (len == 0u) ? 5u : 4u;
        break;
    case 4:
        crc ^= b;
        idx++;
        if (idx >= len) {
            st = 5;
        }
        break;
    case 5:
        st = 0;
        if (b == crc) {
            *out_cmd = cmd;
            return 1;
        }
        break;
    }
    return 0;
}

void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_HIGH);

    (void)R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    (void)R_SCI_B_UART_Read(&g_uart0_ctrl, &s_rx_byte, 1u);

    /* 上电快闪 3 次：确认已烧到本联调固件（不是旧慢闪程序） */
    for (int i = 0; i < 3; i++) {
        led_pulse();
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    while (1) {
        uint8_t cmd = 0;
        while (s_rx_rd != s_rx_wr) {
            uint8_t b = s_rx_buf[s_rx_rd % RX_BUF_SIZE];
            s_rx_rd++;
            if (parse_frame(b, &cmd)) {
                /* 任意合法帧都闪灯；心跳再回 ACK */
                led_pulse();
                if (cmd == CMD_HB) {
                    send_ack(CMD_HB);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
