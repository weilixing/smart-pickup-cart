#include "line_follow.h"
#include "app_config.h"
#include "board_hal.h"

static proto_line_t g_status;
static int16_t g_lin;
static int16_t g_ang;
static float g_last_err;

void line_init(void)
{
    g_status.error_mm = 0;
    g_status.on_line = 0;
    g_status.station = 0;
    g_lin = 0;
    g_ang = 0;
    g_last_err = 0;
}

void line_update(void)
{
    uint16_t s[LINE_SENSOR_COUNT];
    board_line_read(s);

    float sum = 0;
    float weighted = 0;
    uint8_t active = 0;
    const float mid = 0.5f * (float)(LINE_SENSOR_COUNT - 1u);

    for (uint8_t i = 0; i < LINE_SENSOR_COUNT; i++) {
        /* 阈值：认为读数 > 500 为黑线，可按实车改 */
        if (s[i] > 500u) {
            float pos = ((float)i - mid) * LINE_SENSOR_SPACING_MM;
            weighted += pos * (float)s[i];
            sum += (float)s[i];
            active++;
        }
    }

    if (active == 0 || sum < 1.0f) {
        g_status.on_line = 0;
        g_status.error_mm = (int16_t)g_last_err; /* 丢线时保持上次偏差方向，便于回线 */
        g_lin = 0;
        g_ang = (int16_t)(g_last_err > 0 ? 600 : -600);
        return;
    }

    float err = weighted / sum; /* mm，左负右正约定可按实车改符号 */
    float derr = err - g_last_err;
    g_last_err = err;

    g_status.on_line = 1;
    g_status.error_mm = (int16_t)err;

    /* 站点检测占位：全部传感器都黑且偏差很小，由上层或后续加色标/RFID 完善 */
    g_status.station = 0;

    float w = LINE_KP * err + LINE_KD * derr * (float)LINE_CTRL_HZ;
    g_lin = (int16_t)LINE_BASE_SPEED_MM_S;
    g_ang = (int16_t)(w * 10.0f); /* 粗映射到 mrad/s，联调时再标定 */
}

void line_get_status(proto_line_t *out)
{
    *out = g_status;
}

void line_get_cmd(int16_t *linear_mm_s, int16_t *angular_mrad_s)
{
    *linear_mm_s = g_lin;
    *angular_mrad_s = g_ang;
}

bool line_on_line(void)
{
    return g_status.on_line != 0;
}
