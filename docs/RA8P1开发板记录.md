# 智能取货小车 — RA8P1 开发板记录

用途：记录板子硬件、开发环境、烧录调试流程，避免以后重新踩坑。  
更新：2026-11-28（UART 双板连通 10/10 ACK）

## 1. 板子硬件

| 项目 | 内容 |
| --- | --- |
| 核心板 | 逐飞 RA8P1K（瑞萨 RA8P1，器件 R7JA8P1KRLSAJ，Cortex-M85） |
| 扩展板 | 逐飞 CPK HDMI 扩展板 V1.0 |
| 板载调试器 | SEGGER J-Link OB（虚拟串口 COM4） |
| 逐飞开源库 | `E:\RA8P1_AI_Kit-master.zip`（引脚定义、例程都在里面） |

用户 LED（**LED3 实测高电平点亮**：拉高亮 / 拉低灭）：

| LED | 引脚 | FSP 宏 |
| --- | --- | --- |
| LED1 | P411 | `BSP_IO_PORT_04_PIN_11` |
| LED2 | P514 | `BSP_IO_PORT_05_PIN_14` |
| LED3 | P110 | `BSP_IO_PORT_01_PIN_10` |
| LED4 | P010 | `BSP_IO_PORT_00_PIN_10` |

### 串口（逐飞 `E1_07_ttl_uart_demo`）

| 用途 | 方向 | 引脚 | 说明 |
| --- | --- | --- | --- |
| 用户 TTL UART1 TX | 板子→外设 | **P707** | 接香橙派 Pin29（URXD7） |
| 用户 TTL UART1 RX | 外设→板子 | **P706** | 接香橙派 Pin7（UTXD7） |
| 调试串口 TX | printf | P209 | 勿与业务混用 |
| 调试串口 RX | — | P208 | 同上 |

业务链路：**SCI1 / P706·P707**，115200 8N1，交叉、共地、3.3V。

## 2. 开发环境

| 项目 | 内容 |
| --- | --- |
| IDE | e² studio 2026-10 |
| 软件包 | FSP 6.6.0（含 FreeRTOS） |
| 工程 | `E:\e2fsp\tes\call_flat` |
| 工程类型 | Flat（非 TrustZone） |
| 模板 | FreeRTOS - Minimal |

## 3. 编译 / 烧录 / 调试

改代码 → Build → Debug（J-Link）→ **Resume（F8）**。  
写引脚前：`R_BSP_PinAccessEnable();`

## 4. 踩坑记录

| 坑 | 原因 | 解决 |
| --- | --- | --- |
| 烧录 failed / SecureFault | TrustZone 双核 | 换 Flat |
| 写引脚 HardFault | PFS 写保护 | `R_BSP_PinAccessEnable()` |
| Debug 后灯不亮 | 停在 main | Resume（F8） |
| UART 驱动名 | 写成 `r_sci_uart` | 用 **`r_sci_b_uart`**，API 为 `R_SCI_B_UART_*` |
| SCICLK Disabled | Custom 板 Clocks 锁死 | 改 `bsp_clock_cfg.h`：`BSP_CFG_SCICLK_SOURCE` = HOCO；Generate/Clean 会还原 |
| **波特率 100% 误差** | Generate 时 SCICLK=0，`g_uart0_baud_setting` 错 | **Open 后** `BaudCalculate(115200)` + `BaudSet`（关键） |
| 香橙派 UART2 无数据 | ttyAMA1 / Pin11·36 环回不通 | 改用 **UART7**：Pin7/29，`/dev/ttyAMA2` |
| Read 缓冲 | 栈上临时 `uint8_t ch` | 改为 **static** 缓冲 |
| LED3 方向 | 早期记成低电平亮 | 实测 **高电平亮** |

## 5. 双板连通（已打通）

固件：`firmware/ra8/examples/uart_led_ping.c` → 替换 `new_thread0_entry.c`  
上位机：`python3 scripts/uart_ping_test.py /dev/ttyAMA2`

| 香橙派 | RA8 |
| --- | --- |
| Pin7 UTXD7 | → P706 RX |
| Pin29 URXD7 | ← P707 TX |
| GND | → GND |

期望：上电快闪 3 下；心跳后回 `aa5590010392`，终端打印「收到 ACK」。  
实测：**10/10 ACK**。

根因一句话：**FSP 在 SCICLK=Disabled 时生成了错误波特率表，必须运行时重算波特率；香橙派必须用 UART7。**
