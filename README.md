# 智能取货小车（ROS2）

一辆演示型取货机器人，跑通完整闭环：**接单 → 前往取货点取货 → 送到指定位置 → 返回出发点待命**。

路线定稿：**先 B 后 A** —— 一期用潜入顶升（Kiva 同款）跑通闭环，二期升级机械臂抓取。

## 硬件架构

| 角色 | 硬件 | 分工 |
| --- | --- | --- |
| ROS2 大脑 | OrangePi AIpro（8T） | 官方 Ubuntu 22.04 + ROS2 Humble LTS；Nav2 / SLAM / 视觉 / 任务编排 |
| 电机下位机 | 瑞萨 RA8（Cortex-M85） | 电机 PID、编码器里程计、RFID 读取；UART 自定义协议与主控通信 |
| 导航传感器 | YDLIDAR X2 / LD19 | /scan → slam_toolbox 建图 → Nav2 定位导航 |
| 视觉 | USB 广角相机 | ArUco / 二维码识别（免驱动，不走 MIPI） |
| 执行机构（一期） | NEMA17 + T8 丝杆顶升平台 | 钻入货架底部顶升驮运，RFID 确认身份 |
| 执行机构（二期） | 6 自由度总线舵机机械臂套件 | ArUco 视觉伺服抓取 |

避坑提示：OrangePi 供电按官方规格给足电流并与电机电源隔离；相机一律走 USB。

## 物理规格（默认设计，详见 [docs/mechanical.md](docs/mechanical.md)）

- 场地：3m × 3m 室内硬地面，取货区 / 投递区 / 充电桩位三个功能点
- 标准货箱：120 × 120 × 100 mm，≤500 g
- 货架：托盘台面 160 × 160 mm，腿高 140 mm（净空 130 mm 供车钻入），RFID 卡贴托盘底面中心
- 车体：甲板 200 × 160 mm，Ø85 轮 ×4，整车最高点 ≤115 mm；顶升行程 50 mm
- 雷达车头低位安装（~90 mm，低于货架净空）；相机立杆为二期臂版专用

## 采购清单（一期）

| 部件 | 建议型号 | 参考价 |
| --- | --- | --- |
| 电机驱动 | TB6612 / AT8236 模块 | ¥25 |
| 底盘 | 4WD 差速底盘 + 编码器电机 ×4（MG513 级别） | ¥350 |
| IMU | MPU6050 / ICM20948 | ¥40 |
| 相机 | USB 广角相机 | ¥60 |
| 激光雷达 | YDLIDAR X2 / LD19 | ¥250 |
| 电源 | 3S 18650 电池组 + 稳压模块 | ¥160 |
| 顶升机构 | NEMA17 + T8 丝杆 + 驱动 | ¥120 |
| 货架 / 托盘 | 成品打印件（按车高与货物尺寸定制） | ¥80 |
| RFID | RC522 模块 + 标签卡 | ¥15 |
| 结构件 | 成品打印件：立柱、云台、支架 | ¥150 |
| 线材杂项 | 杜邦线、开关、保险 | ¥30 |

一期合计约 **¥1,250**（主控为现有硬件）；二期机械臂套件 +¥600~800。

## 软件架构（ROS2 Humble）

| 节点 | 运行位置 | 职责 |
| --- | --- | --- |
| `motor_bridge` | RA8 + OrangePi 串口 | 下发 `/cmd_vel`，上传编码器里程计 `/odom` |
| `robot_state_publisher` | OrangePi | URDF 与 TF 树（base_link / camera / lift） |
| `ydlidar_ros2_driver` | OrangePi | 发布 `/scan` |
| `slam_toolbox` | OrangePi | SLAM 建图 |
| `nav2_bringup` | OrangePi | 定位 + 点到点导航 |
| `rfid_reader` | RA8 固件 + 桥接 | 读取货架标签 → `/rfid_id` |
| `lift_controller` | OrangePi | 一期：顶升升降 action |
| `aruco_detect` | OrangePi | 二期：货物与站点标记识别 |
| `arm_controller` | OrangePi | 二期：机械臂轨迹与抓取 action |
| `task_orchestrator` | OrangePi | 任务状态机：接单 → 取 → 送 → 回 |

## 仓库结构

```
ros2_ws/src/
  pickup_description/   # URDF/xacro 模型 + robot_state_publisher launch
  motor_bridge/         # OrangePi ↔ RA8 串口桥（cmd_vel / odom / imu / rfid / 顶升）
  lift_controller/      # 顶升平台 /lift/set 服务
  aruco_detect/         # ArUco 标记识别（二期货物位姿）
  task_orchestrator/    # 任务状态机：接单 → 取 → 送 → 回
  pickup_bringup/       # 整车 bringup launch + EKF / SLAM 配置
firmware/ra8/           # RA8 固件：protocol.h + 搭建指引
docs/                   # mechanical.md（尺寸设计）、serial_protocol.md、wiring.md
```

## 快速开始（OrangePi 上）

```bash
# 1. 安装依赖（Ubuntu 22.04 + ROS2 Humble）
sudo apt install ros-humble-robot-localization ros-humble-slam-toolbox \
  ros-humble-nav2-bringup ros-humble-nav2-simple-commander \
  ros-humble-cv-bridge python3-serial python3-opencv

# 2. 编译
cd ros2_ws && colcon build && source install/setup.bash

# 3. 点亮底盘（键盘遥控）
ros2 launch pickup_bringup bringup.launch.py serial_port:=/dev/ttyUSB0
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# 4. 建图与导航（雷达驱动用 YDLIDAR 官方包）
ros2 launch ydlidar_ros2_driver ydlidar_launch.py
ros2 launch slam_toolbox online_async_launch.py \
  params_file:=install/pickup_bringup/share/pickup_bringup/config/slam_toolbox.yaml
ros2 launch nav2_bringup navigation_launch.py map:=/path/to/map.yaml

# 5. 跑一次取送闭环（站点坐标先在 RViz 里量好）
ros2 run task_orchestrator orchestrator_node --ros-args \
  -p home_pose:="[0.0, 0.0, 0.0]" -p pickup_pose:="[1.0, 0.0, 0.0]" \
  -p deliver_pose:="[0.0, 1.0, 0.0]"
ros2 topic pub --once /start_delivery std_msgs/msg/Bool "{data: true}"
```

## 推进路线

- [ ] 一期 · OrangePi 刷机 + ROS2 Humble + RA8 电机 PID 点亮底盘，键盘遥控
- [ ] 一期 · URDF / TF 树 + EKF（轮式里程计 + IMU 融合）
- [ ] 一期 · 雷达建图（slam_toolbox）+ Nav2 定点导航
- [ ] 一期 · 顶升机构 + RFID 集成，跑通完整闭环（**里程碑：可演示**）
- [ ] 一期 · 演示打磨：充电桩位、失败重试、灯效 / 语音反馈
- [ ] 二期 · 机械臂套件装配 + 臂控节点接入
- [ ] 二期 · ArUco 视觉伺服抓取调优（可接 MoveIt2）
- [ ] 二期 · 臂版取货与顶升双模式并存
