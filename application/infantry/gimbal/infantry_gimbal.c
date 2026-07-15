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

#include "can.h"
#include "dji_motor.h"
#include "ins_task.h"
#include "infantry_config.h"
#include "infantry_debug.h"
#include "infantry_types.h"
#include "user_lib.h"

static DJIMotorInstance *motor_yaw = NULL;
static DJIMotorInstance *motor_pitch = NULL;
static attitude_t *gimbal_imu = NULL;

static GimbalMode_e current_mode = GIMBAL_FOLLOW_CHASSIS;
static GimbalMode_e last_mode_logged = GIMBAL_FOLLOW_CHASSIS;
static uint8_t gimbal_enabled = 0U;
static uint32_t gimbal_last_tick = 0U;
static uint32_t gimbal_debug_last_tick = 0U;

static float yaw_speed_ref_last = 0.0f;
static float pitch_speed_ref_last = 0.0f;
static float yaw_hold_ref = 0.0f;
static float pitch_hold_ref = 0.0f;
static float pitch_release_hold_ref = 0.0f;
static float pitch_current_ff = 0.0f;
static float pitch_imu_speed_fdb = 0.0f;
static AxisCtrlMode_e pitch_ctrl_mode = AXIS_CTRL_ANGLE;
static AxisCtrlMode_e last_pitch_ctrl_mode = AXIS_CTRL_ANGLE;
static uint8_t pitch_cmd_active = 0U;
static uint8_t pitch_enter_cnt = 0U;
static uint8_t pitch_exit_cnt = 0U;
static uint8_t pitch_brake_stable_count = 0U;
static uint32_t pitch_brake_start_ms = 0U;

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

static void ResetPidState(PIDInstance *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->Measure = 0.0f;
    pid->Last_Measure = 0.0f;
    pid->Err = 0.0f;
    pid->Last_Err = 0.0f;
    pid->Last_ITerm = 0.0f;
    pid->Pout = 0.0f;
    pid->Iout = 0.0f;
    pid->Dout = 0.0f;
    pid->ITerm = 0.0f;
    pid->Output = 0.0f;
    pid->Last_Output = 0.0f;
    pid->Last_Dout = 0.0f;
    pid->Ref = 0.0f;
    pid->DWT_CNT = 0U;
    pid->dt = 0.0f;
}

static void ResetPitchPidRuntime(void)
{
    if (motor_pitch == NULL) {
        return;
    }

    ResetPidState(&motor_pitch->motor_controller.angle_PID);
    ResetPidState(&motor_pitch->motor_controller.speed_PID);
    ResetPidState(&motor_pitch->motor_controller.current_PID);
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

static void UpdatePitchGravityFeedforward(void)
{
    float pitch_ff_raw;

    if (motor_pitch == NULL) {
        pitch_current_ff = 0.0f;
        return;
    }

    pitch_ff_raw =
        PITCH_GRAVITY_FF_K *
        sinf((motor_pitch->measure.total_angle - PITCH_GRAVITY_FF_OFFSET_DEG) * (float)M_PI / 180.0f);
    pitch_ff_raw = ClampFloat(pitch_ff_raw, -PITCH_GRAVITY_FF_MAX, PITCH_GRAVITY_FF_MAX);
    pitch_current_ff = pitch_current_ff * PITCH_FF_LPF + pitch_ff_raw * (1.0f - PITCH_FF_LPF);
}

static float GetPitchSpeedFeedback(void)
{
    if (gimbal_imu != NULL) {
        pitch_imu_speed_fdb = gimbal_imu->Gyro[0];
    } else if (motor_pitch != NULL) {
        pitch_imu_speed_fdb = motor_pitch->measure.speed_aps;
    } else {
        pitch_imu_speed_fdb = 0.0f;
    }
    return pitch_imu_speed_fdb;
}

static uint8_t IsVTMouseActive(const Input_Data_t *input)
{
    if (input == NULL) {
        return 0U;
    }
    return (input->active_input == INPUT_ACTIVE_VT &&
            (input->vt_raw.mouse_x != 0 || input->vt_raw.mouse_y != 0))
               ? 1U
               : 0U;
}

static float SelectYawDeadzone(const Input_Data_t *input)
{
    if (input == NULL) {
        return GIMBAL_SPEED_DEADZONE_ET08;
    }
    if (input->active_input == INPUT_ACTIVE_ET08) {
        return GIMBAL_SPEED_DEADZONE_ET08;
    }
    if (IsVTMouseActive(input)) {
        return GIMBAL_SPEED_DEADZONE_VT_MOUSE;
    }
    return GIMBAL_SPEED_DEADZONE_VT_STICK;
}

static float SelectPitchDeadzone(const Input_Data_t *input)
{
    if (input == NULL) {
        return GIMBAL_SPEED_DEADZONE_ET08;
    }
    if (input->active_input == INPUT_ACTIVE_ET08) {
        return GIMBAL_SPEED_DEADZONE_ET08;
    }
    if (IsVTMouseActive(input)) {
        return GIMBAL_SPEED_DEADZONE_VT_MOUSE;
    }
    return GIMBAL_SPEED_DEADZONE_VT_STICK;
}

static float SelectPitchSpeedFullScale(const Input_Data_t *input)
{
    if (input == NULL) {
        return GM6020_SPEED_MAX * ET08_PITCH_SPEED_SCALE;
    }
    if (input->active_input == INPUT_ACTIVE_ET08) {
        return GM6020_SPEED_MAX * ET08_PITCH_SPEED_SCALE;
    }
    if (IsVTMouseActive(input)) {
        return VT_MOUSE_PITCH_MODE_FULL_SCALE;
    }
    return 660.0f * VT_STICK_PITCH_SPEED_SCALE;
}

static void UpdatePitchControlMode(const Input_Data_t *input, float pitch_speed_cmd, uint32_t now_ms)
{
    int16_t pitch_abs = 0;

    if (input == NULL) {
        return;
    }

    if (input->active_input == INPUT_ACTIVE_ET08) {
        int16_t pitch_in = input->rc_raw.right_y;
        pitch_abs = (pitch_in >= 0) ? pitch_in : (int16_t)(-pitch_in);
    } else {
        float speed_ratio = fabsf(pitch_speed_cmd) / SelectPitchSpeedFullScale(input);
        pitch_abs = (int16_t)ClampFloat(speed_ratio * RC_STICK_SCALE, 0.0f, RC_STICK_SCALE);
    }

    if (pitch_cmd_active != 0U) {
        if (pitch_abs <= 90) {
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
        if (pitch_abs >= 180) {
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
            ResetPitchPidRuntime();
        }
        pitch_ctrl_mode = AXIS_CTRL_SPEED;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = 0U;
    } else if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        pitch_release_hold_ref =
            motor_pitch->measure.total_angle + GetPitchSpeedFeedback() * PITCH_RELEASE_SPEED_PREDICT_GAIN;
        pitch_hold_ref = pitch_release_hold_ref;
        pitch_ctrl_mode = AXIS_CTRL_BRAKE;
        pitch_brake_stable_count = 0U;
        pitch_brake_start_ms = now_ms;
        ResetPidState(&motor_pitch->motor_controller.speed_PID);
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
            ResetPitchPidRuntime();
        }
    }
}

void Gimbal_Init(void)
{
    Motor_Init_Config_s yaw_config;
    Motor_Init_Config_s pitch_config;

    gimbal_imu = INS_Init();

    yaw_config = (Motor_Init_Config_s){
        .motor_type = GM6020,
        .can_init_config = {
            .can_handle = &YAW_CAN,
            .tx_id = YAW_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = YAW_SEPARATE_ANGLE_KP,
                .Ki = YAW_SEPARATE_ANGLE_KI,
                .Kd = YAW_SEPARATE_ANGLE_KD,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_SEPARATE_ANGLE_MAX_OUT,
            },
            .speed_PID = {
                .Kp = YAW_SEPARATE_SPEED_KP,
                .Ki = YAW_SEPARATE_SPEED_KI,
                .Kd = YAW_SEPARATE_SPEED_KD,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = YAW_SEPARATE_SPEED_MAX_OUT,
            },
            .other_angle_feedback_ptr = (gimbal_imu != NULL) ? &gimbal_imu->YawTotalAngle : NULL,
            .other_speed_feedback_ptr = (gimbal_imu != NULL) ? &gimbal_imu->Gyro[2] : NULL,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = OTHER_FEED,
            .speed_feedback_source = OTHER_FEED,
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
            .other_speed_feedback_ptr = &pitch_imu_speed_fdb,
            .current_feedforward_ptr = &pitch_current_ff,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = OTHER_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .feedforward_flag = CURRENT_FEEDFORWARD,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    motor_yaw = DJIMotorInit(&yaw_config);
    motor_pitch = DJIMotorInit(&pitch_config);

    if (motor_yaw != NULL) {
        DJIMotorOuterLoop(motor_yaw, ANGLE_LOOP);
        DJIMotorStop(motor_yaw);
        yaw_hold_ref = (gimbal_imu != NULL) ? gimbal_imu->YawTotalAngle : motor_yaw->measure.total_angle;
    }

    if (motor_pitch != NULL) {
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorStop(motor_pitch);
        pitch_hold_ref = motor_pitch->measure.total_angle;
        pitch_release_hold_ref = pitch_hold_ref;
    }

    ResetPitchState();
    current_mode = GIMBAL_FOLLOW_CHASSIS;
    last_mode_logged = current_mode;
}

void Gimbal_Update(Input_Data_t *input, float chassis_wz)
{
    uint32_t now_ms;
    float dt_s = 0.02f;
    float yaw_speed_cmd;
    float pitch_speed_cmd;
    Gimbal_Cmd_t cmd = {0};
    (void)chassis_wz;

    if (input == NULL || !input->online || input->emergency_stop) {
        MDBG_GMB("stop by input offline/estop");
        Gimbal_Stop();
        return;
    }

    if (motor_yaw == NULL || motor_pitch == NULL) {
        return;
    }

    now_ms = HAL_GetTick();
    if (gimbal_last_tick != 0U) {
        dt_s = (float)(now_ms - gimbal_last_tick) / 1000.0f;
        dt_s = ClampFloat(dt_s, 0.001f, 0.05f);
    }
    gimbal_last_tick = now_ms;

    if (!gimbal_enabled) {
        DJIMotorEnable(motor_yaw);
        DJIMotorEnable(motor_pitch);
        gimbal_enabled = 1U;
        yaw_hold_ref = (gimbal_imu != NULL) ? gimbal_imu->YawTotalAngle : motor_yaw->measure.total_angle;
        pitch_hold_ref = motor_pitch->measure.total_angle;
        pitch_release_hold_ref = pitch_hold_ref;
        ResetPitchState();
    }

    current_mode = input->gimbal_mode;
    if (current_mode != last_mode_logged) {
        MDBG_GMB("mode switch %u -> %u", (unsigned)last_mode_logged, (unsigned)current_mode);
        last_mode_logged = current_mode;
    }

    UpdatePitchGravityFeedforward();
    GetPitchSpeedFeedback();

    yaw_speed_cmd = float_constrain(input->yaw_speed, -GM6020_SPEED_MAX, GM6020_SPEED_MAX);
    if (fabsf(yaw_speed_cmd) < SelectYawDeadzone(input)) {
        yaw_speed_cmd = 0.0f;
    }
    yaw_hold_ref += yaw_speed_cmd * dt_s;

    DJIMotorChangeFeed(motor_yaw, ANGLE_LOOP, (gimbal_imu != NULL) ? OTHER_FEED : MOTOR_FEED);
    DJIMotorChangeFeed(motor_yaw, SPEED_LOOP, (gimbal_imu != NULL) ? OTHER_FEED : MOTOR_FEED);
    DJIMotorOuterLoop(motor_yaw, ANGLE_LOOP);
    DJIMotorSetRef(motor_yaw, yaw_hold_ref);

    pitch_speed_cmd = float_constrain(input->pitch_speed, -GM6020_SPEED_MAX, GM6020_SPEED_MAX);
    if (fabsf(pitch_speed_cmd) < SelectPitchDeadzone(input)) {
        pitch_speed_cmd = 0.0f;
    }

    UpdatePitchControlMode(input, pitch_speed_cmd, now_ms);

    if (pitch_ctrl_mode == AXIS_CTRL_SPEED) {
        int16_t pitch_abs = (input->active_input == INPUT_ACTIVE_ET08)
                                ? ((input->rc_raw.right_y >= 0) ? input->rc_raw.right_y : -input->rc_raw.right_y)
                                : (int16_t)ClampFloat(fabsf(pitch_speed_cmd) / SelectPitchSpeedFullScale(input) *
                                                          RC_STICK_SCALE,
                                                      0.0f, RC_STICK_SCALE);
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorSetRef(motor_pitch, pitch_speed_cmd);
        if (fabsf(pitch_speed_cmd) >= 300.0f && pitch_abs >= 220) {
            pitch_hold_ref = motor_pitch->measure.total_angle;
        }
    } else if (pitch_ctrl_mode == AXIS_CTRL_BRAKE) {
        DJIMotorChangeFeed(motor_pitch, SPEED_LOOP, (gimbal_imu != NULL) ? OTHER_FEED : MOTOR_FEED);
        DJIMotorOuterLoop(motor_pitch, SPEED_LOOP);
        DJIMotorSetRef(motor_pitch, 0.0f);
    } else {
        DJIMotorChangeFeed(motor_pitch, ANGLE_LOOP, MOTOR_FEED);
        DJIMotorChangeFeed(motor_pitch, SPEED_LOOP, (gimbal_imu != NULL) ? OTHER_FEED : MOTOR_FEED);
        DJIMotorOuterLoop(motor_pitch, ANGLE_LOOP);
        DJIMotorSetRef(motor_pitch, pitch_hold_ref);
    }

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
        MDBG_GMB("mode=%u pitch_loop=%u yaw_ref=%ld yaw_imu=%ld off=%ld pitch_ref=%ld pitch_enc=%ld pitch_imu_spd=%ld",
                 (unsigned)current_mode,
                 (unsigned)pitch_ctrl_mode,
                 (long)(yaw_hold_ref * 10.0f),
                 (long)(Gimbal_GetYawIMUAngle() * 10.0f),
                 (long)(Gimbal_GetYawOffsetLogicDeg() * 10.0f),
                 (long)(pitch_hold_ref * 10.0f),
                 (long)(Gimbal_GetPitchEncoderAngle() * 10.0f),
                 (long)(GetPitchSpeedFeedback() * 10.0f));
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
    pitch_imu_speed_fdb = 0.0f;
    ResetPitchState();
    ResetPitchPidRuntime();

    g_robot.gimbal.control_mode = CTRL_ZERO_FORCE;
    g_robot.gimbal.ref_type = REF_SPEED;

    if (motor_yaw != NULL) {
        DJIMotorStop(motor_yaw);
    }
    if (motor_pitch != NULL) {
        DJIMotorStop(motor_pitch);
    }
}

GimbalMode_e Gimbal_GetMode(void)
{
    return current_mode;
}

float Gimbal_GetYawSpeedRef(void)
{
    return yaw_speed_ref_last;
}

float Gimbal_GetYawSpeedFdb(void)
{
    if (gimbal_imu != NULL) {
        return gimbal_imu->Gyro[2];
    }
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return motor_yaw->measure.speed_aps;
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
    return motor_yaw->measure.total_angle;
}

float Gimbal_GetPitchEncoderAngle(void)
{
    if (motor_pitch == NULL) {
        return 0.0f;
    }
    return motor_pitch->measure.total_angle;
}

float Gimbal_GetYawIMUAngle(void)
{
    if (gimbal_imu == NULL) {
        return 0.0f;
    }
    return gimbal_imu->YawTotalAngle;
}

float Gimbal_GetPitchIMUAngle(void)
{
    if (gimbal_imu == NULL) {
        return 0.0f;
    }
    return gimbal_imu->Pitch;
}

float Gimbal_GetYawOffsetRawDeg(void)
{
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return WrapAngleDeg180(motor_yaw->measure.angle_single_round - YAW_ALIGN_ANGLE_DEG);
}

float Gimbal_GetYawOffsetLogicDeg(void)
{
    return WrapAngleDeg180(Gimbal_GetYawOffsetRawDeg() - YAW_OFFSET_LOGIC_ZERO_DEG);
}

float Gimbal_GetYawRelativeSpeedDeg(void)
{
    if (motor_yaw == NULL) {
        return 0.0f;
    }
    return motor_yaw->measure.speed_aps;
}

float Gimbal_GetYawLogicAngle(void)
{
    if (gimbal_imu == NULL) {
        return 0.0f;
    }
    return WrapAngleDeg180(gimbal_imu->YawTotalAngle - IMU_YAW_LOGIC_ZERO_TOTAL_DEG);
}

AxisCtrlMode_e Gimbal_GetPitchCtrlMode(void)
{
    return pitch_ctrl_mode;
}
