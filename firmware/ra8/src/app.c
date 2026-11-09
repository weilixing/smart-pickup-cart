#include "app.h"
#include "app_config.h"
#include "board_hal.h"
#include "comm.h"
#include "motor.h"
#include "line_follow.h"
#include "obstacle.h"
#include "arm.h"
#include "voice.h"

#include <stdbool.h>

static uint32_t g_last_1khz;
static uint32_t g_last_line;
static uint32_t g_last_obs;
static uint32_t g_last_odom_tx;
static uint32_t g_last_line_tx;
static uint32_t g_last_obs_tx;
static uint32_t g_last_arm_tx;

void app_init(void)
{
    board_init();
    motor_init();
    line_init();
    obstacle_init();
    arm_init();
    voice_init();
    comm_init();

    uint32_t now = board_millis();
    g_last_1khz = now;
    g_last_line = now;
    g_last_obs = now;
    g_last_odom_tx = now;
    g_last_line_tx = now;
    g_last_obs_tx = now;
    g_last_arm_tx = now;
}

void app_loop(void)
{
    comm_poll_rx();
    const host_cmd_t *cmd = comm_cmd();
    uint32_t now = board_millis();
    const bool link_alive =
        (now - cmd->last_rx_ms) <= APP_HEARTBEAT_TIMEOUT_MS;
    const bool force_stop = (!link_alive) || (cmd->mode == 3u);

    if (cmd->arm_seq_req != 0xFF) {
        arm_request_seq(cmd->arm_seq_req);
        comm_clear_arm_req();
    }
    if (cmd->voice_index_req != 0xFF) {
        voice_play_index(cmd->voice_index_req);
        comm_clear_voice_req();
    }

    if ((now - g_last_1khz) >= (1000u / MOTOR_CTRL_HZ)) {
        g_last_1khz = now;

        int16_t lin = 0;
        int16_t ang = 0;
        if (!force_stop && cmd->mode == 1) {
            lin = cmd->cmd_linear_mm_s;
            ang = cmd->cmd_angular_mrad_s;
        } else if (!force_stop && cmd->mode == 2) {
            line_get_cmd(&lin, &ang);
            obstacle_apply(&lin, &ang);
        }

        if (force_stop) {
            motor_estop();
        } else {
            motor_clear_estop();
            motor_set_target(lin, ang);
        }
        motor_update_1khz();
    }

    if ((now - g_last_line) >= (1000u / LINE_CTRL_HZ)) {
        g_last_line = now;
        if (cmd->mode == 2) {
            line_update();
        }
    }

    if ((now - g_last_obs) >= (1000u / OBSTACLE_CTRL_HZ)) {
        g_last_obs = now;
        if (cmd->mode == 2) {
            obstacle_update();
        }
    }

    arm_update();
    voice_update();

    if ((now - g_last_odom_tx) >= (1000u / TELEMETRY_ODOM_HZ)) {
        g_last_odom_tx = now;
        proto_odom_t o;
        motor_get_odom(&o);
        comm_send_odom(&o);
    }
    if ((now - g_last_line_tx) >= (1000u / TELEMETRY_LINE_HZ)) {
        g_last_line_tx = now;
        proto_line_t l;
        line_get_status(&l);
        comm_send_line(&l);
    }
    if ((now - g_last_obs_tx) >= (1000u / TELEMETRY_OBS_HZ)) {
        g_last_obs_tx = now;
        proto_obstacle_t o;
        obstacle_get_status(&o);
        comm_send_obstacle(&o);
    }
    if ((now - g_last_arm_tx) >= 50u) {
        g_last_arm_tx = now;
        proto_arm_state_t a;
        arm_get_state(&a);
        comm_send_arm_state(&a);
    }

    uint8_t done = voice_last_done_index();
    if (done != 0xFF) {
        comm_send_voice_done(done);
    }
}
