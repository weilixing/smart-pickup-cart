# 智能取货小车 — RA8P1 开发板记录

用途：记录板子硬件、开发环境、烧录调试流程，避免以后重新踩坑。  
更新：2026-11-27（来自本地笔记同步）

## 1. 板子硬件

| 项目 | 内容 |
| --- | --- |
| 核心板 | 逐飞 RA8P1K（瑞萨 RA8P1，器件 R7JA8P1KRLSAJ，Cortex-M85） |
| 扩展板 | 逐飞 CPK HDMI 扩展板 V1.0 |
| 板载调试器 | SEGGER J-Link OB（虚拟串口 COM4） |
| 逐飞开源库 | `E:\RA8P1_AI_Kit-master.zip`（引脚定义、例程都在里面） |

用户 LED（**低电平点亮**，拉低亮 / 拉高灭）：

| LED | 引脚 | FSP 宏 |
| --- | --- | --- |
| LED1 | P411 | `BSP_IO_PORT_04_PIN_11` |
| LED2 | P514 | `BSP_IO_PORT_05_PIN_14` |
| LED3 | P110 | `BSP_IO_PORT_01_PIN_10` |
| LED4 | P010 | `BSP_IO_PORT_00_PIN_10` |

注意：之前误以为 LED 在「108/109/110」，实际只有 110（P1.10 = LED3）是对的。

### 串口（来自逐飞 `E1_07_ttl_uart_demo`）

| 用途 | 方向 | 引脚 | 说明 |
| --- | --- | --- | --- |
| 用户 TTL UART1 TX | 板子→外设 | **P707** | 接香橙派 USB-TTL 的 **RX** |
| 用户 TTL UART1 RX | 外设→板子 | **P706** | 接香橙派 USB-TTL 的 **TX** |
| 调试串口 TX | printf | P209 | J-Link/调试打印，勿与业务混用 |
| 调试串口 RX | — | P208 | 同上 |

业务链路用 **UART1（P706/P707）**，115200 8N1，与香橙派 **交叉、共地、3.3V**。  
SYN6288 等模块也走交叉接法；若 UART1 已给 Pi，语音需另开一路 UART 或先共用再拆。

例程宏：`UART_INDEX (UART_1)`，`UART1_TX_P707` / `UART1_RX_P706`。

## 2. 开发环境

| 项目 | 内容 |
| --- | --- |
| IDE | e² studio 2026-10（`C:\Renesas\e2_studio`） |
| 软件包 | FSP 6.6.0（含 FreeRTOS） |
| 工程 | `E:\e2fsp\tes\call_flat` |
| 工程类型 | Flat（非 TrustZone）单核，器件 R7JA8P1KRLSAJ |
| 模板 | FreeRTOS - Minimal |

旧的 `test_CPU*` 系列（TrustZone 双核）已废弃，别再用。

## 3. 编译 / 烧录 / 调试流程

标准流程：改代码 → Ctrl+S → Build → Debug（J-Link ARM）→ **Resume（F8）**。

代码要点：写引脚前先 `R_BSP_PinAccessEnable();`

```c
#include "new_thread0.h"

void new_thread0_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED(pvParameters);
    R_BSP_PinAccessEnable();

    while (1) {
        R_BSP_PinWrite(BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_LOW);   // LED3 亮
        vTaskDelay(pdMS_TO_TICKS(1000));
        R_BSP_PinWrite(BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_HIGH);  // 灭
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## 4. 踩坑记录

| 坑 | 原因 | 解决 |
| --- | --- | --- |
| 烧录 failed / SecureFault | TrustZone 双核工程 | 换 Flat 非 TrustZone |
| 写引脚 HardFault | PFS 写保护 | `R_BSP_PinAccessEnable()` |
| Debug 后灯不亮 | 停在 main | Resume（F8） |
| LED 引脚错 | 记成 108/109/110 | P411/P514/P110/P010 |
| LED 方向反 | 以为拉高亮 | 实际低电平亮 |

## 5. 与小车固件的衔接（当前方向）

香橙派侧：ROS2 +（计划）YDLIDAR X3 建图导航。  
RA8P1 侧：电机 / 里程计 / 取货执行机构 / 语音 / 与 Pi 的串口协议。

**下一步：UART1 ↔ 香橙派心跳联调**

1. 参考 `firmware/ra8/examples/uart_led_ping.c`，对照 `E1_07_ttl_uart_demo` 改 API 名后贴进 `new_thread0_entry`  
2. 接线：Pi TX→P706，Pi RX→P707，GND 共地  
3. 香橙派：`python3 scripts/uart_ping_test.py /dev/ttyUSB0`  
4. 期望：RA8 LED3 闪烁，PC/Pi 侧打印收到 ACK `aa5590...`
