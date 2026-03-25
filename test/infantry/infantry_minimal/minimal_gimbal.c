/**
 * @file minimal_gimbal.c
 * @brief 云台控制模块实现
 *
 * 模式定义:
 * - GIMBAL_FOLLOW_CHASSIS: Yaw使用编码器角度闭环,与底盘强耦合
 * - GIMBAL_SEPARATE: Yaw/Pitch使用IMU角度闭环,无遥控输入时保持朝向
 */

#include "minimal_gimbal.h"
#include "minimal_config.h"
#include "minimal_types.h"
#include "minimal_debug.h"
#include "dji_motor.h"
#include "user_lib.h"
#include "ins_task.h"
#include "can.h"
#include <math.h>

/*============================================================================
 * 私有变量
 *============================================================================*/
static DJIMotorInstance *motor_yaw = NULL;
static DJIMotorInstance *motor_pitch = NULL;
static attitude_t *gimbal_imu = NULL;

static GimbalMode_e current_mode = GIMBAL_FOLLOW_CHASSIS;
static uint8_t gimbal_enabled = 0U;
static uint32_t gimbal_last_tick = 0U;
static float yaw_speed_ref_last = 0.0f;
static float pitch_speed_ref_last = 0.0f;
static GimbalMode_e last_mode_logged = GIMBAL_FOLLOW_CHASSIS;
static float yaw_follow_target_angle = 0.0f;     /* 编码器总角度 */
static float pitch_follow_target_angle = 0.0f;   /* 编码器总角度 */
static float yaw_separate_target_angle = 0.0f;   /* IMU yaw总角度 */
static float pitch_separate_target_angle = 0.0f; /* IMU pitch角度 */
static float yaw_separate_center = 0.0f;
static uint32_t gimbal_debug_last_tick = 0U;

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

/*============================================================================
 * 公共函数
 *============================================================================*/
void Gimbal_Init(void)
{
    gimbal_imu = INS_Init();

    Motor_Init_Config_s yaw_config = {
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
    motor_yaw = DJIMotorInit(&yaw_config);
    if (motor_yaw) {
        DJIMotorOuterLoop(motor_yaw, GIMBAL_YAW_INIT_LOOP);
        DJIMotorStop(motor_yaw);
    }
    
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
            .other_angle_feedback_ptr = (gimbal_imu != NULL) ? &gimbal_imu->Pitch : NULL,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };
    motor_pitch = DJIMotorInit(&pitch_config);
    if (motor_pitch) {
        DJIMotorOuterLoop(motor_pitch, GIMBAL_PITCH_INIT_LOOP);
        DJIMotorStop(motor_pitch);
    }
    
    if (motor_yaw != NULL) {
        yaw_follow_target_angle = motor_yaw->measure.total_angle;
    }
    if (motor_pitch != NULL) {
        pitch_follow_target_angle = motor_pitch->measure.total_angle;
    }
    if (gimbal_imu != NULL) {
        yaw_separate_target_angle = gimbal_imu->YawTotalAngle;
        yaw_separate_center = gimbal_imu->YawTotalAngle;
        pitch_separate_target_angle = gimbal_imu->Pitch;
    } else {
        yaw_separate_target_angle = yaw_follow_target_angle;
        yaw_separate_center = yaw_follow_target_angle;
        pitch_separate_target_angle = 0.0f;
    }
    last_mode_logged = current_mode;
}

void Gimbal_Update(Input_Data_t *input, float chassis_wz)
{
    Gimbal_Cmd_t cmd = {0};
    float dt = 0.02f;
    uint32_t now = HAL_GetTick();
    float yaw_speed_cmd;
    float pitch_speed_cmd;
    (void)chassis_wz;

    if (input == NULL || !input->online || input->emergency_stop) {
        MDBG_GMB("stop by input offline/estop");
        Gimbal_Stop();
        return;
    }
    if (motor_yaw == NULL || motor_pitch == NULL) {
        return;
    }
    if (gimbal_last_tick != 0U) {
        dt = (float)(now - gimbal_last_tick) / 1000.0f;
        dt = float_constrain(dt, 0.001f, 0.05f);
    }
    gimbal_last_tick = now;

    if (!gimbal_enabled) {
        DJIMotorEnable(motor_yaw);
        DJIMotorEnable(motor_pitch);
        gimbal_enabled = 1U;
    }
    
    current_mode = input->gimbal_mode;
    if (current_mode != last_mode_logged) {
        MDBG_GMB("mode switch %u -> %u", (unsigned)last_mode_logged, (unsigned)current_mode);
        if (current_mode == GIMBAL_FOLLOW_CHASSIS) {
            yaw_follow_target_angle = motor_yaw->measure.total_angle;
            pitch_follow_target_angle = motor_pitch->measure.total_angle;
            MDBG_GMB("follow targets reset yaw_x10=%ld pitch_x10=%ld",
                     (long)(yaw_follow_target_angle * 10.0f),
                     (long)(pitch_follow_target_angle * 10.0f));
        } else {
            if (gimbal_imu != NULL) {
                yaw_separate_target_angle = gimbal_imu->YawTotalAngle;
                yaw_separate_center = gimbal_imu->YawTotalAngle;
                pitch_separate_target_angle = gimbal_imu->Pitch;
                MDBG_GMB("separate targets reset imu_yaw_x10=%ld imu_pitch_x10=%ld",
                         (long)(yaw_separate_target_angle * 10.0f),
                         (long)(pitch_separate_target_angle * 10.0f));
            } else {
                yaw_separate_target_angle = motor_yaw->measure.total_angle;
                yaw_separate_center = yaw_separate_target_angle;
                pitch_separate_target_angle = 0.0f;
                MDBG_GMB("separate no imu fallback yaw_x10=%ld pitch_x10=%ld",
                         (long)(yaw_separate_target_angle * 10.0f),
                         (long)(pitch_separate_target_angle * 10.0f));
            }
        }
        last_mode_logged = current_mode;
    }
    cmd.mode = current_mode;
    cmd.control_mode = CTRL_ENABLE;

    yaw_speed_cmd = float_constrain(input->yaw_speed, -GM6020_SPEED_MAX, GM6020_SPEED_MAX);
    if (fabsf(yaw_speed_cmd) < GIMBAL_SPEED_DEADZONE) {
        yaw_speed_cmd = 0.0f;
    }
    pitch_speed_cmd = float_constrain(input->pitch_speed, -GM6020_SPEED_MAX, GM6020_SPEED_MAX);
    if (fabsf(pitch_speed_cmd) < GIMBAL_SPEED_DEADZONE) {
        pitch_speed_cmd = 0.0f;
    }

    if (current_mode == GIMBAL_FOLLOW_CHASSIS) {
        /* Follow: yaw锁定在编码器角度，底盘承担转向 */
        DJIMotorChangeFeed(motor_yaw, ANGLE_LOOP, MOTOR_FEED);
        DJIMotorOuterLoop(motor_yaw, GIMBAL_YAW_RUN_LOOP_FOLLOW);
        DJIMotorSetRef(motor_yaw, yaw_follow_target_angle);

        /* Follow下pitch改为角度控制: 遥控速度积分成目标角 */
        pitch_follow_target_angle += pitch_speed_cmd * dt;
        DJIMotorChangeFeed(motor_pitch, ANGLE_LOOP, MOTOR_FEED);
        DJIMotorOuterLoop(motor_pitch, GIMBAL_PITCH_RUN_LOOP_HOLD);
        DJIMotorSetRef(motor_pitch, pitch_follow_target_angle);

        cmd.ref_type = REF_ANGLE;
        cmd.yaw_angle = yaw_follow_target_angle;
        cmd.yaw_speed = 0.0f;
        cmd.pitch_speed = pitch_speed_cmd;
        cmd.pitch_angle = pitch_follow_target_angle;
        cmd.manual_pitch = 0U;
        yaw_speed_ref_last = 0.0f;
        pitch_speed_ref_last = pitch_speed_cmd;
    } else {
        /* Separate: yaw/pitch都使用IMU角度闭环，遥控只更新目标 */
        if (gimbal_imu != NULL) {
            yaw_separate_target_angle += yaw_speed_cmd * dt;
            yaw_separate_target_angle =
                ClampFloat(yaw_separate_target_angle,
                           yaw_separate_center - GIMBAL_SEPARATE_YAW_MAX_ANGLE,
                           yaw_separate_center + GIMBAL_SEPARATE_YAW_MAX_ANGLE);

            pitch_separate_target_angle += pitch_speed_cmd * dt;
            pitch_separate_target_angle =
                ClampFloat(pitch_separate_target_angle,
                           GIMBAL_SEPARATE_PITCH_MIN_ANGLE,
                           GIMBAL_SEPARATE_PITCH_MAX_ANGLE);

            DJIMotorChangeFeed(motor_yaw, ANGLE_LOOP, OTHER_FEED);
            DJIMotorOuterLoop(motor_yaw, GIMBAL_YAW_RUN_LOOP_SEPARATE);
            DJIMotorSetRef(motor_yaw, yaw_separate_target_angle);

            DJIMotorChangeFeed(motor_pitch, ANGLE_LOOP, OTHER_FEED);
            DJIMotorOuterLoop(motor_pitch, GIMBAL_PITCH_RUN_LOOP_HOLD);
            DJIMotorSetRef(motor_pitch, pitch_separate_target_angle);

            cmd.ref_type = REF_ANGLE;
            cmd.yaw_angle = yaw_separate_target_angle;
            cmd.pitch_angle = pitch_separate_target_angle;
            cmd.yaw_speed = yaw_speed_cmd;
            cmd.pitch_speed = pitch_speed_cmd;
            cmd.manual_pitch = 0U;
        } else {
            /* 无IMU时回退到编码器角度模式 */
            yaw_separate_target_angle += yaw_speed_cmd * dt;
            pitch_follow_target_angle += pitch_speed_cmd * dt;

            DJIMotorChangeFeed(motor_yaw, ANGLE_LOOP, MOTOR_FEED);
            DJIMotorOuterLoop(motor_yaw, GIMBAL_YAW_RUN_LOOP_SEPARATE);
            DJIMotorSetRef(motor_yaw, yaw_separate_target_angle);

            DJIMotorChangeFeed(motor_pitch, ANGLE_LOOP, MOTOR_FEED);
            DJIMotorOuterLoop(motor_pitch, GIMBAL_PITCH_RUN_LOOP_HOLD);
            DJIMotorSetRef(motor_pitch, pitch_follow_target_angle);

            cmd.ref_type = REF_ANGLE;
            cmd.yaw_angle = yaw_separate_target_angle;
            cmd.pitch_angle = pitch_follow_target_angle;
            cmd.yaw_speed = yaw_speed_cmd;
            cmd.pitch_speed = pitch_speed_cmd;
            cmd.manual_pitch = 0U;
        }
        yaw_speed_ref_last = yaw_speed_cmd;
        pitch_speed_ref_last = pitch_speed_cmd;
    }
    g_robot.gimbal = cmd;

    if ((now - gimbal_debug_last_tick) >= GIMBAL_DEBUG_DETAIL_PERIOD_MS) {
        gimbal_debug_last_tick = now;
        MDBG_GMB("mode=%u in(yaw_spd_x10=%ld pitch_spd_x10=%ld) tgt(yaw_x10=%ld pitch_x10=%ld) enc(yaw_x10=%ld pitch_x10=%ld) imu(yaw_x10=%ld pitch_x10=%ld)",
                 (unsigned)current_mode,
                 (long)(yaw_speed_cmd * 10.0f),
                 (long)(pitch_speed_cmd * 10.0f),
                 (long)(((current_mode == GIMBAL_FOLLOW_CHASSIS) ? yaw_follow_target_angle : yaw_separate_target_angle) * 10.0f),
                 (long)(((current_mode == GIMBAL_FOLLOW_CHASSIS) ? pitch_follow_target_angle : pitch_separate_target_angle) * 10.0f),
                 (long)(Gimbal_GetYawEncoderAngle() * 10.0f),
                 (long)(Gimbal_GetPitchEncoderAngle() * 10.0f),
                 (long)(Gimbal_GetYawIMUAngle() * 10.0f),
                 (long)(Gimbal_GetPitchIMUAngle() * 10.0f));
    }
}

void Gimbal_Stop(void)
{
    gimbal_enabled = 0U;
    gimbal_last_tick = 0U;
    gimbal_debug_last_tick = 0U;
    yaw_speed_ref_last = 0.0f;
    pitch_speed_ref_last = 0.0f;
    g_robot.gimbal.control_mode = CTRL_ZERO_FORCE;
    g_robot.gimbal.ref_type = REF_SPEED;
    
    if (motor_yaw) DJIMotorStop(motor_yaw);
    if (motor_pitch) DJIMotorStop(motor_pitch);
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
    if (motor_pitch == NULL) {
        return 0.0f;
    }
    return motor_pitch->measure.speed_aps;
}

float Gimbal_GetPitchTargetAngle(void)
{
    if (current_mode == GIMBAL_SEPARATE) {
        return pitch_separate_target_angle;
    }
    return pitch_follow_target_angle;
}

float Gimbal_GetYawTargetAngle(void)
{
    if (current_mode == GIMBAL_SEPARATE) {
        return yaw_separate_target_angle;
    }
    return yaw_follow_target_angle;
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
