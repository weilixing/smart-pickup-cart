#pragma once

void app_init(void);
void app_loop(void);   /* 主循环里反复调用；内部按时间片跑各模块 */
