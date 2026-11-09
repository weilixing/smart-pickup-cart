#include "comm.h"
#include "board_hal.h"
#include <string.h>

static host_cmd_t g_cmd;
static uint8_t g_rx[128];
static uint16_t g_rx_len;

void comm_init(void)
{
    memset(&g_cmd, 0, sizeof(g_cmd));
    g_cmd.mode = 0;
    g_cmd.arm_seq_req = 0xFF;
    g_cmd.voice_index_req = 0xFF;
    g_cmd.last_rx_ms = board_millis();
    g_rx_len = 0;
}

const host_cmd_t *comm_cmd(void)
{
    return &g_cmd;
}

void comm_clear_arm_req(void)
{
    g_cmd.arm_seq_req = 0xFF;
}

void comm_clear_voice_req(void)
{
    g_cmd.voice_index_req = 0xFF;
}

static void handle_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    g_cmd.last_rx_ms = board_millis();
    switch (cmd) {
    case PROTO_CMD_SET_MODE:
        if (len >= 1) {
            g_cmd.mode = payload[0];
        }
        break;
    case PROTO_CMD_SET_VEL:
        if (len >= 4) {
            g_cmd.cmd_linear_mm_s = (int16_t)(payload[0] | (payload[1] << 8));
            g_cmd.cmd_angular_mrad_s = (int16_t)(payload[2] | (payload[3] << 8));
        }
        break;
    case PROTO_CMD_HEARTBEAT:
        break;
    case PROTO_CMD_ARM_SEQ:
        if (len >= 1) {
            g_cmd.arm_seq_req = payload[0];
        }
        break;
    case PROTO_CMD_ARM_JOINT:
        /* 单关节调试由 app 层读原始帧时可扩展；此处忽略 */
        break;
    case PROTO_CMD_VOICE:
        if (len >= 1) {
            g_cmd.voice_index_req = payload[0];
        }
        break;
    default:
        break;
    }
}

void comm_poll_rx(void)
{
    uint8_t tmp[64];
    int n = board_host_uart_read(tmp, sizeof(tmp));
    if (n <= 0) {
        return;
    }

    for (int i = 0; i < n; i++) {
        if (g_rx_len == 0 && tmp[i] != PROTO_HEADER0) {
            continue;
        }
        if (g_rx_len == 1 && tmp[i] != PROTO_HEADER1) {
            g_rx_len = (tmp[i] == PROTO_HEADER0) ? 1u : 0u;
            if (g_rx_len) {
                g_rx[0] = PROTO_HEADER0;
            }
            continue;
        }
        if (g_rx_len < sizeof(g_rx)) {
            g_rx[g_rx_len++] = tmp[i];
        } else {
            g_rx_len = 0;
            continue;
        }

        if (g_rx_len >= 4) {
            uint8_t len = g_rx[3];
            uint16_t total = (uint16_t)(4u + len + 1u);
            if (g_rx_len < total) {
                continue;
            }
            uint8_t crc = proto_crc8(&g_rx[2], (uint16_t)(2u + len));
            if (crc == g_rx[total - 1u]) {
                handle_frame(g_rx[2], &g_rx[4], len);
            }
            g_rx_len = 0;
        }
    }
}

static void send_raw(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[64];
    if ((uint16_t)len + 5u > sizeof(frame)) {
        return;
    }
    uint16_t n = proto_pack(cmd, payload, len, frame);
    board_host_uart_write(frame, n);
}

void comm_send_odom(const proto_odom_t *o)
{
    send_raw(PROTO_CMD_ODOM, (const uint8_t *)o, (uint8_t)sizeof(*o));
}

void comm_send_line(const proto_line_t *l)
{
    send_raw(PROTO_CMD_LINE, (const uint8_t *)l, (uint8_t)sizeof(*l));
}

void comm_send_obstacle(const proto_obstacle_t *o)
{
    send_raw(PROTO_CMD_OBSTACLE, (const uint8_t *)o, (uint8_t)sizeof(*o));
}

void comm_send_arm_state(const proto_arm_state_t *a)
{
    send_raw(PROTO_CMD_ARM_STATE, (const uint8_t *)a, (uint8_t)sizeof(*a));
}

void comm_send_voice_done(uint8_t index)
{
    send_raw(PROTO_CMD_VOICE_DONE, &index, 1);
}
