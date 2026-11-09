#include "obstacle.h"
#include "app_config.h"
#include "board_hal.h"

static proto_obstacle_t g_st;
static obs_action_t g_action;

void obstacle_init(void)
{
    g_st.front_mm = 0xFFFF;
    g_st.left_mm = 0xFFFF;
    g_st.right_mm = 0xFFFF;
    g_st.state = OBS_OK;
    g_action = OBS_OK;
}

void obstacle_update(void)
{
    g_st.front_mm = board_ultrasonic_front_mm();
    g_st.left_mm = board_ultrasonic_left_mm();
    g_st.right_mm = board_ultrasonic_right_mm();

    uint16_t f = g_st.front_mm;
    if (f == 0xFFFF) {
        g_action = OBS_OK;
    } else if (f < OBS_STOP_MM) {
        g_action = OBS_STOP;
    } else if (f < OBS_SLOW_MM) {
        g_action = OBS_AVOID;
    } else if (f < OBS_CLEAR_MM) {
        g_action = OBS_SLOW;
    } else {
        g_action = OBS_OK;
    }
    g_st.state = (uint8_t)g_action;
}

void obstacle_get_status(proto_obstacle_t *out)
{
    *out = g_st;
}

obs_action_t obstacle_action(void)
{
    return g_action;
}

void obstacle_apply(int16_t *linear_mm_s, int16_t *angular_mrad_s)
{
    switch (g_action) {
    case OBS_STOP:
        *linear_mm_s = 0;
        *angular_mrad_s = 0;
        break;
    case OBS_SLOW:
        if (*linear_mm_s > 80) {
            *linear_mm_s = 80;
        }
        break;
    case OBS_AVOID: {
        /* 往距离更大的一侧绕 */
        int16_t turn = (g_st.left_mm > g_st.right_mm) ? 900 : -900;
        *linear_mm_s = 60;
        *angular_mrad_s = turn;
        break;
    }
    case OBS_OK:
    default:
        break;
    }
}
