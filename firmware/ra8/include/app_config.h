#pragma once

/* 可按实车修改的参数；引脚映射在 board_hal 中对接 FSP */

#define APP_TICK_HZ                 1000u
#define APP_HEARTBEAT_TIMEOUT_MS    500u

#define MOTOR_CTRL_HZ               1000u
#define LINE_CTRL_HZ                100u
#define OBSTACLE_CTRL_HZ            50u
#define TELEMETRY_ODOM_HZ           50u
#define TELEMETRY_LINE_HZ           20u
#define TELEMETRY_OBS_HZ            20u

#define WHEEL_TRACK_MM              150.0f
#define TICKS_PER_METER             4000.0f   /* 标定后改 */
#define MAX_LINEAR_MM_S             400
#define MAX_ANGULAR_MRAD_S          2000
#define ACCEL_MM_S2                 800.0f

#define LINE_SENSOR_COUNT           8u
#define LINE_SENSOR_SPACING_MM      8.0f
#define LINE_KP                     1.2f
#define LINE_KD                     0.08f
#define LINE_BASE_SPEED_MM_S        180

#define OBS_STOP_MM                 120u
#define OBS_SLOW_MM                 250u
#define OBS_CLEAR_MM                300u

#define ARM_JOINT_COUNT             4u
#define VOICE_UART_BAUD             9600u
#define HOST_UART_BAUD              115200u
