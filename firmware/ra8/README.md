# RA8 下位机固件

在 e2 studio + FSP 中新建工程（按手上的具体 RA8 板型选择），实现以下四个模块。
`protocol.h` 可直接加入工程，与 ROS 侧 `motor_bridge` 节点对应。

## 模块划分

| 模块 | 职责 | 周期 |
| --- | --- | --- |
| 电机 PID | 读 4 路编码器 → 左右两侧速度闭环 → TB6612 PWM | 1 kHz |
| 里程计积分 | 左右轮速 → x / y / θ 积分，ODOM 帧上报 | 50 Hz |
| 串口协议 | 解析 SET_VEL / LIFT_CMD，发送 ODOM / IMU / RFID / LIFT_STATE | 事件 + 周期 |
| 机构与外设 | IMU（I2C）、RC522（SPI）、顶升丝杆 STEP/DIR + 双限位 | 按需 |

## FSP 配置要点

- **GPT × N**：2 路 PWM 输出（TB6612 PWMA/PWMB，≥20kHz 避免啸叫）；编码器用输入捕获或外部 IRQ 计数
- **SCI UART**：115200 8N1，接 OrangePi（或经 USB-TTL），开 RX 中断做帧解析状态机
- **IIC（I2C）**：MPU6050/ICM20948，读取后按 `proto_imu_t` 定点化上报
- **SPI**：RC522 RFID，读到卡即上报一次 UID
- **GPIO**：顶升 STEP/DIR + 上下限位（带中断）；限位触发立即停步进

## 安全约束（先写进固件，别靠上位机）

1. **看门狗急停**：500ms 未收到任何下行帧 → 电机 PWM 归零
2. **限位优先**：顶升运动中被触发限位立即停并上报 LIFT_STATE
3. **软启动**：SET_VEL 目标值做斜坡限制（如 500 mm/s²），保护电机与齿轮

## 调试顺序

1. 先调串口回环 + HEARTBEAT，确认帧编解码一致（用 `motor_bridge` 的 `protocol.py` 做对照）
2. 单电机开环转 → 读编码器方向 → 再闭 PID
3. 里程计直线走 1m 校准 `ticks_per_meter`，原地转 360° 校准轮距
4. 最后接顶升与 RFID
