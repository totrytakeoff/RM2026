/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Infantry ET08 minimal gimbal demo
 ******************************************************************************
 * @attention
 *
 * 仅保留云台测试链路：
 * ET08(SBUS) -> yaw/pitch指令 -> GM6020 速度/位置双模式控制
 *
 * - SBUS 输入: USART3
 * - 调试输出: USART6
 * - 云台电机: Yaw(CAN1 ID1), Pitch(CAN2 ID1)
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "cmsis_os.h"
#include "crc.h"
#include "dac.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "dji_motor.h"
#include "et08_remote.h"
#include "ins_task.h"
#include "minimal_config.h"

/* Private define ------------------------------------------------------------*/
#define TELEMETRY_RX_DUMMY 32U
#define TELEMETRY_PERIOD_MS 100U
#define VOFA_PERIOD_MS 20U
#define INS_TASK_PERIOD_MS 1U
#define DAEMON_TASK_PERIOD_MS 1U
#define MOTOR_CONTROL_PERIOD_MS 2U
#define RC_INPUT_DEADZONE 50
#define RC_INPUT_ENTER_DEADZONE 180
#define RC_INPUT_EXIT_DEADZONE 90
#define RC_INPUT_ENTER_COUNT 4
#define RC_INPUT_EXIT_COUNT 10
#define HOLD_REF_UPDATE_MIN_SPEED 300.0f
#define HOLD_REF_UPDATE_MIN_RAW 220
#define PITCH_BRAKE_SPEED_EPS 20.0f
#define PITCH_BRAKE_STABLE_COUNT 3U
#define PITCH_BRAKE_TIMEOUT_MS 120U
#define GM6020_SPEED_MIN (-GM6020_SPEED_MAX)
#define ET08_SWITCH_POS_UP 0U
#define ET08_SWITCH_POS_DOWN 2U

/* Private typedef -----------------------------------------------------------*/
typedef enum {
    GIMBAL_MODE_FOLLOW = 0,
    GIMBAL_MODE_SEPARATE = 1,
} GimbalMode_t;

typedef enum {
    AXIS_CTRL_ANGLE = 0,
    AXIS_CTRL_SPEED = 1,
    AXIS_CTRL_BRAKE = 2,
} AxisCtrlMode_t;

/* Private variables ---------------------------------------------------------*/
static ET08_Ctrl_t *et08_ctrl = NULL;
static USARTInstance *telemetry_usart = NULL;
static attitude_t *gimbal_imu = NULL;

static DJIMotorInstance *motor_yaw = NULL;
static DJIMotorInstance *motor_pitch = NULL;

static uint8_t gimbal_enabled = 0U;
static uint8_t last_online_state = 0U;

static GimbalMode_t gimbal_mode = GIMBAL_MODE_FOLLOW;
static float yaw_speed_ref = 0.0f;
static float pitch_speed_ref = 0.0f;
static float yaw_hold_ref = 0.0f;
static float pitch_hold_ref = 0.0f;
static float yaw_separate_center = 0.0f;
static AxisCtrlMode_t yaw_ctrl_mode = AXIS_CTRL_ANGLE;
static AxisCtrlMode_t pitch_ctrl_mode = AXIS_CTRL_ANGLE;
static GimbalMode_t last_gimbal_mode = GIMBAL_MODE_FOLLOW;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

static void Debug_DisableWatchdogs(void);
static void InitTelemetryUsart(void);
static void TelemetrySendBuffer(const uint8_t *buffer, uint16_t len);
static void TelemetrySendString(const char *str);
static void TelemetryPrintf(const char *fmt, ...);

static int16_t ApplyDeadzone(int16_t v, int16_t deadzone);
static float ClampFloat(float value, float min_value, float max_value);
static float GetYawHoldFeedbackAngle(void);
static float LimitYawSeparateHoldRef(float ref);
static void ApplyYawPidProfile(GimbalMode_t mode);
static float PidSatRatio(const PIDInstance *pid);
static uint8_t PidNearlySaturated(const PIDInstance *pid);
static const char *AxisCtrlModeName(AxisCtrlMode_t mode);
static uint8_t ET08_MapUpperSwitchPos(uint8_t state);

static void GimbalMotorsInit(void);
static void GimbalStop(void);
static bool GimbalUpdateFromET08(void);
static void SendTelemetry(void);
static void SendVofaFrame(void);
static uint8_t UpdateStickActiveState(int16_t raw, uint8_t prev_state, uint8_t *enter_cnt, uint8_t *exit_cnt);

/* Private user code ---------------------------------------------------------*/
static void Debug_DisableWatchdogs(void)
{
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}

static void InitTelemetryUsart(void)
{
    USART_Init_Config_s config = {
        .module_callback = NULL,
        .recv_buff_size = TELEMETRY_RX_DUMMY,
        .usart_handle = &huart6,
    };
    telemetry_usart = USARTRegister(&config);
    TelemetrySendString("[gmb_et08] USART6 telemetry ready\r\n");
}

static void TelemetrySendBuffer(const uint8_t *buffer, uint16_t len)
{
    if (telemetry_usart == NULL || buffer == NULL || len == 0U)
        return;

    USARTSend(telemetry_usart, (uint8_t *)buffer, len, USART_TRANSFER_BLOCKING);
}

static void TelemetrySendString(const char *str)
{
    if (str == NULL)
        return;
    TelemetrySendBuffer((const uint8_t *)str, (uint16_t)strlen(str));
}

static void TelemetryPrintf(const char *fmt, ...)
{
    if (telemetry_usart == NULL || fmt == NULL)
        return;

    char msg[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (n <= 0)
        return;

    uint16_t len = (n < (int)sizeof(msg)) ? (uint16_t)n : (uint16_t)(sizeof(msg) - 1U);
    TelemetrySendBuffer((const uint8_t *)msg, len);
}

static int16_t ApplyDeadzone(int16_t v, int16_t deadzone)
{
    if (v > -deadzone && v < deadzone) {
        return 0;
    }
    return v;
}

static float ClampFloat(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float GetYawHoldFeedbackAngle(void)
{
    if (gimbal_mode == GIMBAL_MODE_SEPARATE && gimbal_imu != NULL) {
        return gimbal_imu->YawTotalAngle;
    }
    return (motor_yaw != NULL) ? motor_yaw->measure.total_angle : 0.0f;
}

static float LimitYawSeparateHoldRef(float ref)
{
    if (gimbal_mode == GIMBAL_MODE_SEPARATE && gimbal_imu != NULL) {
        return ClampFloat(ref,
                          yaw_separate_center - GIMBAL_SEPARATE_YAW_MAX_ANGLE,
                          yaw_separate_center + GIMBAL_SEPARATE_YAW_MAX_ANGLE);
    }
    return ref;
}

static void ApplyYawPidProfile(GimbalMode_t mode)
{
    PID_Init_Config_s angle_cfg = {
        .Kp = YAW_FOLLOW_ANGLE_KP,
        .Ki = YAW_FOLLOW_ANGLE_KI,
        .Kd = YAW_FOLLOW_ANGLE_KD,
        .IntegralLimit = 500.0f,
        .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
        .MaxOut = YAW_FOLLOW_ANGLE_MAX_OUT,
    };
    PID_Init_Config_s speed_cfg = {
        .Kp = YAW_FOLLOW_SPEED_KP,
        .Ki = YAW_FOLLOW_SPEED_KI,
        .Kd = YAW_FOLLOW_SPEED_KD,
        .IntegralLimit = 1000.0f,
        .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
        .MaxOut = YAW_FOLLOW_SPEED_MAX_OUT,
    };

    if (motor_yaw == NULL) {
        return;
    }

    if (mode == GIMBAL_MODE_SEPARATE) {
        angle_cfg.Kp = YAW_SEPARATE_ANGLE_KP;
        angle_cfg.Ki = YAW_SEPARATE_ANGLE_KI;
        angle_cfg.Kd = YAW_SEPARATE_ANGLE_KD;
        angle_cfg.MaxOut = YAW_SEPARATE_ANGLE_MAX_OUT;

        speed_cfg.Kp = YAW_SEPARATE_SPEED_KP;
        speed_cfg.Ki = YAW_SEPARATE_SPEED_KI;
        speed_cfg.Kd = YAW_SEPARATE_SPEED_KD;
        speed_cfg.MaxOut = YAW_SEPARATE_SPEED_MAX_OUT;
    }

    PIDInit(&motor_yaw->motor_controller.angle_PID, &angle_cfg);
    PIDInit(&motor_yaw->motor_controller.speed_PID, &speed_cfg);
}

static float PidSatRatio(const PIDInstance *pid)
{
    if (pid == NULL || pid->MaxOut <= 1e-6f) {
        return 0.0f;
    }
    return fabsf(pid->Output) / pid->MaxOut;
}

static uint8_t PidNearlySaturated(const PIDInstance *pid)
{
    return (PidSatRatio(pid) >= 0.98f) ? 1U : 0U;
}

static const char *AxisCtrlModeName(AxisCtrlMode_t mode)
{
    switch (mode) {
    case AXIS_CTRL_SPEED:
        return "SPEED";
    case AXIS_CTRL_BRAKE:
        return "BRAKE";
    case AXIS_CTRL_ANGLE:
    default:
        return "ANGLE";
    }
}

static uint8_t ET08_MapUpperSwitchPos(uint8_t state)
{
    if (state == 0xFFU || state > 5U) {
        return 0xFFU;
    }
    return (state <= 2U) ? ET08_SWITCH_POS_UP : ET08_SWITCH_POS_DOWN;
}

static uint8_t UpdateStickActiveState(int16_t raw, uint8_t prev_state, uint8_t *enter_cnt, uint8_t *exit_cnt)
{
    int16_t abs_raw = (raw >= 0) ? raw : (int16_t)(-raw);
    if (enter_cnt == NULL || exit_cnt == NULL) {
        return prev_state;
    }
    if (prev_state != 0U) {
        if (abs_raw <= RC_INPUT_EXIT_DEADZONE) {
            if (*exit_cnt < 255U) {
                (*exit_cnt)++;
            }
        } else {
            *exit_cnt = 0U;
        }
        *enter_cnt = 0U;
        if (*exit_cnt >= RC_INPUT_EXIT_COUNT) {
            *exit_cnt = 0U;
            return 0U;
        }
        return 1U;
    }
    if (abs_raw >= RC_INPUT_ENTER_DEADZONE) {
        if (*enter_cnt < 255U) {
            (*enter_cnt)++;
        }
    } else {
        *enter_cnt = 0U;
    }
    *exit_cnt = 0U;
    if (*enter_cnt >= RC_INPUT_ENTER_COUNT) {
        *enter_cnt = 0U;
        return 1U;
    }
    return 0U;
}

static void GimbalMotorsInit(void)
{
    Motor_Init_Config_s yaw_config = {
        .motor_type = GM6020,
        .can_init_config = {
            .can_handle = &YAW_CAN,
            .tx_id = YAW_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = YAW_FOLLOW_ANGLE_KP,
                .Ki = YAW_FOLLOW_ANGLE_KI,
                .Kd = YAW_FOLLOW_ANGLE_KD,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_FOLLOW_ANGLE_MAX_OUT,
            },
            .speed_PID = {
                .Kp = YAW_FOLLOW_SPEED_KP,
                .Ki = YAW_FOLLOW_SPEED_KI,
                .Kd = YAW_FOLLOW_SPEED_KD,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_FOLLOW_SPEED_MAX_OUT,
            },
            .other_angle_feedback_ptr = (gimbal_imu != NULL) ? &gimbal_imu->YawTotalAngle : NULL,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    Motor_Init_Config_s pitch_config = {
        .motor_type = GM6020,
        .can_init_config = {
            .can_handle = &PITCH_CAN,
            .tx_id = PITCH_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = PITCH_ANGLE_KP,
                .Ki = PITCH_ANGLE_KI,
                .Kd = PITCH_ANGLE_KD,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = PITCH_ANGLE_MAX_OUT,
            },
            .speed_PID = {
                .Kp = PITCH_SPEED_KP,
                .Ki = PITCH_SPEED_KI,
                .Kd = PITCH_SPEED_KD,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = PITCH_SPEED_MAX_OUT,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    motor_yaw = DJIMotorInit(&yaw_config);
    motor_pitch = DJIMotorInit(&pitch_config);

    if (motor_yaw) {
        DJIMotorOuterLoop(motor_yaw, SPEED_LOOP);
        DJIMotorStop(motor_yaw);
        ApplyYawPidProfile(GIMBAL_MODE_FOLLOW);
        yaw_hold_ref = GetYawHoldFeedbackAngle();
        yaw_separate_center = yaw_hold_ref;
    }
    if (motor_pitch) {
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorStop(motor_pitch);
        pitch_hold_ref = motor_pitch->measure.total_angle;
    }

    TelemetrySendString("[gmb_et08] gimbal motors initialized\r\n");
}

static void GimbalStop(void)
{
    yaw_speed_ref = 0.0f;
    pitch_speed_ref = 0.0f;
    yaw_ctrl_mode = AXIS_CTRL_ANGLE;
    pitch_ctrl_mode = AXIS_CTRL_ANGLE;
    gimbal_enabled = 0U;

    if (motor_yaw) {
        DJIMotorStop(motor_yaw);
    }
    if (motor_pitch) {
        DJIMotorStop(motor_pitch);
    }
}

static bool GimbalUpdateFromET08(void)
{
    static AxisCtrlMode_t last_yaw_ctrl_mode = AXIS_CTRL_ANGLE;
    static AxisCtrlMode_t last_pitch_ctrl_mode = AXIS_CTRL_ANGLE;
    static uint8_t yaw_cmd_active = 0U;
    static uint8_t pitch_cmd_active = 0U;
    static uint8_t yaw_enter_cnt = 0U;
    static uint8_t yaw_exit_cnt = 0U;
    static uint8_t pitch_enter_cnt = 0U;
    static uint8_t pitch_exit_cnt = 0U;
    static uint8_t yaw_brake_stable_count = 0U;
    static uint32_t yaw_brake_start_ms = 0U;
    static uint8_t pitch_brake_stable_count = 0U;
    static uint32_t pitch_brake_start_ms = 0U;

    if (et08_ctrl == NULL || motor_yaw == NULL || motor_pitch == NULL) {
        return false;
    }

    if (!ET08_IsOnline() || et08_ctrl->failsafe || et08_ctrl->frame_lost) {
        return false;
    }

    if (!gimbal_enabled) {
        DJIMotorEnable(motor_yaw);
        DJIMotorEnable(motor_pitch);
        gimbal_enabled = 1U;
        TelemetryPrintf("[gmb_et08] motors re-enabled hold(y_x10=%ld p_x10=%ld)\r\n",
                        (long)(yaw_hold_ref * 10.0f),
                        (long)(pitch_hold_ref * 10.0f));
    }

    const int16_t yaw_raw = et08_ctrl->right.x;
    const int16_t pitch_raw = et08_ctrl->right.y;
    const int16_t yaw_in = ApplyDeadzone(yaw_raw, RC_INPUT_DEADZONE);
    const int16_t pitch_in = ApplyDeadzone(pitch_raw, RC_INPUT_DEADZONE);
    const int16_t yaw_abs = (yaw_in >= 0) ? yaw_in : (int16_t)(-yaw_in);
    const int16_t pitch_abs = (pitch_in >= 0) ? pitch_in : (int16_t)(-pitch_in);
    const uint32_t now_ms = DWT_GetTimeline_ms();

    uint8_t sd_pos = ET08_MapUpperSwitchPos(et08_ctrl->switch_sd_sc_state);
    if (sd_pos == ET08_SWITCH_POS_UP) {
        gimbal_mode = GIMBAL_MODE_FOLLOW;
    } else {
        gimbal_mode = GIMBAL_MODE_SEPARATE;
    }
    if (gimbal_mode != last_gimbal_mode) {
        ApplyYawPidProfile(gimbal_mode);
        if (gimbal_mode == GIMBAL_MODE_SEPARATE && gimbal_imu != NULL) {
            yaw_separate_center = gimbal_imu->YawTotalAngle;
        }
        yaw_hold_ref = LimitYawSeparateHoldRef(GetYawHoldFeedbackAngle());
        TelemetryPrintf("[gmb_et08] mode switch -> %s\r\n",
                        (gimbal_mode == GIMBAL_MODE_FOLLOW) ? "FOLLOW" : "SEPARATE");
        TelemetryPrintf("[gmb_et08] yaw pid profile -> %s angle(kp_x100=%ld ki_x100=%ld kd_x100=%ld) speed(kp_x100=%ld ki_x100=%ld kd_x100=%ld)\r\n",
                        (gimbal_mode == GIMBAL_MODE_FOLLOW) ? "FOLLOW" : "SEPARATE",
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_ANGLE_KP : YAW_SEPARATE_ANGLE_KP) * 100.0f),
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_ANGLE_KI : YAW_SEPARATE_ANGLE_KI) * 100.0f),
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_ANGLE_KD : YAW_SEPARATE_ANGLE_KD) * 100.0f),
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_SPEED_KP : YAW_SEPARATE_SPEED_KP) * 100.0f),
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_SPEED_KI : YAW_SEPARATE_SPEED_KI) * 100.0f),
                        (long)(((gimbal_mode == GIMBAL_MODE_FOLLOW) ? YAW_FOLLOW_SPEED_KD : YAW_SEPARATE_SPEED_KD) * 100.0f));
        TelemetryPrintf("[gmb_et08] yaw mode lock hold_x10=%ld enc_x10=%ld imu_yaw_total_x10=%ld\r\n",
                        (long)(yaw_hold_ref * 10.0f),
                        (long)((motor_yaw != NULL) ? (motor_yaw->measure.total_angle * 10.0f) : 0.0f),
                        (long)((gimbal_imu != NULL) ? (gimbal_imu->YawTotalAngle * 10.0f) : 0.0f));
        if (gimbal_mode == GIMBAL_MODE_SEPARATE && gimbal_imu != NULL) {
            TelemetryPrintf("[gmb_et08] separate center hold_x10=%ld limit_x10=%ld\r\n",
                            (long)(yaw_separate_center * 10.0f),
                            (long)(GIMBAL_SEPARATE_YAW_MAX_ANGLE * 10.0f));
        }
        last_gimbal_mode = gimbal_mode;
    }

    {
        float yaw_ratio = ClampFloat((float)yaw_in / RC_STICK_SCALE, -1.0f, 1.0f);
        yaw_speed_ref = yaw_ratio * GM6020_SPEED_MAX * YAW_SPEED_SCALE;
        yaw_speed_ref = ClampFloat(yaw_speed_ref, GM6020_SPEED_MIN, GM6020_SPEED_MAX);
    }
    if (fabsf(yaw_speed_ref) < GIMBAL_SPEED_DEADZONE) {
        yaw_speed_ref = 0.0f;
    }

    {
        float pitch_ratio = ClampFloat((float)pitch_in / RC_STICK_SCALE, -1.0f, 1.0f);
        pitch_speed_ref = pitch_ratio * GM6020_SPEED_MAX * PITCH_SPEED_SCALE;
        if (fabsf(pitch_speed_ref) < GIMBAL_SPEED_DEADZONE) {
            pitch_speed_ref = 0.0f;
        }
    }
    float pitch_speed_cmd = ClampFloat(pitch_speed_ref, GM6020_SPEED_MIN, GM6020_SPEED_MAX);
    yaw_cmd_active = UpdateStickActiveState(yaw_in, yaw_cmd_active, &yaw_enter_cnt, &yaw_exit_cnt);
    pitch_cmd_active = UpdateStickActiveState(pitch_in, pitch_cmd_active, &pitch_enter_cnt, &pitch_exit_cnt);
    if (yaw_cmd_active) {
        yaw_ctrl_mode = AXIS_CTRL_SPEED;
        yaw_brake_stable_count = 0U;
        yaw_brake_start_ms = 0U;
    } else if (yaw_ctrl_mode == AXIS_CTRL_SPEED) {
        yaw_hold_ref = LimitYawSeparateHoldRef(GetYawHoldFeedbackAngle());
        yaw_ctrl_mode = AXIS_CTRL_BRAKE;
        yaw_brake_stable_count = 0U;
        yaw_brake_start_ms = now_ms;
        TelemetryPrintf("[gmb_et08] yaw speed -> brake lock_x10=%ld spd_x10=%ld\r\n",
                        (long)(yaw_hold_ref * 10.0f),
                        (long)(motor_yaw->measure.speed_aps * 10.0f));
    } else if (yaw_ctrl_mode == AXIS_CTRL_BRAKE) {
        yaw_hold_ref = LimitYawSeparateHoldRef(GetYawHoldFeedbackAngle());
        if (fabsf(motor_yaw->measure.speed_aps) <= YAW_BRAKE_SPEED_EPS) {
            if (yaw_brake_stable_count < 255U) {
                yaw_brake_stable_count++;
            }
        } else {
            yaw_brake_stable_count = 0U;
        }
        if (yaw_brake_stable_count >= YAW_BRAKE_STABLE_COUNT ||
            (now_ms - yaw_brake_start_ms) >= YAW_BRAKE_TIMEOUT_MS) {
            yaw_hold_ref = LimitYawSeparateHoldRef(GetYawHoldFeedbackAngle());
            yaw_ctrl_mode = AXIS_CTRL_ANGLE;
            TelemetryPrintf("[gmb_et08] yaw brake -> hold lock_x10=%ld spd_x10=%ld\r\n",
                            (long)(yaw_hold_ref * 10.0f),
                            (long)(motor_yaw->measure.speed_aps * 10.0f));
        }
    }

    if (yaw_ctrl_mode == AXIS_CTRL_SPEED) {
        DJIMotorOuterLoop(motor_yaw, SPEED_LOOP);
        DJIMotorSetRef(motor_yaw, yaw_speed_ref);
        yaw_hold_ref = LimitYawSeparateHoldRef(GetYawHoldFeedbackAngle());
    } else if (yaw_ctrl_mode == AXIS_CTRL_BRAKE) {
        DJIMotorOuterLoop(motor_yaw, SPEED_LOOP);
        DJIMotorSetRef(motor_yaw, 0.0f);
    } else {
        yaw_hold_ref = LimitYawSeparateHoldRef(yaw_hold_ref);
        if (last_yaw_ctrl_mode != AXIS_CTRL_ANGLE) {
            TelemetryPrintf("[gmb_et08] yaw -> hold lock_x10=%ld now_x10=%ld\r\n",
                            (long)(yaw_hold_ref * 10.0f),
                            (long)(motor_yaw->measure.total_angle * 10.0f));
        }
        DJIMotorChangeFeed(motor_yaw,
                           ANGLE_LOOP,
                           (gimbal_mode == GIMBAL_MODE_SEPARATE && gimbal_imu != NULL) ? OTHER_FEED : MOTOR_FEED);
        DJIMotorOuterLoop(motor_yaw, ANGLE_LOOP);
        DJIMotorSetRef(motor_yaw, yaw_hold_ref);
    }

    if (pitch_cmd_active) {
        pitch_ctrl_mode = AXIS_CTRL_SPEED;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = 0U;
    } else if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        pitch_ctrl_mode = AXIS_CTRL_BRAKE;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = now_ms;
    } else if (pitch_ctrl_mode == AXIS_CTRL_BRAKE) {
        if (fabsf(motor_pitch->measure.speed_aps) <= PITCH_BRAKE_SPEED_EPS) {
            if (pitch_brake_stable_count < 255U) {
                pitch_brake_stable_count++;
            }
        } else {
            pitch_brake_stable_count = 0U;
        }
        if (pitch_brake_stable_count >= PITCH_BRAKE_STABLE_COUNT ||
            (now_ms - pitch_brake_start_ms) >= PITCH_BRAKE_TIMEOUT_MS) {
            pitch_hold_ref = motor_pitch->measure.total_angle;
            pitch_ctrl_mode = AXIS_CTRL_ANGLE;
            TelemetryPrintf("[gmb_et08] pitch brake -> hold lock_x10=%ld spd_x10=%ld\r\n",
                            (long)(pitch_hold_ref * 10.0f),
                            (long)(motor_pitch->measure.speed_aps * 10.0f));
        }
    }

    if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorSetRef(motor_pitch, pitch_speed_cmd);
        if (fabsf(pitch_speed_cmd) >= HOLD_REF_UPDATE_MIN_SPEED && pitch_abs >= HOLD_REF_UPDATE_MIN_RAW) {
            pitch_hold_ref = motor_pitch->measure.total_angle;
        }
    } else if (pitch_ctrl_mode == AXIS_CTRL_BRAKE) {
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorSetRef(motor_pitch, 0.0f);
    } else {
        if (last_pitch_ctrl_mode != AXIS_CTRL_ANGLE) {
            TelemetryPrintf("[gmb_et08] pitch -> hold lock_x10=%ld now_x10=%ld\r\n",
                            (long)(pitch_hold_ref * 10.0f),
                            (long)(motor_pitch->measure.total_angle * 10.0f));
        }
        DJIMotorOuterLoop(motor_pitch, ANGLE_LOOP);
        DJIMotorSetRef(motor_pitch, pitch_hold_ref);
    }

    if (yaw_ctrl_mode != last_yaw_ctrl_mode) {
        TelemetryPrintf("[gmb_et08] yaw loop -> %s\r\n", AxisCtrlModeName(yaw_ctrl_mode));
    }
    if (pitch_ctrl_mode != last_pitch_ctrl_mode) {
        TelemetryPrintf("[gmb_et08] pitch loop -> %s\r\n", AxisCtrlModeName(pitch_ctrl_mode));
    }
    last_yaw_ctrl_mode = yaw_ctrl_mode;
    last_pitch_ctrl_mode = pitch_ctrl_mode;

    return true;
}

static void SendTelemetry(void)
{
    static uint32_t seq = 0U;
    seq++;

    const int16_t rx = (et08_ctrl != NULL) ? et08_ctrl->right.x : 0;
    const int16_t ry = (et08_ctrl != NULL) ? et08_ctrl->right.y : 0;
    const long imu_yaw_total_x10 = (long)((gimbal_imu != NULL) ? (gimbal_imu->YawTotalAngle * 10.0f) : 0.0f);
    const long imu_yaw_x10 = (long)((gimbal_imu != NULL) ? (gimbal_imu->Yaw * 10.0f) : 0.0f);
    const long imu_pitch_x10 = (long)((gimbal_imu != NULL) ? (gimbal_imu->Pitch * 10.0f) : 0.0f);
    const long imu_gz_x10 = (long)((gimbal_imu != NULL) ? (gimbal_imu->Gyro[2] * 10.0f) : 0.0f);
    const long imu_gx_x10 = (long)((gimbal_imu != NULL) ? (gimbal_imu->Gyro[0] * 10.0f) : 0.0f);

    TelemetryPrintf("[gmb_et08] #%lu online=%u fs=%u lost=%u mode=%s raw(rx=%d ry=%d)\r\n",
                    (unsigned long)seq,
                    (unsigned int)ET08_IsOnline(),
                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->failsafe : 0U),
                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->frame_lost : 0U),
                    (gimbal_mode == GIMBAL_MODE_FOLLOW) ? "FOLLOW" : "SEPARATE",
                    rx,
                    ry);

    TelemetryPrintf("[gmb_et08] loop(y=%s p=%s) ref_spd(y_x10=%ld p_x10=%ld) fdb_spd(y_x10=%ld p_x10=%ld) hold_ang(y_x10=%ld p_x10=%ld)\r\n",
                    AxisCtrlModeName(yaw_ctrl_mode),
                    AxisCtrlModeName(pitch_ctrl_mode),
                    (long)(yaw_speed_ref * 10.0f),
                    (long)(pitch_speed_ref * 10.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->measure.speed_aps * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->measure.speed_aps * 10.0f) : 0.0f),
                    (long)(yaw_hold_ref * 10.0f),
                    (long)(pitch_hold_ref * 10.0f));

    TelemetryPrintf("[gmb_et08] yaw_state hold_x10=%ld enc_x10=%ld imu_yaw_total_x10=%ld imu_yaw_x10=%ld imu_gz_x10=%ld\r\n",
                    (long)(yaw_hold_ref * 10.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->measure.total_angle * 10.0f) : 0.0f),
                    imu_yaw_total_x10,
                    imu_yaw_x10,
                    imu_gz_x10);

    TelemetryPrintf("[gmb_et08] imu yaw_total_x10=%ld yaw_x10=%ld pitch_x10=%ld gz_x10=%ld gx_x10=%ld\r\n",
                    imu_yaw_total_x10,
                    imu_yaw_x10,
                    imu_pitch_x10,
                    imu_gz_x10,
                    imu_gx_x10);

    TelemetryPrintf("[gmb_et08] yaw_pid a(ref_x10=%ld fdb_x10=%ld err_x10=%ld out_x10=%ld sat=%u) s(ref_x10=%ld fdb_x10=%ld err_x10=%ld out_x10=%ld sat=%u)\r\n",
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.angle_PID.Ref * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.angle_PID.Measure * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.angle_PID.Err * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.angle_PID.Output * 10.0f) : 0.0f),
                    (unsigned int)((motor_yaw != NULL) ? PidNearlySaturated(&motor_yaw->motor_controller.angle_PID) : 0U),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.speed_PID.Ref * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.speed_PID.Measure * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.speed_PID.Err * 10.0f) : 0.0f),
                    (long)((motor_yaw != NULL) ? (motor_yaw->motor_controller.speed_PID.Output * 10.0f) : 0.0f),
                    (unsigned int)((motor_yaw != NULL) ? PidNearlySaturated(&motor_yaw->motor_controller.speed_PID) : 0U));

    TelemetryPrintf("[gmb_et08] pitch_pid a(ref_x10=%ld fdb_x10=%ld err_x10=%ld out_x10=%ld sat=%u) s(ref_x10=%ld fdb_x10=%ld err_x10=%ld out_x10=%ld sat=%u)\r\n",
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.angle_PID.Ref * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.angle_PID.Measure * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.angle_PID.Err * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.angle_PID.Output * 10.0f) : 0.0f),
                    (unsigned int)((motor_pitch != NULL) ? PidNearlySaturated(&motor_pitch->motor_controller.angle_PID) : 0U),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.speed_PID.Ref * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.speed_PID.Measure * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.speed_PID.Err * 10.0f) : 0.0f),
                    (long)((motor_pitch != NULL) ? (motor_pitch->motor_controller.speed_PID.Output * 10.0f) : 0.0f),
                    (unsigned int)((motor_pitch != NULL) ? PidNearlySaturated(&motor_pitch->motor_controller.speed_PID) : 0U));
}

static void SendVofaFrame(void)
{
    float ch[38];
    uint8_t txbuf[sizeof(ch) + 4U];
    static const uint8_t tail[4] = {0x00U, 0x00U, 0x80U, 0x7FU};

    ch[0] = (float)HAL_GetTick();
    ch[1] = (float)ET08_IsOnline();
    ch[2] = (float)gimbal_mode;
    ch[3] = (float)yaw_ctrl_mode;
    ch[4] = (float)pitch_ctrl_mode;
    ch[5] = yaw_speed_ref;
    ch[6] = (motor_yaw != NULL) ? motor_yaw->measure.speed_aps : 0.0f;
    ch[7] = yaw_hold_ref;
    ch[8] = pitch_speed_ref;
    ch[9] = (motor_pitch != NULL) ? motor_pitch->measure.speed_aps : 0.0f;
    ch[10] = pitch_hold_ref;
    ch[11] = (motor_pitch != NULL) ? motor_pitch->measure.total_angle : 0.0f;
    ch[12] = (motor_pitch != NULL) ? motor_pitch->motor_controller.angle_PID.Ref : 0.0f;
    ch[13] = (motor_pitch != NULL) ? motor_pitch->motor_controller.angle_PID.Measure : 0.0f;
    ch[14] = (motor_pitch != NULL) ? motor_pitch->motor_controller.angle_PID.Err : 0.0f;
    ch[15] = (motor_pitch != NULL) ? motor_pitch->motor_controller.angle_PID.Output : 0.0f;
    ch[16] = (motor_pitch != NULL) ? motor_pitch->motor_controller.angle_PID.MaxOut : 0.0f;
    ch[17] = (motor_pitch != NULL) ? motor_pitch->motor_controller.speed_PID.Ref : 0.0f;
    ch[18] = (motor_pitch != NULL) ? motor_pitch->motor_controller.speed_PID.Measure : 0.0f;
    ch[19] = (motor_pitch != NULL) ? motor_pitch->motor_controller.speed_PID.Err : 0.0f;
    ch[20] = (motor_pitch != NULL) ? motor_pitch->motor_controller.speed_PID.Output : 0.0f;
    ch[21] = (motor_pitch != NULL) ? PidSatRatio(&motor_pitch->motor_controller.angle_PID) : 0.0f;
    ch[22] = (motor_yaw != NULL) ? motor_yaw->measure.total_angle : 0.0f;
    ch[23] = (motor_yaw != NULL) ? motor_yaw->motor_controller.angle_PID.Ref : 0.0f;
    ch[24] = (motor_yaw != NULL) ? motor_yaw->motor_controller.angle_PID.Measure : 0.0f;
    ch[25] = (motor_yaw != NULL) ? motor_yaw->motor_controller.angle_PID.Err : 0.0f;
    ch[26] = (motor_yaw != NULL) ? motor_yaw->motor_controller.angle_PID.Output : 0.0f;
    ch[27] = (motor_yaw != NULL) ? motor_yaw->motor_controller.angle_PID.MaxOut : 0.0f;
    ch[28] = (motor_yaw != NULL) ? motor_yaw->motor_controller.speed_PID.Ref : 0.0f;
    ch[29] = (motor_yaw != NULL) ? motor_yaw->motor_controller.speed_PID.Measure : 0.0f;
    ch[30] = (motor_yaw != NULL) ? motor_yaw->motor_controller.speed_PID.Err : 0.0f;
    ch[31] = (motor_yaw != NULL) ? motor_yaw->motor_controller.speed_PID.Output : 0.0f;
    ch[32] = (motor_yaw != NULL) ? PidSatRatio(&motor_yaw->motor_controller.angle_PID) : 0.0f;
    ch[33] = (gimbal_imu != NULL) ? gimbal_imu->YawTotalAngle : 0.0f;
    ch[34] = (gimbal_imu != NULL) ? gimbal_imu->Yaw : 0.0f;
    ch[35] = (gimbal_imu != NULL) ? gimbal_imu->Pitch : 0.0f;
    ch[36] = (gimbal_imu != NULL) ? gimbal_imu->Gyro[2] : 0.0f;
    ch[37] = (gimbal_imu != NULL) ? gimbal_imu->Gyro[0] : 0.0f;

    memcpy(txbuf, ch, sizeof(ch));
    memcpy(txbuf + sizeof(ch), tail, sizeof(tail));
    TelemetrySendBuffer(txbuf, (uint16_t)sizeof(txbuf));
}

/* Main ----------------------------------------------------------------------*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    MX_SPI1_Init();
    MX_TIM4_Init();
    MX_TIM5_Init();
    MX_USART3_UART_Init();
    MX_RNG_Init();
    MX_RTC_Init();
    MX_TIM1_Init();
    MX_TIM10_Init();
    MX_USART1_UART_Init();
    MX_USART6_UART_Init();
    MX_TIM8_Init();
    MX_I2C2_Init();
    MX_I2C3_Init();
    MX_SPI2_Init();
    MX_CRC_Init();
    MX_DAC_Init();

    Debug_DisableWatchdogs();
    DWT_Init(168);
    BSPLogInit();
    InitTelemetryUsart();

    et08_ctrl = ET08_Init(&RC_UART);
    gimbal_imu = INS_Init();
    GimbalMotorsInit();

    HAL_Delay(MOTOR_STABILIZE_TIME_MS);
    TelemetrySendString("[gmb_et08] control loop start\r\n");

    uint32_t last_control_tick = DWT_GetTimeline_ms();
    uint32_t last_telemetry_tick = last_control_tick;
    uint32_t last_vofa_tick = last_control_tick;
    uint32_t last_ins_tick = last_control_tick;
    uint32_t last_daemon_tick = last_control_tick;
    uint32_t last_motor_tick = last_control_tick;

    while (1) {
        uint32_t now = DWT_GetTimeline_ms();

        if ((now - last_ins_tick) >= INS_TASK_PERIOD_MS) {
            last_ins_tick = now;
            INS_Task();
        }
        if ((now - last_daemon_tick) >= DAEMON_TASK_PERIOD_MS) {
            last_daemon_tick = now;
            DaemonTask();
        }
        if ((now - last_motor_tick) >= MOTOR_CONTROL_PERIOD_MS) {
            last_motor_tick = now;
            DJIMotorControl();
        }

        if ((now - last_control_tick) >= MAIN_LOOP_PERIOD_MS) {
            last_control_tick = now;

            bool online = GimbalUpdateFromET08();
            if (!online) {
                GimbalStop();
                if (last_online_state != 0U) {
                    TelemetryPrintf("[gmb_et08] remote offline/failsafe -> stop online=%u fs=%u lost=%u raw(rx=%d ry=%d)\r\n",
                                    (unsigned int)ET08_IsOnline(),
                                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->failsafe : 0U),
                                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->frame_lost : 0U),
                                    (int)((et08_ctrl != NULL) ? et08_ctrl->right.x : 0),
                                    (int)((et08_ctrl != NULL) ? et08_ctrl->right.y : 0));
                    last_online_state = 0U;
                }
            } else if (last_online_state == 0U) {
                TelemetrySendString("[gmb_et08] remote online -> run\r\n");
                last_online_state = 1U;
            }
        }

        if ((now - last_telemetry_tick) >= TELEMETRY_PERIOD_MS) {
            last_telemetry_tick = now;
            SendTelemetry();
        }
        if ((now - last_vofa_tick) >= VOFA_PERIOD_MS) {
            last_vofa_tick = now;
            SendVofaFrame();
        }

        HAL_Delay(1);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        HAL_IncTick();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}
