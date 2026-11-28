/*
 * board_hal_fsp.c —— RA8P1 的 FSP HAL 实现（逐飞 RA8P1K 板，FSP 6.6.0）
 *
 * 实现 board_hal.h 的全部接口。当前进度：
 *   ✅ Host UART（接 OrangePi，115200）已跑通
 *   ⬜ Voice UART（SYN6288，9600）待接独立 SCI
 *   ⬜ 电机 PWM / 编码器 / 灰度 ADC / 超声波 / 舵机 PWM 待实现
 *
 * ── FSP 资源映射 ──────────────────────────────────────────────────────────
 * Host UART ：SCI1，TXD1=P707，RXD1=P706，115200 8N1，实例 g_uart0，回调 uart0_callback
 * Voice UART：待定（建议独立 SCI，勿占用 SCI1）
 *
 * ── 关键坑（详见 examples/uart_led_ping.c 注释）──────────────────────────
 * 1. 驱动是 r_sci_b_uart（R_SCI_B_UART_*），不是 r_sci_uart。
 * 2. SCICLK 必须 HOCO；Custom User Board 下 Clocks 界面锁死，手动改 ra_gen/bsp_clock_cfg.h。
 * 3. 波特率静态配置是 100% 误差（Generate 时 SCICLK=Disabled 生成的），
 *    必须在运行时用 R_SCI_B_UART_BaudCalculate 重算（见 board_init）。
 *
 * 用法：把本文件加入工程（改名 board_hal.c 或保持 board_hal_fsp.c 皆可），
 *       同时不要编译 board_hal_stub.c。
 */

#include "board_hal.h"
#include "app_config.h"
#include "hal_data.h"

/* ===================== Host UART（OrangePi ↔ RA8） ===================== */

#define HOST_RX_BUF_SIZE 256u
static uint8_t           s_host_rx_buf[HOST_RX_BUF_SIZE];
static volatile uint16_t s_host_rx_wr;   /* 写索引（中断回调写） */
static uint16_t          s_host_rx_rd;   /* 读索引（board_host_uart_read 读） */
static uint8_t           s_host_rx_byte; /* Read() 的目标缓冲必须是静态的，不能用栈上临时变量 */

/* Host UART 接收回调（中断上下文；非 static，名字须与 Smart Configurator 里 Callback 一致） */
void uart0_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR) {
        s_host_rx_buf[s_host_rx_wr % HOST_RX_BUF_SIZE] = (uint8_t)p_args->data;
        s_host_rx_wr++;
    }
    else if (p_args->event == UART_EVENT_RX_COMPLETE) {
        s_host_rx_buf[s_host_rx_wr % HOST_RX_BUF_SIZE] = s_host_rx_byte;
        s_host_rx_wr++;
        (void)R_SCI_B_UART_Read(&g_uart0_ctrl, &s_host_rx_byte, 1u);
    }
}

/* ===================== 初始化 ===================== */

void board_init(void)
{
    /* ---- Host UART ---- */
    (void)R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);

    /* 波特率重算并应用：静态配置是 SCICLK=Disabled 时生成的（100% 误差），必须重算 */
    {
        sci_b_baud_setting_t baud;
        (void)R_SCI_B_UART_BaudCalculate(HOST_UART_BAUD, false, 1500, &baud);
        (void)R_SCI_B_UART_BaudSet(&g_uart0_ctrl, &baud);
    }

    (void)R_SCI_B_UART_Read(&g_uart0_ctrl, &s_host_rx_byte, 1u);   /* 启动第一次单字节接收 */

    /* TODO: 电机 PWM / 编码器 / 灰度 ADC / 超声波 / 舵机 PWM / Voice UART 的初始化 */
}

/* ===================== Host UART ===================== */

void board_host_uart_write(const uint8_t *data, uint16_t len)
{
    (void)R_SCI_B_UART_Write(&g_uart0_ctrl, data, len);
}

int board_host_uart_read(uint8_t *data, uint16_t max_len)
{
    uint16_t n = 0;
    while ((s_host_rx_rd != s_host_rx_wr) && (n < max_len)) {
        data[n++] = s_host_rx_buf[s_host_rx_rd % HOST_RX_BUF_SIZE];
        s_host_rx_rd++;
    }
    return (int)n;
}

/* ===================== Voice UART（SYN6288） ===================== */

void board_voice_uart_write(const uint8_t *data, uint16_t len)
{
    /* TODO: SYN6288 建议独立 SCI（9600 8N1），配好后改成对应的 g_uartX_ctrl。
     * 例如：R_SCI_B_UART_Write(&g_uart1_ctrl, data, len); */
    (void)data;
    (void)len;
}

/* ===================== 其余 HAL（待实现） ===================== */

uint32_t board_millis(void)
{
    /* TODO: FreeRTOS 下可用 xTaskGetTickCount()（需 1ms tick）；
     *       裸机用 SysTick 计数。返回毫秒。 */
    return 0;
}

void board_delay_ms(uint32_t ms)
{
    /* TODO: R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS); */
    (void)ms;
}

void board_motor_set_duty(int16_t left_duty, int16_t right_duty)
{
    /* TODO: duty ∈ [-1000,1000]，映射到电机 PWM 占空比与 DIR 引脚 */
    (void)left_duty;
    (void)right_duty;
}

int32_t board_encoder_left(void)
{
    /* TODO: 左编码器累计脉冲 */
    return 0;
}

int32_t board_encoder_right(void)
{
    /* TODO: 右编码器累计脉冲 */
    return 0;
}

void board_line_read(uint16_t out[8])
{
    /* TODO: 8 路灰度 ADC，返回 0~1000 */
    for (uint8_t i = 0; i < 8u; i++) {
        out[i] = 0;
    }
}

uint16_t board_ultrasonic_front_mm(void) { return 0xFFFF; } /* TODO */
uint16_t board_ultrasonic_left_mm(void)  { return 0xFFFF; } /* TODO */
uint16_t board_ultrasonic_right_mm(void) { return 0xFFFF; } /* TODO */

void board_servo_set_deg(uint8_t joint, float angle_deg)
{
    /* TODO: 4 路舵机，0~180° 映射到 PWM 脉宽 */
    (void)joint;
    (void)angle_deg;
}
