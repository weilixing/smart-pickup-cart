# 香橙派算力尝试（先于雷达实车）

目的：确认 OrangePi AIpro 是否扛得住 **ROS2 + YDLIDAR X3 建图/导航**，再决定后面结构。

## 快速跑

```bash
cd ~/smart-pickup-cart
git pull
chmod +x scripts/orangepi_bench.sh

# 建议先退出 conda，避免污染
conda deactivate 2>/dev/null || true

# 可选：更准的 CPU 压测
sudo apt install -y stress-ng

bash scripts/orangepi_bench.sh | tee ~/opi_bench_$(date +%Y%m%d_%H%M).log
```

把生成的 log 发我。

## 我们看什么

| 项目 | 及格 | 舒服 |
| --- | --- | --- |
| 可用内存 | ≥ 2.5 GB | ≥ 4 GB |
| CPU 满载 20s | 不死机、能 SSH | 温度可控、不明显卡顿 |
| `ros2 topic hz` | 接近发布频率 | 稳定 |
| 磁盘剩余 | ≥ 8 GB | ≥ 15 GB |

## 分档结论（拿到 log 后对号）

1. **够用**：先上 X3，跑 `ydlidar` + `slam_toolbox` 建一张小地图。  
2. **勉强**：建图可以，Nav2 参数降负载（降频率、小地图、少层代价地图）。  
3. **偏紧**：导航放远程电脑做（板子只驱动+桥），或减节点。

## 有雷达之后的第二步（现在可先不买）

```bash
# 示例：装驱动后再测（型号以 X3 官方包为准）
sudo apt install -y ros-humble-slam-toolbox ros-humble-nav2-bringup
# 再按 YDLIDAR 文档编译/安装 x3 的 ros2 驱动
```

第二步才测：`/scan` 频率、slam 时 `top`/`free`。

## 和 NPU 的关系

- **建图 / Nav2**：主要用 CPU + RAM  
- **8T NPU**：以后做视觉识别再启用，本次体检不强制  

取货机构（臂/吸盘/顶升）都不吃香橙派算力，可后定。
