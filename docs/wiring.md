# 接线表

## 电源

| 从 | 到 | 说明 |
| --- | --- | --- |
| 3S 18650（12.6V） | TB6612 VM | 电机动力电，直取电池 |
| 3S 18650 | 稳压模块 → OrangePi | 按 AIpro 官方规格供足电流，独立一路 |
| 稳压 5V | RA8 开发板 5V | 板载稳出 3.3V |
| — | — | **所有地必须共地**，电机电源与逻辑电源分开走线 |

## RA8 ↔ TB6612（电机驱动）

| RA8 | TB6612 | 说明 |
| --- | --- | --- |
| GPT PWM ×2 | PWMA / PWMB | 左右侧电机调速（同侧两轮并一路） |
| GPIO ×4 | AIN1/AIN2、BIN1/BIN2 | 方向 |
| 3.3V | VCC | 逻辑电平 |
| GND | GND | 共地 |

## RA8 ↔ 编码器电机 ×4

- 每路编码器 A/B 相接 RA8 的 GPT 输入捕获/外部中断引脚
- 编码器供电 3.3V（若电机编码器为 5V 需分压）

## RA8 其他外设

| 外设 | 接口 | 说明 |
| --- | --- | --- |
| MPU6050/ICM20948 | I2C | 随 IMU 帧（0x85）上报给 OrangePi |
| RC522 RFID | SPI | 读到卡即上报 RFID 帧（0x84） |
| NEMA17 驱动（A4988/DRV8825） | STEP/DIR GPIO | 顶升丝杆，两端加限位开关 |
| 限位开关 ×2 | GPIO（带上拉） | 顶升上/下限位 |

## OrangePi AIpro

| 外设 | 接口 | 说明 |
| --- | --- | --- |
| RA8 | UART（USB-TTL 或板间 3.3V UART） | 跑串口协议，见 `docs/serial_protocol.md` |
| YDLIDAR | USB | 官方驱动包 ydlidar_ros2_driver |
| USB 相机 | USB | 免驱，v4l2 |
