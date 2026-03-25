/**
 * @file minimal_chassis.c
 * @brief 底盘控制模块实现 - 十字全向轮
 */

#include "minimal_chassis.h"
#include "minimal_config.h"
#include "minimal_types.h"
#include "minimal_referee.h"
#include "minimal_debug.h"
#include "dji_motor.h"
#include "user_lib.h"
#include "can.h"
#include <math.h>
#include <string.h>

/*============================================================================
 * 私有变量
 *============================================================================*/
static DJIMotorInstance *motor_fr = NULL;
static DJIMotorInstance *motor_fl = NULL;
static DJIMotorInstance *motor_br = NULL;
static DJIMotorInstance *motor_bl = NULL;

static float last_wz = 0.0f;  // 供云台使用
static uint8_t chassis_enabled = 0U;
static float last_wheel_ref[4] = {0.0f};
static float last_power_scale = 1.0f;
static float filtered_vx = 0.0f;
static float filtered_vy = 0.0f;
static float filtered_wz = 0.0f;

/*============================================================================
 * 十字全向轮运动学
 *============================================================================*/
/**
 * @brief 十字全向轮逆运动学
 * @param vx 横向速度 (m/s, 左为正)
 * @param vy 纵向速度 (m/s, 前为正)
 * @param wz 旋转角速度 (rad/s)
 * @param out 4个轮子速度输出 (rad/s): FR, FL, BR, BL
 */
static void OmniInverseKinematics(float vx, float vy, float wz, float out[4])
{
    float L = CHASSIS_WHEEL_BASE / 2.0f;
    float v_fr = vy - vx - (L * wz);
    float v_fl = vy + vx - (L * wz);
    float v_br = -vy + vx - (L * wz);
    float v_bl = -vy - vx - (L * wz);

    out[0] = v_fr / CHASSIS_WHEEL_RADIUS;
    out[1] = v_fl / CHASSIS_WHEEL_RADIUS;
    out[2] = v_br / CHASSIS_WHEEL_RADIUS;
    out[3] = v_bl / CHASSIS_WHEEL_RADIUS;
}

/*============================================================================
 * 公共函数
 *============================================================================*/
void Chassis_Init(void)
{
    // 底盘电机配置 - M3508速度环控制
    Motor_Init_Config_s config = {
        .motor_type = M3508,
        .can_init_config = {
            .can_handle = &CHASSIS_CAN,
            .tx_id = CHASSIS_MOTOR_FR_ID,
        },
        .controller_param_init_config = {
            .speed_PID = {
                .Kp = CHASSIS_SPEED_KP,
                .Ki = CHASSIS_SPEED_KI,
                .Kd = CHASSIS_SPEED_KD,
                .MaxOut = CHASSIS_SPEED_MAX_OUT,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
            },
            .current_PID = {
                .Kp = 0.35f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .MaxOut = 8000.0f,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = SPEED_LOOP | CURRENT_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };
    
    // 前右
    motor_fr = DJIMotorInit(&config);
    if (motor_fr) {
        DJIMotorOuterLoop(motor_fr, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_fr);
    }
    
    // 前左
    config.can_init_config.tx_id = CHASSIS_MOTOR_FL_ID;
    motor_fl = DJIMotorInit(&config);
    if (motor_fl) {
        DJIMotorOuterLoop(motor_fl, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_fl);
    }
    
    // 后右
    config.can_init_config.tx_id = CHASSIS_MOTOR_BR_ID;
    motor_br = DJIMotorInit(&config);
    if (motor_br) {
        DJIMotorOuterLoop(motor_br, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_br);
    }
    
    // 后左
    config.can_init_config.tx_id = CHASSIS_MOTOR_BL_ID;
    motor_bl = DJIMotorInit(&config);
    if (motor_bl) {
        DJIMotorOuterLoop(motor_bl, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_bl);
    }
}

void Chassis_Update(Input_Data_t *input)
{
    Chassis_Cmd_t cmd = {0};

    if (input == NULL || !input->online || input->emergency_stop) {
        MDBG_CHS("stop by input offline/estop");
        Chassis_Stop();
        return;
    }
    if (!MinimalReferee_AllowChassis()) {
        MDBG_CHS("stop by referee lock");
        Chassis_Stop();
        return;
    }

    cmd.vx = input->vx;
    cmd.vy = input->vy;
    cmd.wz = input->wz;
    cmd.mode = (input->gimbal_mode == GIMBAL_FOLLOW_CHASSIS) ? CHASSIS_FOLLOW : CHASSIS_NO_FOLLOW;
    cmd.control_mode = CTRL_ENABLE;
    cmd.ref_type = REF_SPEED;
    g_robot.chassis = cmd;
    
    float vx = cmd.vx;
    float vy = cmd.vy;
    float wz = cmd.wz;
    float power_scale = MinimalReferee_ChassisScale();
    last_power_scale = power_scale;

    if (input->gimbal_mode == GIMBAL_FOLLOW_CHASSIS) {
        wz += input->yaw_speed * CHASSIS_FOLLOW_YAW_TO_WZ_GAIN;
    }
    
    // 保存供云台使用
    last_wz = wz;
    
    // 速度限幅
    vx = float_constrain(vx, -CHASSIS_MAX_VX, CHASSIS_MAX_VX);
    vy = float_constrain(vy, -CHASSIS_MAX_VY, CHASSIS_MAX_VY);
    wz = float_constrain(wz, -CHASSIS_MAX_WZ, CHASSIS_MAX_WZ);

    // 与omni_demo对齐: 输入死区 + 一阶滤波，避免抖动与突变
    if (fabsf(vx) < CHASSIS_DEADZONE_VX && fabsf(vy) < CHASSIS_DEADZONE_VY && fabsf(wz) < CHASSIS_DEADZONE_WZ) {
        vx = 0.0f;
        vy = 0.0f;
        wz = 0.0f;
        filtered_vx = 0.0f;
        filtered_vy = 0.0f;
        filtered_wz = 0.0f;
    } else {
        filtered_vx = filtered_vx * CHASSIS_SPEED_FILTER_COEF + vx * (1.0f - CHASSIS_SPEED_FILTER_COEF);
        filtered_vy = filtered_vy * CHASSIS_SPEED_FILTER_COEF + vy * (1.0f - CHASSIS_SPEED_FILTER_COEF);
        filtered_wz = filtered_wz * CHASSIS_SPEED_FILTER_COEF + wz * (1.0f - CHASSIS_SPEED_FILTER_COEF);
        vx = filtered_vx;
        vy = filtered_vy;
        wz = filtered_wz;
    }
    
    // 全向轮逆运动学
    float wheel_speeds[4];
    OmniInverseKinematics(vx, vy, wz, wheel_speeds);
    
    // 转换为deg/s并发送给电机
    for (int i = 0; i < 4; i++) {
        wheel_speeds[i] = wheel_speeds[i] * 180.0f / PI;  // rad/s -> deg/s
        wheel_speeds[i] *= CHASSIS_SPEED_SCALE * power_scale;
        if (fabsf(wheel_speeds[i]) < CHASSIS_SPEED_DEADZONE) {
            wheel_speeds[i] = 0.0f;
        }
        wheel_speeds[i] = float_constrain(wheel_speeds[i], -M3508_SPEED_MAX, M3508_SPEED_MAX);
        last_wheel_ref[i] = wheel_speeds[i];
    }
    
    // 设置电机参考速度
    if (!chassis_enabled) {
        if (motor_fr) DJIMotorEnable(motor_fr);
        if (motor_fl) DJIMotorEnable(motor_fl);
        if (motor_br) DJIMotorEnable(motor_br);
        if (motor_bl) DJIMotorEnable(motor_bl);
        chassis_enabled = 1U;
    }
    if (motor_fr) {
        DJIMotorOuterLoop(motor_fr, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_fr, wheel_speeds[0]);
    }
    if (motor_fl) {
        DJIMotorOuterLoop(motor_fl, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_fl, wheel_speeds[1]);
    }
    if (motor_br) {
        DJIMotorOuterLoop(motor_br, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_br, wheel_speeds[2]);
    }
    if (motor_bl) {
        DJIMotorOuterLoop(motor_bl, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_bl, wheel_speeds[3]);
    }
}

void Chassis_Stop(void)
{
    last_wz = 0.0f;
    chassis_enabled = 0U;
    last_power_scale = 0.0f;
    filtered_vx = 0.0f;
    filtered_vy = 0.0f;
    filtered_wz = 0.0f;
    memset(last_wheel_ref, 0, sizeof(last_wheel_ref));
    g_robot.chassis.control_mode = CTRL_ZERO_FORCE;
    g_robot.chassis.ref_type = REF_SPEED;
    
    if (motor_fr) DJIMotorStop(motor_fr);
    if (motor_fl) DJIMotorStop(motor_fl);
    if (motor_br) DJIMotorStop(motor_br);
    if (motor_bl) DJIMotorStop(motor_bl);
}

float Chassis_GetWz(void)
{
    return last_wz;
}

float Chassis_GetFRSpeedRef(void)
{
    return last_wheel_ref[0];
}

float Chassis_GetFRSpeedFdb(void)
{
    if (motor_fr == NULL) {
        return 0.0f;
    }
    return motor_fr->measure.speed_aps;
}

float Chassis_GetPowerScale(void)
{
    return last_power_scale;
}
