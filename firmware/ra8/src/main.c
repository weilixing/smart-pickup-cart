#include "app.h"

/*
 * e2 studio 工程里：
 * 1) 用 FSP 配置时钟、GPT/PWM、ADC、UART、ICU 等
 * 2) 实现 board_hal.c（可参考 board_hal_fsp.c.example）
 * 3) 把本目录 src 下的 .c 与 include/ 加入工程
 * 4) 将启动文件里的主循环改为调用 app_init + app_loop
 *
 * 若工程已有 hal_entry()，也可删掉本文件，在 hal_entry 中调用 app_init/app_loop。
 */

int main(void)
{
    app_init();
    for (;;) {
        app_loop();
    }
    return 0;
}
