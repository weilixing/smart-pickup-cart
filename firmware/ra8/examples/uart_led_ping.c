/*
 * 双板串口连通测试（RA8 下位机，e2 studio + FSP 6.6.0，已跑通）
 *
 * 功能：收到 OrangePi 心跳帧 AA 55 03 00 03 → 回 ACK AA 55 90 01 03 92，并闪 LED3 一下。
 * 对应上位机测试脚本：scripts/uart_ping_test.py
 *
 * ── FSP 配置要点（照抄即可，都是踩过的坑）──────────────────────────────
 * 1. 器件 RA8P1（R7JA8P1KRLSAJ），工程类型 Flat（非 TrustZone）。
 * 2. Stacks → 添加 "UART (r_sci_b_uart)"，注意是 r_sci_b_uart（不是 r_sci_uart）：
 *      - Channel = 1（SCI1）
 *      - Baud 115200, 8N1
 *      - Callback = uart0_callback
 * 3. Pins：RXD1 = P706，TXD1 = P707（与香橙派交叉、共地、3.3V TTL）。
 * 4. Clocks：必须把 SCICLK 时钟源从 Disabled 改成 HOCO（48MHz），否则 Generate 报错
 *      "Requires SCISPICLK/SCICLK to be enabled"。
 *      坑：自定义板（Custom User Board）下，SCICLK 在 Clocks 界面可能是锁死的；
 *      可手动改工程根目录的 .secure_xml（不是 configuration.xml）：
 *          <node id="board.clock.sciclk.source" option="board.clock.sciclk.source.hoco"/>
 *      然后 Generate 前先完全重启 e2 studio 让它重读。
 * 5. 回调函数 uart0_callback 需要被 FSP 生成代码引用，声明在 hal_data.h（Generate 后自动有）。
 *
 * 依赖：本文件是 FreeRTOS 线程入口，直接替换 call_flat 工程的 new_thread0_entry.c。
 */

#include "new_thread0.h"   /* 包含 bsp_api.h / FreeRTOS / hal_data.h */

#define PROTO_H0   0xAAu
#define PROTO_H1   0x55u
#define CMD_HB     0x03u   /* 心跳 */
#define CMD_ACK    0x90u   /* 应答 */

#define LED3  BSP_IO_PORT_01_PIN_10   /* 逐飞 RA8P1K 板载 LED3，低电平点亮 */

#define RX_BUF_SIZE  128u
static uint8_t           s_rx_buf[RX_BUF_SIZE];
static volatile uint16_t s_rx_wr;   /* 写索引（UART 中断回调写） */
static uint16_t          s_rx_rd;   /* 读索引（任务读） */

/* UART 接收回调（中断上下文，非 static 供 FSP 配置引用） */
void uart0_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR) {
        s_rx_buf[s_rx_wr % RX_BUF_SIZE] = (uint8_t)p_args->data;
        s_rx_wr++;

        uint8_t ch;
        R_SCI_B_UART_Read(&g_uart0_ctrl, &ch, 1u);   /* 重新武装下一次单字节接收 */
    }
}

static uint8_t crc8(const uint8_t *d, uint16_t len)
{
    uint8_t c = 0;
    while (len--) { c ^= *d++; }
    return c;
}

static void send_ack(uint8_t got_cmd)
{
    uint8_t f[6] = {PROTO_H0, PROTO_H1, CMD_ACK, 1u, got_cmd, 0};
    f[5] = crc8(&f[2], 3u);   /* CRC 覆盖 [CMD, LEN, PAYLOAD] */
    R_SCI_B_UART_Write(&g_uart0_ctrl, f, sizeof(f));
}

static void led_pulse(void)
{
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_LOW);
    vTaskDelay(pdMS_TO_TICKS(80));
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_HIGH);
}

/* 增量帧解析状态机，返回 1 表示解析出完整合法帧 */
static int parse_frame(uint8_t b, uint8_t *out_cmd)
{
    static uint8_t st = 0, cmd = 0, len = 0, idx = 0, crc = 0;

    switch (st) {
    case 0:   /* 等 0xAA */
        if (b == PROTO_H0) { st = 1; }
        break;
    case 1:   /* 等 0x55 */
        if (b == PROTO_H1) { st = 2; }
        else { st = (b == PROTO_H0) ? 1u : 0u; }
        break;
    case 2:   /* CMD */
        cmd = b; crc = b; st = 3;
        break;
    case 3:   /* LEN */
        len = b; crc ^= b; idx = 0;
        st = (len == 0u) ? 5u : 4u;
        break;
    case 4:   /* PAYLOAD */
        crc ^= b; idx++;
        if (idx >= len) { st = 5; }
        break;
    case 5:   /* CRC */
        st = 0;
        if (b == crc) { *out_cmd = cmd; return 1; }
        break;
    }
    return 0;
}

void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    R_BSP_PinAccessEnable();

    R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    {
        uint8_t ch;
        R_SCI_B_UART_Read(&g_uart0_ctrl, &ch, 1u);   /* 启动第一次单字节接收 */
    }

    while (1) {
        uint8_t cmd = 0;
        while (s_rx_rd != s_rx_wr) {
            uint8_t b = s_rx_buf[s_rx_rd % RX_BUF_SIZE];
            s_rx_rd++;
            if (parse_frame(b, &cmd) && cmd == CMD_HB) {
                send_ack(CMD_HB);
                led_pulse();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
