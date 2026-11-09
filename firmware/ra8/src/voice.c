#include "voice.h"
#include "board_hal.h"
#include <string.h>

/*
 * SYN6288 常见帧：0xFD + 数据区长度高/低 + 命令 + GBK/文本...
 * 这里先用“预置词条编号”方案：RA8 内置几句固定 GB2312/GBK 文本。
 * 若模块已录好提示音，可改成发对应编号命令。
 */

typedef struct {
    uint8_t busy;
    uint8_t pending_done;
    uint8_t last_index;
    uint32_t start_ms;
} voice_t;

static voice_t g;

/* 简化：只发 index 对应的短命令占位，实机按 SYN6288 手册改写 */
static void send_index_frame(uint8_t index)
{
    /* 示例帧：FD 00 02 01 <index> —— 仅为占位，需按手册替换 */
    uint8_t frame[5] = {0xFD, 0x00, 0x02, 0x01, index};
    board_voice_uart_write(frame, sizeof(frame));
}

void voice_init(void)
{
    memset(&g, 0, sizeof(g));
    g.pending_done = 0xFF;
}

void voice_play_index(uint8_t index)
{
    send_index_frame(index);
    g.busy = 1;
    g.last_index = index;
    g.start_ms = board_millis();
}

void voice_update(void)
{
    if (!g.busy) {
        return;
    }
    /* 无忙脚时用超时近似“播完”（约 1.5s），有 BUSY 脚可改成读 GPIO */
    if ((board_millis() - g.start_ms) > 1500u) {
        g.busy = 0;
        g.pending_done = g.last_index;
    }
}

bool voice_busy(void)
{
    return g.busy != 0;
}

uint8_t voice_last_done_index(void)
{
    uint8_t v = g.pending_done;
    g.pending_done = 0xFF;
    return v;
}
