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
ros2_ws/        # ROS2 工作空间（待搭建）
firmware/ra8/   # RA8 下位机固件（待搭建）
docs/           # 接线表、串口协议等文档
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
