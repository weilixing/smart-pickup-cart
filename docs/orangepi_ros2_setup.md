# OrangePi AIpro 安装 ROS2 Humble

适用前提：板子已刷 **官方 Ubuntu 22.04** 镜像，能联网，能 `sudo`。

架构一般为 `aarch64`。若 `lsb_release -a` 不是 22.04，先换系统再装 ROS2。

## 0. 确认系统

```bash
uname -m          # 期望 aarch64
lsb_release -a    # 期望 Ubuntu 22.04
```

建议先更新：

```bash
sudo apt update
sudo apt upgrade -y
```

## 1. 设置软件源与密钥

```bash
sudo apt install -y curl gnupg lsb-release software-properties-common

sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg

echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" \
| sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

sudo apt update
```

若访问 GitHub / packages.ros.org 很慢，可改用国内镜像（任选其一，不要重复添加）：

```bash
# 清华源示例
sudo curl -sSL https://mirrors.tuna.tsinghua.edu.cn/rosdistro/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg

echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
https://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu jammy main" \
| sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

sudo apt update
```

## 2. 安装 ROS2 Humble

桌面调试（带 RViz）推荐：

```bash
sudo apt install -y ros-humble-desktop
```

若空间紧张、只要命令行：

```bash
sudo apt install -y ros-humble-ros-base
```

本项目还会用到：

```bash
sudo apt install -y \
  python3-colcon-common-extensions \
  python3-rosdep \
  python3-serial \
  ros-humble-xacro \
  ros-humble-robot-state-publisher \
  ros-humble-teleop-twist-keyboard
```

初始化 rosdep（首次）：

```bash
sudo rosdep init || true
rosdep update
```

## 3. 环境变量（每次开终端生效）

```bash
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

验证：

```bash
ros2 --help
echo $ROS_DISTRO    # 应输出 humble
```

## 4. 拉代码并编译本仓库

```bash
cd ~
git clone https://github.com/weilixing/smart-pickup-cart.git
cd smart-pickup-cart/ros2_ws

# 若缺依赖，可尝试：
# rosdep install --from-paths src -y --ignore-src

colcon build --symlink-install
source install/setup.bash
echo "source ~/smart-pickup-cart/ros2_ws/install/setup.bash" >> ~/.bashrc
```

快速自检（可不接 RA8）：

```bash
ros2 pkg list | grep -E 'motor_bridge|arm_controller|voice_announce|task_orchestrator|pickup_'
ros2 launch pickup_description description.launch.py
# 另开一个终端（已 source）：
ros2 topic list
```

## 5. 串口权限（联调 RA8 时再用）

```bash
sudo usermod -aG dialout $USER
# 重新登录后生效
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

联调示例：

```bash
source ~/smart-pickup-cart/ros2_ws/install/setup.bash
ros2 launch pickup_bringup bringup.launch.py serial_port:=/dev/ttyUSB0
# 另开终端遥控：
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

## 常见问题

1. **`ros-humble-desktop` 找不到**  
   检查是否 Ubuntu 22.04、`ros2.list` 是否指向 `jammy`，再 `sudo apt update`。

2. **网络超时**  
   换清华 / 中科大镜像后重试；或电脑下好 `.deb` 拷到板子 `sudo dpkg -i`。

3. **`colcon build` 报缺少包**  
   先 `sudo apt install python3-colcon-common-extensions`，再按报错补对应 `ros-humble-*`。

4. **架构不是 amd64**  
   正常。AIpro 用 aarch64 官方源即可，不要装 amd64 包。
