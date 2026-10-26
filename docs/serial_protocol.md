# OrangePi ↔ RA8 串口协议

- 链路：UART，115200 8N1，3.3V 电平（经 USB-TTL 或板间直连，**必须共地**）
- 方向：OrangePi（ROS2 `motor_bridge` 节点）⇄ RA8 固件
- 整数一律小端（little-endian）

## 帧格式

```
+--------+--------+-----+-----+==========+-----+
| 0xAA   | 0x55   | CMD | LEN | PAYLOAD  | CRC |
+--------+--------+-----+-----+==========+-----+
   1B       1B      1B    1B     LEN B     1B
```

- `LEN` = PAYLOAD 字节数（0~255）
- `CRC` = 从 CMD 到 PAYLOAD 末尾所有字节逐字节 XOR

## 命令字

### 下行（OrangePi → RA8）

| CMD | 名称 | PAYLOAD | 说明 |
| --- | --- | --- | --- |
| 0x01 | SET_VEL | int16 linear_mm_s + int16 angular_mrad_s | 目标线速度（mm/s）与角速度（mrad/s），20Hz 周期发送 |
| 0x02 | LIFT_CMD | uint8 state（0=降，1=升） | 顶升平台目标状态 |
| 0x03 | HEARTBEAT | 空 | 1Hz 心跳；RA8 连续 500ms 未收到任何下行帧则急停电机 |

### 上行（RA8 → OrangePi）

| CMD | 名称 | PAYLOAD | 说明 |
| --- | --- | --- | --- |
| 0x81 | ODOM | int32 x_mm + int32 y_mm + int32 theta_mrad + int16 v_mm_s + int16 w_mrad_s | MCU 侧积分的里程计，50Hz |
| 0x82 | LIFT_STATE | uint8 state（0=已降下，1=已升起，2=运动中） | 到位后上报 |
| 0x84 | RFID | 12B ASCII（十六进制 UID 字符串，不足补 0x00） | 读到标签时上报一次 |
| 0x85 | IMU | int16 ax_mg + int16 ay_mg + int16 az_mg + int16 gz_cdps | 加速度（mg）与 Z 轴角速度（0.01°/s），100Hz |

> 里程计由 MCU 积分是 v1 取舍；后续可改为上报原始编码器 tick，由 ROS 侧 `robot_localization` 融合。

对应固件头文件见 `firmware/ra8/protocol.h`，ROS 侧解析见 `ros2_ws/src/motor_bridge/motor_bridge/protocol.py`。
