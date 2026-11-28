/*
 * 双板串口连通测试（RA8 下位机，e2 studio + FSP 6.6.0，已跑通）
 *
 * 功能：收到 OrangePi 心跳帧 AA 55 03 00 03 → 回 ACK AA 55 90 01 03 92，并闪 LED3。
 * 上位机：scripts/uart_ping_test.py /dev/ttyAMA2
 * 实测结果：10/10 心跳全部收到 ACK，双板串口打通。
 *
 * ── FSP 配置要点 ──────────────────────────────────────────────────────────
 * 1. 器件 RA8P1（R7JA8P1KRLSAJ），工程类型 Flat（非 TrustZone）。
 * 2. Stacks → 添加 "UART (r_sci_b_uart)"，注意是 r_sci_b_uart（不是 r_sci_uart）：
 *      - Channel = 1（SCI1）
 *      - Baud 115200, 8N1
 *      - Callback = uart0_callback
 * 3. Pins：RXD1 = P706，TXD1 = P707。
 * 4. Clocks：SCICLK 必须选 HOCO（48MHz）。
 * 5. 香橙派侧 UART7：Pin7=TX → P706，Pin29=RX ← P707，GND 共地；设备 /dev/ttyAMA2。
 *    （UART2 / ttyAMA1 在该板上环回不通，不要用。）
 *
 * ── 踩坑记录（为什么之前一直收不到 ACK）────────────────────────────────
 * 【坑1】驱动名是 r_sci_b_uart，不是 r_sci_uart。
 *        API 是 R_SCI_B_UART_Open/Read/Write，写成 R_SCI_UART_* 会链接报错。
 *
 * 【坑2】SCICLK 时钟源默认是 Disabled，UART 没时钟。
 *        Generate 会报 "Requires SCISPICLK/SCICLK to be enabled"。
 *        且 Custom User Board 下 Clocks 界面里 SCICLK 是锁死的、点不动；
 *        手动改 .secure_xml 成 hoco 也不会被 Generate 采用（Generate 仍按设备默认 Disabled 生成）。
 *        临时办法：直接改生成的 ra_gen/bsp_clock_cfg.h：
 *            #define BSP_CFG_SCICLK_SOURCE (BSP_CLOCKS_SOURCE_CLOCK_HOCO)
 *        然后强制重编 bsp_clocks.c。注意：再点 Generate/Clean 会还原，要重改。
 *
 * 【坑3】关键：波特率参数是 100% 误差。
 *        因为 Generate 时 SCICLK=Disabled（0Hz），FSP 生成的 g_uart0_baud_setting 是错的
 *        （hal_data.c 里有注释 "Baud rate calculated with 100.000% error"）。
 *        只改 SCICLK 不够，还要在运行时用实际 SCICLK 重算波特率，见下方
 *        R_SCI_B_UART_BaudCalculate + R_SCI_B_UART_BaudSet。
 *
 * 【坑4】LED3(P1.10) 实为高电平点亮（拉高亮/拉低灭），本文件 led_pulse 里 LOW/HIGH
 *        按"低电平点亮"写的，方向反了；不影响串口，只是常亮/闪灭表现相反。
 *
 * 本文件替换 call_flat 的 new_thread0_entry.c 后：Build → Debug → Resume(F8)。
 * 上电先快闪 3 下；收心跳再闪并回 ACK。
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

    /* 运行时重算并应用波特率：静态配置是 SCICLK=Disabled 时生成的（100% 误差），必须重算 */
    {
        sci_b_baud_setting_t baud;
        (void)R_SCI_B_UART_BaudCalculate(115200, false, 1500, &baud);
        (void)R_SCI_B_UART_BaudSet(&g_uart0_ctrl, &baud);
    }

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
