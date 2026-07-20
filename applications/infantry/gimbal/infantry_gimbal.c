/**
 * @file minimal_gimbal.c
 * @brief 云台控制模块实现
 *
 * 迁移基准:
 * - Yaw 始终使用 IMU YawTotalAngle 闭环
 * - Pitch 使用“速度控制 -> 制动 -> 角度保持”三态
 * - 底盘通过公开接口读取云台相对底盘夹角，不再直接耦合电机对象
 */

#include "infantry_gimbal.h"

#include <math.h>
#include <string.h>

#include "can.h"
#include "dji_motor.h"
#include "ins_task.h"
#include "infantry_config.h"
#include "infantry_debug.h"
#include "infantry_gimbal_limits.h"
#include "infantry_gimbal_units.h"
#include "infantry_types.h"
#include "rm_time.h"
#include "user_lib.h"

static DJIMotorInstance *motor_yaw = NULL;
static DJIMotorInstance *motor_pitch = NULL;
static uint8_t gimbal_imu_initialized = 0U;

static InfantryControlMode_e current_mode = INFANTRY_CONTROL_FOLLOW;
static InfantryControlMode_e last_mode_logged = INFANTRY_CONTROL_FOLLOW;
static uint8_t gimbal_enabled = 0U;
static uint32_t gimbal_last_tick = 0U;
static uint32_t gimbal_debug_last_tick = 0U;

static float yaw_speed_ref_last = 0.0f;
static float pitch_speed_ref_last = 0.0f;
static float yaw_hold_ref = 0.0f;
static float pitch_hold_ref = 0.0f;
static float pitch_release_hold_ref = 0.0f;
static float pitch_current_ff = 0.0f;
/* These three values are written and consumed only by the 5 ms motor task. */
static float motor_yaw_imu_angle_fdb = 0.0f;
static float motor_yaw_imu_speed_fdb = 0.0f;
static float motor_pitch_imu_speed_fdb = 0.0f;
static AxisCtrlMode_e pitch_ctrl_mode = AXIS_CTRL_ANGLE;
static AxisCtrlMode_e last_pitch_ctrl_mode = AXIS_CTRL_ANGLE;
static uint8_t pitch_cmd_active = 0U;
static uint8_t pitch_enter_cnt = 0U;
static uint8_t pitch_exit_cnt = 0U;
static uint8_t pitch_brake_stable_count = 0U;
static uint32_t pitch_brake_start_ms = 0U;
static uint8_t pitch_pid_reset_request = DJI_MOTOR_PID_RESET_NONE;

static const GimbalPitchLimitConfig pitch_limit_config = {
    .min_angle_deg = GIMBAL_PITCH_MIN_DEG,
    .max_angle_deg = GIMBAL_PITCH_MAX_DEG,
    .soft_margin_deg = GIMBAL_PITCH_SOFT_MARGIN_DEG,
    .command_to_imu_sign = GIMBAL_PITCH_IMU_DIRECTION_SIGN,
};

static float MotorTotalAngle(const DJIMotorInstance *motor)
{
    DJI_Motor_Measure_s measure;

    return (motor != NULL && DJIMotorGetMeasure(motor, &measure))
               ? measure.total_angle
               : 0.0f;
}

static float MotorSingleRoundAngle(const DJIMotorInstance *motor)
{
    DJI_Motor_Measure_s measure;

    return (motor != NULL && DJIMotorGetMeasure(motor, &measure))
               ? measure.angle_single_round
               : 0.0f;
}

static float MotorSpeed(const DJIMotorInstance *motor)
{
    DJI_Motor_Measure_s measure;

    return (motor != NULL && DJIMotorGetMeasure(motor, &measure))
               ? measure.speed_aps
               : 0.0f;
}

static float ClampFloat(float value, float min_value, float max_value)
{
    if (!isfinite(value) || !isfinite(min_value) || !isfinite(max_value) ||
        min_value > max_value) {
        return 0.0f;
    }
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float WrapAngleDeg180(float angle_deg)
{
    while (angle_deg > 180.0f) {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f) {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

static bool ReadAttitude(attitude_t *attitude)
{
    return (gimbal_imu_initialized != 0U) && INS_Read(attitude);
}

static void RequestPitchPidReset(uint8_t mask)
{
    pitch_pid_reset_request |= mask & DJI_MOTOR_PID_RESET_ALL;
}

static bool PublishYawCommand(float reference,
                              Motor_Working_Type_e working_state,
                              bool imu_available,
                              uint8_t pid_reset_mask)
{
    DJIMotorCommand command;

    if (!DJIMotorGetCommand(motor_yaw, &command)) {
        return false;
    }
    command.settings.angle_feedback_source =
        imu_available ? OTHER_FEED : MOTOR_FEED;
    command.settings.speed_feedback_source =
        imu_available ? OTHER_FEED : MOTOR_FEED;
    command.settings.outer_loop_type = ANGLE_LOOP;
    command.reference = reference;
    command.working_state = working_state;
    command.pid_reset_mask = pid_reset_mask;
    return DJIMotorPublishCommand(motor_yaw, &command);
}

static bool PublishPitchCommand(Closeloop_Type_e outer_loop,
                                float reference,
                                Motor_Working_Type_e working_state,
                                bool imu_available)
{
    DJIMotorCommand command;

    if (!DJIMotorGetCommand(motor_pitch, &command)) {
        return false;
    }
    command.settings.angle_feedback_source = MOTOR_FEED;
    command.settings.speed_feedback_source =
        imu_available ? OTHER_FEED : MOTOR_FEED;
    command.settings.outer_loop_type = outer_loop;
    command.reference = reference;
    command.current_feedforward = pitch_current_ff;
    command.external_input_mask |= DJI_MOTOR_EXTERNAL_CURRENT_FF;
    command.working_state = working_state;
    command.pid_reset_mask = pitch_pid_reset_request;
    if (!DJIMotorPublishCommand(motor_pitch, &command)) {
        return false;
    }

    pitch_pid_reset_request = DJI_MOTOR_PID_RESET_NONE;
    return true;
}

static void ResetPitchState(void)
{
    pitch_ctrl_mode = AXIS_CTRL_ANGLE;
    last_pitch_ctrl_mode = AXIS_CTRL_ANGLE;
    pitch_cmd_active = 0U;
    pitch_enter_cnt = 0U;
    pitch_exit_cnt = 0U;
    pitch_brake_stable_count = 0U;
    pitch_brake_start_ms = 0U;
}

static void UpdatePitchGravityFeedforward(bool imu_available,
                                          float pitch_imu_deg)
{
    float pitch_ff_raw;

    if (motor_pitch == NULL || !imu_available || !isfinite(pitch_imu_deg)) {
        pitch_current_ff = 0.0f;
        return;
    }

    pitch_ff_raw = InfantryGimbal_GravityFeedforward(
        pitch_imu_deg, PITCH_GRAVITY_HORIZONTAL_DEG,
        PITCH_GRAVITY_FF_K, PITCH_GRAVITY_FF_MAX);
    pitch_current_ff = pitch_current_ff * PITCH_FF_LPF +
                       pitch_ff_raw * (1.0f - PITCH_FF_LPF);
}

static float GetPitchSpeedFeedback(void)
{
    attitude_t attitude = {0};

    if (ReadAttitude(&attitude)) {
        return InfantryGimbal_RadPerSecToDegPerSec(attitude.Gyro[0]);
    }
    return (motor_pitch != NULL) ? MotorSpeed(motor_pitch) : 0.0f;
}

static void UpdatePitchControlMode(const Input_Data_t *input, uint32_t now_ms)
{
    if (input == NULL) {
        return;
    }

    if (pitch_cmd_active != 0U) {
        if (input->pitch_control_active == 0U) {
            if (pitch_exit_cnt < 255U) {
                pitch_exit_cnt++;
            }
        } else {
            pitch_exit_cnt = 0U;
        }
        pitch_enter_cnt = 0U;
        if (pitch_exit_cnt >= 10U) {
            pitch_exit_cnt = 0U;
            pitch_cmd_active = 0U;
        }
    } else {
        if (input->pitch_control_active != 0U) {
            if (pitch_enter_cnt < 255U) {
                pitch_enter_cnt++;
            }
        } else {
            pitch_enter_cnt = 0U;
        }
        pitch_exit_cnt = 0U;
        if (pitch_enter_cnt >= 4U) {
            pitch_enter_cnt = 0U;
            pitch_cmd_active = 1U;
        }
    }

    if (pitch_cmd_active != 0U) {
        if (pitch_ctrl_mode != AXIS_CTRL_SPEED) {
            RequestPitchPidReset(DJI_MOTOR_PID_RESET_ALL);
        }
        pitch_ctrl_mode = AXIS_CTRL_SPEED;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = 0U;
    } else if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        pitch_release_hold_ref =
            MotorTotalAngle(motor_pitch) +
            GetPitchSpeedFeedback() * PITCH_RELEASE_SPEED_PREDICT_GAIN;
        pitch_hold_ref = pitch_release_hold_ref;
        pitch_ctrl_mode = AXIS_CTRL_BRAKE;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = now_ms;
        RequestPitchPidReset(DJI_MOTOR_PID_RESET_SPEED);
    } else if (pitch_ctrl_mode == AXIS_CTRL_BRAKE) {
        if (fabsf(GetPitchSpeedFeedback()) <= PITCH_BRAKE_SPEED_EPS) {
            if (pitch_brake_stable_count < 255U) {
                pitch_brake_stable_count++;
            }
        } else {
            pitch_brake_stable_count = 0U;
        }

        if (pitch_brake_stable_count >= PITCH_BRAKE_STABLE_COUNT ||
            (now_ms - pitch_brake_start_ms) >= PITCH_BRAKE_TIMEOUT_MS) {
            pitch_hold_ref = pitch_release_hold_ref;
            pitch_ctrl_mode = AXIS_CTRL_ANGLE;
            RequestPitchPidReset(DJI_MOTOR_PID_RESET_ALL);
        }
    }
}

bool Gimbal_Init(void)
{
    Motor_Init_Config_s yaw_config;
    Motor_Init_Config_s pitch_config;
    attitude_t attitude;

    if (!GimbalPitchLimit_IsConfigValid(&pitch_limit_config)) {
        MDBG_GMB("invalid pitch limit config");
        return false;
    }

    gimbal_imu_initialized =
        (INS_InitWithTimeout(INFANTRY_IMU_INIT_TIMEOUT_MS) != NULL) ? 1U : 0U;
    motor_yaw_imu_angle_fdb = 0.0f;
    motor_yaw_imu_speed_fdb = 0.0f;
    motor_pitch_imu_speed_fdb = 0.0f;

    if (gimbal_imu_initialized == 0U) {
        MDBG_SYS("INS init failed status=%u sensor_error=0x%02x",
                 (unsigned)INS_GetInitStatus(),
                 (unsigned)INS_GetSensorInitError());
        return false;
    }

    yaw_config = (Motor_Init_Config_s){
        .motor_type = GM6020,
        .can_init_config = {
            .can_handle = &YAW_CAN,
            .tx_id = YAW_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = YAW_ANGLE_KP,
                .Ki = YAW_ANGLE_KI,
                .Kd = YAW_ANGLE_KD,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_ANGLE_MAX_OUT,
            },
            .speed_PID = {
                .Kp = YAW_SPEED_KP,
                .Ki = YAW_SPEED_KI,
                .Kd = YAW_SPEED_KD,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_SPEED_MAX_OUT,
            },
            .other_angle_feedback_ptr = (gimbal_imu_initialized != 0U)
                                            ? &motor_yaw_imu_angle_fdb
                                            : NULL,
            .other_speed_feedback_ptr = (gimbal_imu_initialized != 0U)
                                            ? &motor_yaw_imu_speed_fdb
                                            : NULL,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = OTHER_FEED,
            .speed_feedback_source = OTHER_FEED,
            .speed_unit = MOTOR_SPEED_DEG_PER_SEC,
            .outer_loop_type = ANGLE_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    pitch_config = (Motor_Init_Config_s){
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
            .other_speed_feedback_ptr = (gimbal_imu_initialized != 0U)
                                            ? &motor_pitch_imu_speed_fdb
                                            : NULL,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = OTHER_FEED,
            .speed_unit = MOTOR_SPEED_DEG_PER_SEC,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .feedforward_flag = CURRENT_FEEDFORWARD,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    motor_yaw = DJIMotorInit(&yaw_config);
    motor_pitch = DJIMotorInit(&pitch_config);

    if ((motor_yaw == NULL) || (motor_pitch == NULL)) {
        MDBG_SYS("gimbal motor registration failed yaw=%u pitch=%u",
                 motor_yaw != NULL ? 1U : 0U,
                 motor_pitch != NULL ? 1U : 0U);
    }

    if (motor_yaw != NULL) {
        bool imu_available = ReadAttitude(&attitude);

        yaw_hold_ref = imu_available ? attitude.YawTotalAngle
                                     : MotorTotalAngle(motor_yaw);
        (void)PublishYawCommand(yaw_hold_ref, MOTOR_STOP, imu_available,
                                DJI_MOTOR_PID_RESET_ALL);
    }

    if (motor_pitch != NULL) {
        pitch_hold_ref = MotorTotalAngle(motor_pitch);
        pitch_release_hold_ref = pitch_hold_ref;
        RequestPitchPidReset(DJI_MOTOR_PID_RESET_ALL);
        (void)PublishPitchCommand(SPEED_LOOP, 0.0f, MOTOR_STOP,
                                  ReadAttitude(&attitude));
    }

    ResetPitchState();
    current_mode = INFANTRY_CONTROL_FOLLOW;
    last_mode_logged = current_mode;
    MDBG_GMB("limits yaw_dps=%ld pitch_dps=%ld gravity_k=%ld gravity_max=%ld horizontal_x10=%ld",
             (long)GIMBAL_YAW_MAX_SPEED_DEG_S,
             (long)GIMBAL_PITCH_MAX_SPEED_DEG_S,
             (long)PITCH_GRAVITY_FF_K,
             (long)PITCH_GRAVITY_FF_MAX,
             (long)(PITCH_GRAVITY_HORIZONTAL_DEG * 10.0f));
    return (gimbal_imu_initialized != 0U) && (motor_yaw != NULL) &&
           (motor_pitch != NULL);
}

void Gimbal_MotorStep(void)
{
    attitude_t attitude;

    if (!ReadAttitude(&attitude)) {
        return;
    }

    motor_yaw_imu_angle_fdb = attitude.YawTotalAngle;
    motor_yaw_imu_speed_fdb =
        InfantryGimbal_RadPerSecToDegPerSec(attitude.Gyro[2]);
    motor_pitch_imu_speed_fdb =
        InfantryGimbal_RadPerSecToDegPerSec(attitude.Gyro[0]);
}

void Gimbal_Update(Input_Data_t *input)
{
    uint32_t now_ms;
    float dt_s = 0.02f;
    float yaw_speed_cmd;
    float pitch_speed_cmd;
    float pitch_reference;
    Closeloop_Type_e pitch_outer_loop;
    attitude_t attitude = {0};
    bool imu_available;
    Gimbal_Cmd_t cmd = {0};

    if (input == NULL || !input->online || input->emergency_stop) {
        MDBG_GMB("stop by input offline/estop");
        Gimbal_Stop();
        return;
    }

    if (motor_yaw == NULL || motor_pitch == NULL) {
        return;
    }

    now_ms = RmTime_NowMs();
    if (gimbal_last_tick != 0U) {
        dt_s = (float)(now_ms - gimbal_last_tick) / 1000.0f;
        dt_s = ClampFloat(dt_s, 0.001f, 0.05f);
    }
    gimbal_last_tick = now_ms;
    imu_available = ReadAttitude(&attitude);

    if (!gimbal_enabled) {
        gimbal_enabled = 1U;
        yaw_hold_ref = imu_available ? attitude.YawTotalAngle
                                     : MotorTotalAngle(motor_yaw);
        pitch_hold_ref = MotorTotalAngle(motor_pitch);
        pitch_release_hold_ref = pitch_hold_ref;
        ResetPitchState();
        RequestPitchPidReset(DJI_MOTOR_PID_RESET_ALL);
    }

    current_mode = input->control_mode;
    if (current_mode != last_mode_logged) {
        MDBG_GMB("mode switch %u -> %u", (unsigned)last_mode_logged, (unsigned)current_mode);
        last_mode_logged = current_mode;
    }

    UpdatePitchGravityFeedforward(imu_available, attitude.Pitch);
    GetPitchSpeedFeedback();

    yaw_speed_cmd = ClampFloat(input->gimbal_yaw_intent, -1.0f, 1.0f) *
                    GIMBAL_YAW_MAX_SPEED_DEG_S;
    if (input->yaw_control_active == 0U) {
        yaw_speed_cmd = 0.0f;
    }
    yaw_hold_ref += yaw_speed_cmd * dt_s;

    (void)PublishYawCommand(yaw_hold_ref, MOTOR_ENALBED, imu_available,
                            DJI_MOTOR_PID_RESET_NONE);

    pitch_speed_cmd = ClampFloat(input->gimbal_pitch_intent, -1.0f, 1.0f) *
                      GIMBAL_PITCH_MAX_SPEED_DEG_S;
    if (input->pitch_control_active == 0U) {
        pitch_speed_cmd = 0.0f;
    }
    pitch_speed_cmd = GimbalPitchLimit_ClampSpeed(
        pitch_speed_cmd, imu_available,
        imu_available ? attitude.Pitch : 0.0f, &pitch_limit_config);

    UpdatePitchControlMode(input, now_ms);

    if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        pitch_outer_loop = SPEED_LOOP;
        pitch_reference = pitch_speed_cmd;
        if (fabsf(pitch_speed_cmd) > 1.0f) {
            pitch_hold_ref = MotorTotalAngle(motor_pitch);
        }
    } else if (pitch_ctrl_mode == AXIS_CTRL_BRAKE) {
        pitch_outer_loop = SPEED_LOOP;
        pitch_reference = 0.0f;
    } else {
        pitch_outer_loop = ANGLE_LOOP;
        pitch_reference = GimbalPitchLimit_ClampAngleReference(
            pitch_hold_ref, MotorTotalAngle(motor_pitch), imu_available,
            imu_available ? attitude.Pitch : 0.0f, &pitch_limit_config);
        /* 丢弃被执行层截断的外向目标，避免每周期重新请求越界。 */
        pitch_hold_ref = pitch_reference;
    }
    (void)PublishPitchCommand(pitch_outer_loop, pitch_reference,
                              MOTOR_ENALBED, imu_available);

    if (pitch_ctrl_mode != last_pitch_ctrl_mode) {
        MDBG_GMB("pitch loop -> %u", (unsigned)pitch_ctrl_mode);
        last_pitch_ctrl_mode = pitch_ctrl_mode;
    }

    yaw_speed_ref_last = yaw_speed_cmd;
    pitch_speed_ref_last = pitch_speed_cmd;

    cmd.yaw_speed = yaw_speed_cmd;
    cmd.pitch_speed = pitch_speed_cmd;
    cmd.yaw_angle = yaw_hold_ref;
    cmd.pitch_angle = pitch_hold_ref;
    cmd.mode = current_mode;
    cmd.manual_pitch = (pitch_ctrl_mode == AXIS_CTRL_SPEED) ? 1U : 0U;
    cmd.pitch_ctrl_mode = pitch_ctrl_mode;
    cmd.control_mode = CTRL_ENABLE;
    cmd.ref_type = (pitch_ctrl_mode == AXIS_CTRL_SPEED) ? REF_SPEED : REF_ANGLE;
    g_robot.gimbal = cmd;

    if ((now_ms - gimbal_debug_last_tick) >= GIMBAL_DEBUG_DETAIL_PERIOD_MS) {
        gimbal_debug_last_tick = now_ms;
        MDBG_GMB("mode=%u pitch_loop=%u yaw_ref=%ld yaw_imu=%ld off=%ld pitch_ref=%ld pitch_enc=%ld pitch_imu=%ld pitch_spd=%ld ff=%ld",
                 (unsigned)current_mode,
                 (unsigned)pitch_ctrl_mode,
                 (long)(yaw_hold_ref * 10.0f),
                 (long)(Gimbal_GetYawIMUAngle() * 10.0f),
                 (long)(Gimbal_GetYawOffsetLogicDeg() * 10.0f),
                 (long)(pitch_hold_ref * 10.0f),
                 (long)(Gimbal_GetPitchEncoderAngle() * 10.0f),
                 (long)(attitude.Pitch * 10.0f),
                 (long)(GetPitchSpeedFeedback() * 10.0f),
                 (long)pitch_current_ff);
    }
}

void Gimbal_Stop(void)
{
    gimbal_enabled = 0U;
    gimbal_last_tick = 0U;
    gimbal_debug_last_tick = 0U;
    yaw_speed_ref_last = 0.0f;
    pitch_speed_ref_last = 0.0f;
    pitch_current_ff = 0.0f;
    ResetPitchState();
    RequestPitchPidReset(DJI_MOTOR_PID_RESET_ALL);

    g_robot.gimbal = (Gimbal_Cmd_t){
        .mode = current_mode,
        .pitch_ctrl_mode = AXIS_CTRL_ANGLE,
        .control_mode = CTRL_ZERO_FORCE,
        .ref_type = REF_SPEED,
    };

    if (motor_yaw != NULL) {
        (void)PublishYawCommand(yaw_hold_ref, MOTOR_STOP, INS_IsReady(),
                                DJI_MOTOR_PID_RESET_ALL);
    }
    if (motor_pitch != NULL) {
        (void)PublishPitchCommand(SPEED_LOOP, 0.0f, MOTOR_STOP,
                                  INS_IsReady());
    }
}

InfantryControlMode_e Gimbal_GetMode(void)
{
    return current_mode;
}

float Gimbal_GetYawSpeedRef(void)
{
    return yaw_speed_ref_last;
}

float Gimbal_GetYawSpeedFdb(void)
{
    attitude_t attitude;

    if (ReadAttitude(&attitude)) {
        return InfantryGimbal_RadPerSecToDegPerSec(attitude.Gyro[2]);
    }
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return MotorSpeed(motor_yaw);
}

float Gimbal_GetPitchSpeedRef(void)
{
    return pitch_speed_ref_last;
}

float Gimbal_GetPitchSpeedFdb(void)
{
    return GetPitchSpeedFeedback();
}

float Gimbal_GetPitchTargetAngle(void)
{
    return pitch_hold_ref;
}

float Gimbal_GetYawTargetAngle(void)
{
    return yaw_hold_ref;
}

float Gimbal_GetYawEncoderAngle(void)
{
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return MotorTotalAngle(motor_yaw);
}

float Gimbal_GetPitchEncoderAngle(void)
{
    if (motor_pitch == NULL) {
        return 0.0f;
    }
    return MotorTotalAngle(motor_pitch);
}

float Gimbal_GetYawIMUAngle(void)
{
    attitude_t attitude;

    return ReadAttitude(&attitude) ? attitude.YawTotalAngle : 0.0f;
}

float Gimbal_GetPitchIMUAngle(void)
{
    attitude_t attitude;

    return ReadAttitude(&attitude) ? attitude.Pitch : 0.0f;
}

float Gimbal_GetPitchGravityFeedforward(void)
{
    return pitch_current_ff;
}

float Gimbal_GetYawOffsetRawDeg(void)
{
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return WrapAngleDeg180(MotorSingleRoundAngle(motor_yaw) -
                           YAW_ALIGN_ANGLE_DEG);
}

float Gimbal_GetYawOffsetLogicDeg(void)
{
    return WrapAngleDeg180(Gimbal_GetYawOffsetRawDeg() - YAW_OFFSET_LOGIC_ZERO_DEG);
}

float Gimbal_GetYawOffsetLogicRad(void)
{
    return Gimbal_GetYawOffsetLogicDeg() * ((float)M_PI / 180.0f);
}

float Gimbal_GetYawRelativeSpeedRadS(void)
{
    DJI_Motor_Measure_s measure;

    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return DJIMotorGetMeasure(motor_yaw, &measure) ? measure.speed_rad_s
                                                   : 0.0f;
}

float Gimbal_GetYawLogicAngle(void)
{
    attitude_t attitude;

    if (!ReadAttitude(&attitude)) {
        return 0.0f;
    }
    return WrapAngleDeg180(attitude.YawTotalAngle -
                           IMU_YAW_LOGIC_ZERO_TOTAL_DEG);
}

bool Gimbal_AreMotorsHealthy(void)
{
    return DJIMotorIsOnline(motor_yaw) && DJIMotorIsOnline(motor_pitch);
}

bool Gimbal_IsHealthy(void)
{
    return (gimbal_imu_initialized != 0U) && INS_IsReady() &&
           Gimbal_AreMotorsHealthy();
}

AxisCtrlMode_e Gimbal_GetPitchCtrlMode(void)
{
    return pitch_ctrl_mode;
}

bool Gimbal_GetYawTuningSnapshot(GimbalYawTuningSnapshot *snapshot)
{
    DJI_Motor_Measure_s measure = {0};
    attitude_t attitude = {0};
    bool motor_valid;
    bool imu_valid;

    if (snapshot == NULL) {
        return false;
    }
    memset(snapshot, 0, sizeof(*snapshot));

    motor_valid = motor_yaw != NULL &&
                  DJIMotorGetMeasure(motor_yaw, &measure);
    imu_valid = ReadAttitude(&attitude);

    if (motor_valid) {
        snapshot->encoder_ecd = measure.ecd;
        snapshot->encoder_single_deg = measure.angle_single_round;
        snapshot->offset_raw_deg = WrapAngleDeg180(
            measure.angle_single_round - YAW_ALIGN_ANGLE_DEG);
        snapshot->offset_logic_deg = WrapAngleDeg180(
            snapshot->offset_raw_deg - YAW_OFFSET_LOGIC_ZERO_DEG);
        snapshot->relative_speed_rad_s = measure.speed_rad_s;
    }
    snapshot->target_total_deg = yaw_hold_ref;
    if (imu_valid) {
        snapshot->imu_total_deg = attitude.YawTotalAngle;
        snapshot->imu_single_deg = attitude.Yaw;
        snapshot->imu_gyro_z_rad_s = attitude.Gyro[2];
    }

    return motor_valid && imu_valid;
}
