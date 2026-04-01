/**
 * @file minimal_chassis.c
 * @brief 底盘控制模块实现
 *
 * 迁移基准:
 * - 输入平移指令始终定义在云台坐标系
 * - 底盘使用 yaw 编码器相对角完成坐标变换
 * - FOLLOW 模式通过相对夹角闭环追随云台
 */

#include "minimal_chassis.h"

#include <math.h>
#include <string.h>

#include "can.h"
#include "dji_motor.h"
#include "minimal_config.h"
#include "minimal_debug.h"
#include "minimal_gimbal.h"
#include "minimal_referee.h"
#include "minimal_types.h"
#include "user_lib.h"

static DJIMotorInstance *motor_fr = NULL;
static DJIMotorInstance *motor_fl = NULL;
static DJIMotorInstance *motor_br = NULL;
static DJIMotorInstance *motor_bl = NULL;

static float last_wz = 0.0f;
static uint8_t chassis_enabled = 0U;
static float last_wheel_ref[4] = {0.0f};
static float last_power_scale = 1.0f;
static float filtered_vx = 0.0f;
static float filtered_vy = 0.0f;
static float filtered_wz = 0.0f;
static float follow_wz_integral = 0.0f;
static uint32_t chassis_last_tick = 0U;

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

static void OmniInverseKinematics(float vx, float vy, float wz, float out[4])
{
    const float l = CHASSIS_WHEEL_BASE * 0.5f;
    const float v_fr = vx - vy - l * wz;
    const float v_fl = vx + vy - l * wz;
    const float v_br = -vx - vy - l * wz;
    const float v_bl = -vx + vy - l * wz;

    out[0] = v_fr / CHASSIS_WHEEL_RADIUS;
    out[1] = v_fl / CHASSIS_WHEEL_RADIUS;
    out[2] = v_br / CHASSIS_WHEEL_RADIUS;
    out[3] = v_bl / CHASSIS_WHEEL_RADIUS;
}

void Chassis_Init(void)
{
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

    motor_fr = DJIMotorInit(&config);
    if (motor_fr != NULL) {
        DJIMotorOuterLoop(motor_fr, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_fr);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_FL_ID;
    motor_fl = DJIMotorInit(&config);
    if (motor_fl != NULL) {
        DJIMotorOuterLoop(motor_fl, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_fl);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_BR_ID;
    motor_br = DJIMotorInit(&config);
    if (motor_br != NULL) {
        DJIMotorOuterLoop(motor_br, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_br);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_BL_ID;
    motor_bl = DJIMotorInit(&config);
    if (motor_bl != NULL) {
        DJIMotorOuterLoop(motor_bl, CHASSIS_INIT_LOOP);
        DJIMotorStop(motor_bl);
    }
}

void Chassis_Update(Input_Data_t *input)
{
    uint32_t now_ms;
    float dt_s = MAIN_LOOP_PERIOD_MS / 1000.0f;
    float yaw_offset_deg;
    float theta_deg;
    float cos_theta;
    float sin_theta;
    float manual_wz;
    float follow_wz = 0.0f;
    float wheel_speed_rad_s[4] = {0.0f};
    float power_scale;
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

    now_ms = HAL_GetTick();
    if (chassis_last_tick != 0U) {
        dt_s = ClampFloat((float)(now_ms - chassis_last_tick) / 1000.0f, 0.001f, 0.05f);
    }
    chassis_last_tick = now_ms;

    yaw_offset_deg = Gimbal_GetYawOffsetLogicDeg();
    theta_deg = -yaw_offset_deg;
    cos_theta = cosf(theta_deg * (float)M_PI / 180.0f);
    sin_theta = sinf(theta_deg * (float)M_PI / 180.0f);

    cmd.vx_cmd = input->vx;
    cmd.vy_cmd = input->vy;
    cmd.vx = cmd.vx_cmd * cos_theta + cmd.vy_cmd * sin_theta;
    cmd.vy = -cmd.vx_cmd * sin_theta + cmd.vy_cmd * cos_theta;
    cmd.yaw_offset_deg = yaw_offset_deg;
    cmd.mode = (input->gimbal_mode == GIMBAL_FOLLOW_CHASSIS) ? CHASSIS_FOLLOW : CHASSIS_NO_FOLLOW;
    cmd.spin_enable = input->spin_enable;
    cmd.control_mode = CTRL_ENABLE;
    cmd.ref_type = REF_SPEED;

    if (fabsf(cmd.vx) < CHASSIS_DEADZONE_VX) {
        cmd.vx = 0.0f;
    }
    if (fabsf(cmd.vy) < CHASSIS_DEADZONE_VY) {
        cmd.vy = 0.0f;
    }

    manual_wz = input->wz;
    if (input->active_input != INPUT_ACTIVE_VT) {
        manual_wz = 0.0f;
    }

    if (input->gimbal_mode == GIMBAL_FOLLOW_CHASSIS) {
        if (fabsf(CHASSIS_FOLLOW_WZ_KI) > 1e-6f) {
            float integral_limit = CHASSIS_FOLLOW_WZ_I_MAX / fabsf(CHASSIS_FOLLOW_WZ_KI);
            follow_wz_integral += yaw_offset_deg * dt_s;
            follow_wz_integral = ClampFloat(follow_wz_integral, -integral_limit, integral_limit);
        } else {
            follow_wz_integral = 0.0f;
        }

        follow_wz =
            -(CHASSIS_FOLLOW_WZ_KP * yaw_offset_deg +
              CHASSIS_FOLLOW_WZ_KI * follow_wz_integral +
              CHASSIS_FOLLOW_WZ_KD * Gimbal_GetYawRelativeSpeedDeg()) *
            ((float)M_PI / 180.0f);
        follow_wz = ClampFloat(follow_wz, -CHASSIS_FOLLOW_WZ_MAX, CHASSIS_FOLLOW_WZ_MAX);
        cmd.wz = follow_wz + manual_wz;
    } else {
        follow_wz_integral = 0.0f;
        cmd.wz = manual_wz;
        if (input->spin_enable != 0U) {
            cmd.wz += SPIN_ROTATE_SPEED_RAD_S;
        }
    }

    if (fabsf(cmd.wz) < CHASSIS_DEADZONE_WZ) {
        cmd.wz = 0.0f;
    }
    cmd.wz = ClampFloat(cmd.wz, -CHASSIS_MAX_WZ, CHASSIS_MAX_WZ);

    g_robot.chassis = cmd;
    last_wz = cmd.wz;

    power_scale = MinimalReferee_ChassisScale();
    last_power_scale = power_scale;

    if (fabsf(cmd.vx) < CHASSIS_DEADZONE_VX && fabsf(cmd.vy) < CHASSIS_DEADZONE_VY &&
        fabsf(cmd.wz) < CHASSIS_DEADZONE_WZ) {
        filtered_vx = 0.0f;
        filtered_vy = 0.0f;
        filtered_wz = 0.0f;
    } else {
        filtered_vx = filtered_vx * CHASSIS_SPEED_FILTER_COEF + cmd.vx * (1.0f - CHASSIS_SPEED_FILTER_COEF);
        filtered_vy = filtered_vy * CHASSIS_SPEED_FILTER_COEF + cmd.vy * (1.0f - CHASSIS_SPEED_FILTER_COEF);
        filtered_wz = filtered_wz * CHASSIS_SPEED_FILTER_COEF + cmd.wz * (1.0f - CHASSIS_SPEED_FILTER_COEF);
    }

    OmniInverseKinematics(filtered_vx, filtered_vy, filtered_wz, wheel_speed_rad_s);

    for (uint8_t i = 0; i < 4U; i++) {
        float speed_dps = wheel_speed_rad_s[i] * 180.0f / (float)M_PI;
        float speed_deadzone =
            (input->gimbal_mode == GIMBAL_FOLLOW_CHASSIS) ? CHASSIS_FOLLOW_SPEED_DEADZONE : CHASSIS_SPEED_DEADZONE;
        speed_dps *= CHASSIS_SPEED_SCALE * power_scale;
        if (fabsf(speed_dps) < speed_deadzone) {
            speed_dps = 0.0f;
        }
        last_wheel_ref[i] = ClampFloat(speed_dps, -M3508_SPEED_MAX, M3508_SPEED_MAX);
    }

    if (!chassis_enabled) {
        if (motor_fr != NULL) {
            DJIMotorEnable(motor_fr);
        }
        if (motor_fl != NULL) {
            DJIMotorEnable(motor_fl);
        }
        if (motor_br != NULL) {
            DJIMotorEnable(motor_br);
        }
        if (motor_bl != NULL) {
            DJIMotorEnable(motor_bl);
        }
        chassis_enabled = 1U;
    }

    if (motor_fr != NULL) {
        DJIMotorOuterLoop(motor_fr, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_fr, last_wheel_ref[0]);
    }
    if (motor_fl != NULL) {
        DJIMotorOuterLoop(motor_fl, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_fl, last_wheel_ref[1]);
    }
    if (motor_br != NULL) {
        DJIMotorOuterLoop(motor_br, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_br, last_wheel_ref[2]);
    }
    if (motor_bl != NULL) {
        DJIMotorOuterLoop(motor_bl, CHASSIS_RUN_LOOP_NORMAL);
        DJIMotorSetRef(motor_bl, last_wheel_ref[3]);
    }

    MDBG_CHS("mode=%u spin=%u off=%ld cmd(vx=%ld vy=%ld wz=%ld) filt(vx=%ld vy=%ld wz=%ld)",
             (unsigned)cmd.mode,
             (unsigned)cmd.spin_enable,
             (long)(cmd.yaw_offset_deg * 10.0f),
             (long)(cmd.vx * 1000.0f),
             (long)(cmd.vy * 1000.0f),
             (long)(cmd.wz * 1000.0f),
             (long)(filtered_vx * 1000.0f),
             (long)(filtered_vy * 1000.0f),
             (long)(filtered_wz * 1000.0f));
}

void Chassis_Stop(void)
{
    last_wz = 0.0f;
    chassis_enabled = 0U;
    last_power_scale = 0.0f;
    filtered_vx = 0.0f;
    filtered_vy = 0.0f;
    filtered_wz = 0.0f;
    follow_wz_integral = 0.0f;
    chassis_last_tick = 0U;
    memset(last_wheel_ref, 0, sizeof(last_wheel_ref));

    g_robot.chassis.control_mode = CTRL_ZERO_FORCE;
    g_robot.chassis.ref_type = REF_SPEED;

    if (motor_fr != NULL) {
        DJIMotorSetRef(motor_fr, 0.0f);
        DJIMotorStop(motor_fr);
    }
    if (motor_fl != NULL) {
        DJIMotorSetRef(motor_fl, 0.0f);
        DJIMotorStop(motor_fl);
    }
    if (motor_br != NULL) {
        DJIMotorSetRef(motor_br, 0.0f);
        DJIMotorStop(motor_br);
    }
    if (motor_bl != NULL) {
        DJIMotorSetRef(motor_bl, 0.0f);
        DJIMotorStop(motor_bl);
    }
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
