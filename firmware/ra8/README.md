# RA8 下位机固件框架

与开题方案一致：电机 PID、灰度循迹、超声波避障、四自由度舵机臂、SYN6288 语音、与 OrangePi 的串口协议。

上层 ROS2 协议见 `docs/serial_protocol.md`，头文件 `include/protocol.h` 与 `ros2_ws/.../protocol.py` 对应。

## 目录

```
firmware/ra8/
  include/          公共头文件（protocol / HAL / 各模块）
  src/
    main.c          入口：app_init + app_loop
    app.c           模式调度与时间片
    comm.c          帧解析与上报
    motor.c         差速解算 + 速度环 + 里程计
    line_follow.c   8 路灰度 PD 循迹
    obstacle.c      超声波减速 / 绕行
    arm.c           抓取 / 投放角度序列（示教值待改）
    voice.c         SYN6288 播报占位
    board_hal_stub.c          无板冒烟用
    board_hal_fsp.c.example   上板时改成 FSP 实现
  protocol.h        根目录副本（兼容旧路径）
```

## 接入 e2 studio

1. 新建 RA8 FSP 工程（按你的板型选 BSP）。  
2. 配置：电机 PWM、编码器、ADC、超声波测宽、舵机 PWM、双 UART、系统滴答。  
3. 把本目录 `include/` 加到头文件路径，把 `src/*.c` 加入工程。  
4. 复制 `board_hal_fsp.c.example` → `board_hal.c`，用 FSP API 填空；**不要**同时编译 `board_hal_stub.c`。  
5. 若工程已有 `hal_entry()`，可在其中调用 `app_init()` / `app_loop()`，不必用 `main.c`。

## 运行模式（下行 SET_MODE）

| mode | 含义 |
| --- | --- |
| 0 | 停车 |
| 1 | 遥控（跟 SET_VEL） |
| 2 | 循迹任务（内部循迹 + 避障） |
| 3 | 急停 |

心跳：500 ms 无任何下行帧 → 电机急停。

## 建议调试顺序

1. 只接串口：心跳 + SET_MODE/SET_VEL，用 stub 或实机看回环  
2. 电机开环 duty → 编码器方向 → 调 `TICKS_PER_METER` 与 PID  
3. 灰度循迹  
4. 超声波避障  
5. 改 `arm.c` 里示教角度  
6. 按 SYN6288 手册改 `voice.c` 帧格式  

## 本地冒烟（可选）

无板时可在 Linux 上粗编译检查符号（不能链真实 FSP）：

```bash
cd firmware/ra8
gcc -Iinclude -c src/*.c
```
