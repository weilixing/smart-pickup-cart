#!/usr/bin/env bash
# OrangePi AIpro 算力/资源体检（不依赖雷达实物）
# 用法：bash scripts/orangepi_bench.sh
set -euo pipefail

echo "========== 1. 机器概况 =========="
date
uname -a
echo "---"
lscpu | egrep 'Model name|CPU\(s\)|Thread|MHz|Architecture' || true
echo "---"
free -h
echo "---"
df -h / | tail -1
echo "---"
nproc

echo
echo "========== 2. ROS2 环境 =========="
# setup.bash 会读未定义变量，临时关闭 nounset
set +u
if [ -f /opt/ros/humble/setup.bash ]; then
  # shellcheck disable=SC1091
  source /opt/ros/humble/setup.bash
  echo "ROS_DISTRO=$ROS_DISTRO"
  ros2 --version 2>/dev/null || true
else
  echo "未找到 /opt/ros/humble，请先装 ROS2"
fi

if [ -f "$HOME/smart-pickup-cart/ros2_ws/install/setup.bash" ]; then
  # shellcheck disable=SC1091
  source "$HOME/smart-pickup-cart/ros2_ws/install/setup.bash"
  echo "已 source 工作空间"
  ros2 pkg list 2>/dev/null | grep -E 'motor_bridge|pickup_|arm_|voice_|task_' || true
fi
set -u

echo
echo "========== 3. CPU 压测（约 20 秒） =========="
# 多核满载，观察是否立刻卡死/过热降频
if command -v stress-ng >/dev/null 2>&1; then
  stress-ng --cpu "$(nproc)" --timeout 20s --metrics-brief
elif command -v stress >/dev/null 2>&1; then
  stress --cpu "$(nproc)" --timeout 20
else
  echo "未安装 stress-ng，改用 Python 简易压测..."
  python3 - <<'PY'
import time, threading, os
def burn():
    end = time.time() + 15
    x = 0.0
    while time.time() < end:
        x = x * 1.0000001 + 1.2345
    return x
n = os.cpu_count() or 4
ts = [threading.Thread(target=burn) for _ in range(n)]
t0 = time.time()
for t in ts: t.start()
for t in ts: t.join()
print(f"Python burn {n} threads done in {time.time()-t0:.1f}s")
PY
fi

echo
echo "========== 4. 磁盘顺序写（粗测） =========="
TMP=/tmp/opi_bench_$$
dd if=/dev/zero of="$TMP" bs=8M count=64 status=progress 2>&1 | tail -3
rm -f "$TMP"

echo
echo "========== 5. ROS2 topic 吞吐冒烟（约 10 秒） =========="
if command -v ros2 >/dev/null 2>&1; then
  ros2 topic pub -r 50 /bench/twist geometry_msgs/msg/Twist "{linear: {x: 0.1}}" >/tmp/opi_pub.log 2>&1 &
  PUB_PID=$!
  sleep 1
  timeout 8 ros2 topic hz /bench/twist 2>&1 | tail -5 || true
  kill "$PUB_PID" 2>/dev/null || true
  wait "$PUB_PID" 2>/dev/null || true
else
  echo "跳过：无 ros2"
fi

echo
echo "========== 6. 建议解读 =========="
echo "把本脚本完整输出保存发回即可。"
echo "经验门槛（Humble + 雷达建图）："
echo "  - 内存建议可用 > 2.5GB（跑 slam+nav2 更舒服 > 4GB）"
echo "  - CPU 压测 20s 内不整机卡死、无明显狂降频"
echo "  - ros2 topic hz 在 50Hz pub 下能接近 50Hz"
echo "YDLIDAR X3 + slam_toolbox 一般 AIpro 扛得住；Nav2 全开建议先用默认参数、降代价地图频率。"
echo "8T NPU 留给后期视觉；建图导航主要吃 CPU/RAM。"
echo "========== 结束 =========="
