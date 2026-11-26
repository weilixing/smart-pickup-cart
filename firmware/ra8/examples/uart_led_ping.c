/*
 * 粘贴进 call_flat 的 FreeRTOS 任务（可替换 new_thread0_entry）
 *
 * 依赖：逐飞库 UART 例程 E1_07_ttl_uart_demo 同一套 API。
 * 若函数名不一致，打开该例程对照改下面 3 处：uart_init / 读一字节 / 写缓冲。
 *
 * 接线（交叉、共地、3.3V TTL）：
 *   香橙派 USB-TTL TX  →  RA8 RX  P706
 *   香橙派 USB-TTL RX  →  RA8 TX  P707
 *   GND               →  GND
 *
 * 行为：收到合法协议帧（含 HEARTBEAT 0x03）→ LED3 闪一下，并回一帧 ACK(0x90)
 */

#include "new_thread0.h"   /* 按你工程实际头文件改 */
#include "bsp_api.h"
/* #include "zf_common_headfile.h"  // 逐飞总头，按例程来 */

#include <stdint.h>
#include <string.h>

#define PROTO_H0          0xAAu
#define PROTO_H1          0x55u
#define PROTO_CMD_HB      0x03u
#define PROTO_CMD_ACK     0x90u

#define UART_INDEX        (UART_1)
#define UART_BAUD         (115200u)

#define LED3              (BSP_IO_PORT_01_PIN_10)

static uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t c = 0;
    while (len--) {
        c ^= *data++;
    }
    return c;
}

static void led_pulse(void)
{
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_LOW);
    vTaskDelay(pdMS_TO_TICKS(80));
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_HIGH);
}

static void send_ack(uint8_t got_cmd)
{
    uint8_t frame[6];
    frame[0] = PROTO_H0;
    frame[1] = PROTO_H1;
    frame[2] = PROTO_CMD_ACK;
    frame[3] = 1;
    frame[4] = got_cmd;
    frame[5] = crc8(&frame[2], 3);
    /* 逐飞：按例程改成你的发送函数 */
    uart_write_buffer(UART_INDEX, frame, sizeof(frame));
}

static void handle_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;
    led_pulse();
    send_ack(cmd);
}

/* 增量解析：与仓库 protocol 一致 */
static void feed_byte(uint8_t b)
{
    static uint8_t buf[64];
    static uint16_t n;

    if (n == 0 && b != PROTO_H0) {
        return;
    }
    if (n == 1 && b != PROTO_H1) {
        n = (b == PROTO_H0) ? 1u : 0u;
        if (n) {
            buf[0] = PROTO_H0;
        }
        return;
    }
    if (n < sizeof(buf)) {
        buf[n++] = b;
    } else {
        n = 0;
        return;
    }

    if (n >= 4) {
        uint8_t len = buf[3];
        uint16_t total = (uint16_t)(4u + len + 1u);
        if (n < total) {
            return;
        }
        if (crc8(&buf[2], (uint16_t)(2u + len)) == buf[total - 1u]) {
            handle_frame(buf[2], &buf[4], len);
        }
        n = 0;
    }
}

void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(LED3, BSP_IO_LEVEL_HIGH); /* 先灭 */

    /* 逐飞初始化：引脚在库内已绑 UART1_TX_P707 / UART1_RX_P706 */
    uart_init(UART_INDEX, UART_BAUD);

    /* 上电快闪 3 次表示任务起来了 */
    for (int i = 0; i < 3; i++) {
        led_pulse();
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    while (1) {
        uint8_t b;
        /* 逐飞：有字节可读时取一字节；函数名以 E1_07 例程为准 */
        if (uart_query_byte(UART_INDEX, &b)) {
            feed_byte(b);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}
