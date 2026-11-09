# 基于 ROS2 的智能取货小车

按开题报告方案实现：**循迹行进 → 超声波避障 → 四自由度舵机机械臂抓取投放 → 返航 → 关键节点语音播报**。

双板架构：

| 角色 | 硬件 | 职责 |
| --- | --- | --- |
| 上层决策 | OrangePi AIpro（8T） | Ubuntu 22.04 + ROS2 Humble：任务状态机、语音播报调度 |
| 下层实时 | 瑞萨 RA8（Cortex-M85） | 循迹、避障、电机 PID、机械臂舵机、SYN6288 语音驱动 |

## 功能清单

1. **循迹**：8 路灰度传感器阵列，沿黑色引导线行进  
2. **避障**：前方与两侧超声波，近距红外补充；策略为减速 → 绕行 → 回线  
3. **机械臂**：四自由度舵机（底座回转、大臂、小臂、夹爪）抓取与投放  
4. **返航**：投放后沿引导线返回出发点  
5. **语音**：SYN6288 在到达取货点、抓取完成、投放完成、开始返航等节点播报  
6. **双板协调**：自定义 UART 协议 + 通信中断急停  

## 仓库结构

```
ros2_ws/src/
  pickup_description/   # URDF/xacro（底盘 + 4DOF 臂）
  motor_bridge/         # OrangePi ↔ RA8 串口桥
  voice_announce/       # 语音播报调度（经串口下发文本/索引）
  arm_controller/       # 抓取 / 投放动作序列
  task_orchestrator/    # 状态机：循迹→取货→投放→返航
  pickup_bringup/       # 整车 bringup
firmware/ra8/           # RA8 固件（循迹/避障/电机/舵机/语音/协议）
docs/                   # 机械规格、串口协议、接线表、开题底稿
开题报告.doc / .docx    # 正式提交稿（以卫立星填写版为准）
```

## 开发顺序（对齐开题计划）

1. OrangePi 刷机 + ROS2 Humble；RA8 e2 studio 工程  
2. RA8：编码器 + 轮速 PID + 串口协议 → 底盘遥控  
3. 灰度阵列循迹 + 超声波避障协同  
4. 四自由度臂装配、舵机标定与抓取路径；接入 SYN6288  
5. 上层状态机联调完整闭环  
6. 连续演示与指标测试  

## 快速开始（OrangePi）

刷机与安装 ROS2 的完整步骤见 **[docs/orangepi_ros2_setup.md](docs/orangepi_ros2_setup.md)**。

```bash
cd ros2_ws && colcon build && source install/setup.bash
ros2 launch pickup_bringup bringup.launch.py serial_port:=/dev/ttyUSB0
```

协议与接线见 `docs/serial_protocol.md`、`docs/wiring.md`。
