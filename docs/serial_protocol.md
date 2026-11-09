# OrangePi ↔ RA8 串口协议（循迹 / 避障 / 机械臂 / 语音版）

- 链路：UART，115200 8N1，3.3V，**必须共地**  
- 整数：小端  
- 帧：`[0xAA, 0x55, CMD, LEN, PAYLOAD..., CRC]`  
- `CRC` = 从 CMD 到 PAYLOAD 末尾逐字节 XOR  

## 下行（OrangePi → RA8）

| CMD | 名称 | PAYLOAD | 说明 |
| --- | --- | --- | --- |
| 0x01 | SET_MODE | uint8 mode | 0=停止，1=遥控，2=循迹任务，3=急停 |
| 0x02 | SET_VEL | int16 linear_mm_s + int16 angular_mrad_s | 遥控模式下的目标速度，20 Hz |
| 0x03 | HEARTBEAT | 空 | 1 Hz；RA8 连续 500 ms 无下行帧则急停 |
| 0x10 | ARM_SEQ | uint8 seq_id | 0=回零，1=抓取序列，2=投放序列 |
| 0x11 | ARM_JOINT | uint8 joint + uint16 angle_deg×10 | 单关节调试（joint 0–3） |
| 0x20 | VOICE_PLAY | uint8 index 或 UTF-8 文本（LEN 可变） | index 模式：预置提示语编号；文本模式见固件 |

## 上行（RA8 → OrangePi）

| CMD | 名称 | PAYLOAD | 说明 |
| --- | --- | --- | --- |
| 0x81 | ODOM | int32 x_mm + int32 y_mm + int32 theta_mrad + int16 v + int16 w | 50 Hz |
| 0x82 | LINE_STATUS | int16 error_mm + uint8 on_line + uint8 station | 循迹偏差、是否在线、站点标志 |
| 0x83 | OBSTACLE | uint16 front_mm + uint16 left_mm + uint16 right_mm + uint8 state | state：0正常 1减速 2绕行 |
| 0x84 | ARM_STATE | uint8 busy + uint8 last_seq + uint8 ok | 机械臂忙闲与上次序列结果 |
| 0x85 | VOICE_DONE | uint8 index | 播报完成 |
| 0x86 | IMU | int16 ax + ay + az + gz | 可选 |

对应实现：`firmware/ra8/include/protocol.h`、`ros2_ws/src/motor_bridge/motor_bridge/protocol.py`。
