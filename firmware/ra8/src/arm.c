#include "arm.h"
#include "app_config.h"
#include "board_hal.h"
#include <stddef.h>

typedef struct {
    float angles[ARM_JOINT_COUNT];
    uint16_t dwell_ms;
} arm_pose_t;

/* 占位角度，装机后用示教值替换 */
static const arm_pose_t SEQ_HOME[] = {
    {{90, 90, 90, 20}, 300},
};
static const arm_pose_t SEQ_PICK[] = {
    {{90, 60, 120, 20}, 400},
    {{90, 40, 140, 20}, 400},
    {{90, 40, 140, 80}, 400},
    {{90, 70, 100, 80}, 400},
};
static const arm_pose_t SEQ_PLACE[] = {
    {{90, 60, 120, 80}, 400},
    {{90, 40, 140, 80}, 400},
    {{90, 40, 140, 20}, 400},
    {{90, 90, 90, 20}, 300},
};

typedef struct {
    const arm_pose_t *poses;
    uint8_t count;
    uint8_t index;
    uint32_t since_ms;
    uint8_t busy;
    uint8_t last_seq;
    uint8_t ok;
} arm_t;

static arm_t g;

void arm_init(void)
{
    g.poses = NULL;
    g.count = 0;
    g.index = 0;
    g.busy = 0;
    g.last_seq = 0;
    g.ok = 1;
    for (uint8_t i = 0; i < ARM_JOINT_COUNT; i++) {
        board_servo_set_deg(i, 90.0f);
    }
}

static void start_seq(const arm_pose_t *poses, uint8_t count, uint8_t seq_id)
{
    g.poses = poses;
    g.count = count;
    g.index = 0;
    g.since_ms = board_millis();
    g.busy = 1;
    g.last_seq = seq_id;
    g.ok = 1;
    if (count > 0) {
        for (uint8_t j = 0; j < ARM_JOINT_COUNT; j++) {
            board_servo_set_deg(j, poses[0].angles[j]);
        }
    }
}

void arm_request_seq(uint8_t seq_id)
{
    switch (seq_id) {
    case 0:
        start_seq(SEQ_HOME, (uint8_t)(sizeof(SEQ_HOME) / sizeof(SEQ_HOME[0])), 0);
        break;
    case 1:
        start_seq(SEQ_PICK, (uint8_t)(sizeof(SEQ_PICK) / sizeof(SEQ_PICK[0])), 1);
        break;
    case 2:
        start_seq(SEQ_PLACE, (uint8_t)(sizeof(SEQ_PLACE) / sizeof(SEQ_PLACE[0])), 2);
        break;
    default:
        g.ok = 0;
        break;
    }
}

void arm_set_joint_cdeg(uint8_t joint, uint16_t angle_cdeg)
{
    if (joint >= ARM_JOINT_COUNT) {
        return;
    }
    board_servo_set_deg(joint, (float)angle_cdeg / 10.0f);
}

void arm_update(void)
{
    if (!g.busy || g.poses == NULL) {
        return;
    }
    uint32_t now = board_millis();
    if ((now - g.since_ms) < g.poses[g.index].dwell_ms) {
        return;
    }
    g.index++;
    if (g.index >= g.count) {
        g.busy = 0;
        g.poses = NULL;
        return;
    }
    g.since_ms = now;
    for (uint8_t j = 0; j < ARM_JOINT_COUNT; j++) {
        board_servo_set_deg(j, g.poses[g.index].angles[j]);
    }
}

void arm_get_state(proto_arm_state_t *out)
{
    out->busy = g.busy;
    out->last_seq = g.last_seq;
    out->ok = g.ok;
}

bool arm_busy(void)
{
    return g.busy != 0;
}
