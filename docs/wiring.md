# 接线表

## 电源

| 从 | 到 | 说明 |
| --- | --- | --- |
| 电池组（按电机额定电压） | 电机驱动 VM | 电机动力电 |
| 电池 → 稳压 | OrangePi AIpro | 按官方规格给足电流，独立一路 |
| 稳压 5V | RA8 开发板 / 舵机电源（大电流舵机建议单独 BEC） | 逻辑与舵机供电隔离时注意共地 |
| — | — | **所有地必须共地** |

详见 `docs/power_design.md`。

## OrangePi AIpro ↔ 逐飞 RA8P1（用户 UART1）

| OrangePi 侧 | RA8P1 侧 | 说明 |
| --- | --- | --- |
| USB-TTL TX | **P706（UART1 RX）** | 交叉 |
| USB-TTL RX | **P707（UART1 TX）** | 交叉 |
| GND | GND | **必须共地** |
| — | P208/P209 | 调试串口，给 printf，不要接 Pi |

电平 **3.3V TTL**，波特率 **115200 8N1**。协议见 `docs/serial_protocol.md`。  
冒烟：`scripts/uart_ping_test.py`；固件示例：`firmware/ra8/examples/uart_led_ping.c`。

## RA8 ↔ 底盘（后续）

| RA8 | 外设 | 说明 |
| --- | --- | --- |
| GPT PWM ×2 + GPIO 方向 | TB6612 / AT8236 | 左右侧电机 |
| 输入捕获 / 外部中断 | 编码器 A/B | 里程计与速度闭环 |

## RA8 ↔ 执行机构（后续，方案未最终锁定）

| 外设 | 接口 | 说明 |
| --- | --- | --- |
| 取货机构 | PWM / GPIO | 臂或吸盘等，待定 |
| SYN6288 | 建议独立 UART | 若 UART1 已给 Pi，勿占用 |

## OrangePi 其它

| 外设 | 接口 | 说明 |
| --- | --- | --- |
| YDLIDAR X3 | USB | 官方 ROS2 驱动 |
| USB 相机（可选） | USB | 免驱，v4l2 |
