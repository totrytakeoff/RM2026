/**
 * @file minimal_shoot.c
 * @brief 射击控制模块实现 - 摩擦轮+拨弹电机
 */

#include "infantry_shoot.h"
#include "infantry_config.h"
#include "infantry_types.h"
#include "infantry_referee.h"
#include "infantry_debug.h"
#include "dji_motor.h"
#include "rm_time.h"
#include "user_lib.h"
#include "can.h"
#include <math.h>

/*============================================================================
 * 私有变量
 *============================================================================*/
static DJIMotorInstance *motor_friction_l = NULL;
static DJIMotorInstance *motor_friction_r = NULL;
static DJIMotorInstance *motor_loader = NULL;

static ShootState_e shoot_state = SHOOT_OFF;
static float loader_target_angle = 0.0f;
static float loader_start_angle = 0.0f;
static uint8_t single_shot_active = 0;
static uint8_t pending_shots = 0;
static uint32_t last_shot_tick = 0;
static float loader_speed_cmd = 0.0f;
static uint32_t loader_speed_tick = 0U;
static uint32_t single_shot_start_ms = 0U;
static float loader_ref_last = 0.0f;
static uint8_t last_continuous_mode = 0U;

static bool PublishMotorCommand(DJIMotorInstance *motor,
                                Closeloop_Type_e outer_loop,
                                float reference,
                                Motor_Working_Type_e working_state,
                                bool update_working_state)
{
    DJIMotorCommand command;

    if (!DJIMotorGetCommand(motor, &command)) {
        return false;
    }
    command.settings.outer_loop_type = outer_loop;
    command.reference = reference;
    if (update_working_state) {
        command.working_state = working_state;
    }
    return DJIMotorPublishCommand(motor, &command);
}

static float LoaderTotalAngle(void)
{
    DJI_Motor_Measure_s measure;

    return (motor_loader != NULL &&
            DJIMotorGetMeasure(motor_loader, &measure))
               ? measure.total_angle
               : 0.0f;
}

static float LoaderSpeed(void)
{
    DJI_Motor_Measure_s measure;

    return (motor_loader != NULL &&
            DJIMotorGetMeasure(motor_loader, &measure))
               ? measure.speed_aps
               : 0.0f;
}

/*============================================================================
 * 公共函数
 *============================================================================*/
bool Shoot_Init(void)
{
    // 摩擦轮电机配置 - M3508速度环
    Motor_Init_Config_s friction_config = {
        .motor_type = M3508,
        .can_init_config = {
            .can_handle = &FRICTION_CAN,
            .tx_id = FRICTION_LEFT_ID,
        },
        .controller_param_init_config = {
            .speed_PID = {
                .Kp = FRICTION_SPEED_KP,
                .Ki = FRICTION_SPEED_KI,
                .Kd = FRICTION_SPEED_KD,
                .MaxOut = FRICTION_SPEED_MAX_OUT,
                .IntegralLimit = 3000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };
    motor_friction_l = DJIMotorInit(&friction_config);
    if (motor_friction_l) {
        (void)PublishMotorCommand(motor_friction_l, FRICTION_INIT_LOOP, 0.0f,
                                  MOTOR_STOP, true);
    }
    
    // 右摩擦轮(反向)
    friction_config.can_init_config.tx_id = FRICTION_RIGHT_ID;
    friction_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_REVERSE;
    motor_friction_r = DJIMotorInit(&friction_config);
    if (motor_friction_r) {
        (void)PublishMotorCommand(motor_friction_r, FRICTION_INIT_LOOP, 0.0f,
                                  MOTOR_STOP, true);
    }
    
    // 拨弹电机配置 - M2006位置环
    Motor_Init_Config_s loader_config = {
        .motor_type = M2006,
        .can_init_config = {
            .can_handle = &LOADER_CAN,
            .tx_id = LOADER_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = LOADER_ANGLE_KP,
                .Ki = 0.0f,
                .Kd = LOADER_ANGLE_KD,
                .MaxOut = LOADER_ANGLE_MAX_OUT,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
            },
            .speed_PID = {
                .Kp = LOADER_SPEED_KP,
                .Ki = 0.0f,
                .Kd = LOADER_SPEED_KD,
                .IntegralLimit = 3000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = LOADER_SPEED_MAX_OUT,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = LOADER_INIT_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };
    motor_loader = DJIMotorInit(&loader_config);
    if (motor_loader) {
        (void)PublishMotorCommand(motor_loader, LOADER_INIT_LOOP, 0.0f,
                                  MOTOR_STOP, true);
    }

    return (motor_friction_l != NULL) && (motor_friction_r != NULL) &&
           (motor_loader != NULL);
}

void Shoot_Update(Input_Data_t *input)
{
    uint32_t now = RmTime_NowMs();
    Shoot_Cmd_t cmd = {0};
    
    if (input == NULL || !input->online || input->emergency_stop) {
        MDBG_SHT("stop by input offline/estop");
        Shoot_Stop();
        return;
    }
    if (!MinimalReferee_AllowShoot()) {
        MDBG_SHT("stop by referee lock");
        Shoot_Stop();
        return;
    }
    
    cmd.friction = input->friction;
    cmd.loader = input->loader;
    cmd.control_mode = CTRL_ENABLE;
    cmd.ref_type = (input->loader == LOADER_CONTINUOUS) ? REF_SPEED : REF_ANGLE;
    g_robot.shoot = cmd;
    shoot_state = input->shoot_state;
    
    // 摩擦轮控制
    if (shoot_state >= SHOOT_FRICTION_ON) {
        float target_speed = FRICTION_TARGET_SPEED * MinimalReferee_FrictionSpeedScale();
        if (motor_friction_l) {
            (void)PublishMotorCommand(motor_friction_l,
                                      FRICTION_RUN_LOOP_ON, target_speed,
                                      MOTOR_ENALBED, true);
        }
        if (motor_friction_r) {
            (void)PublishMotorCommand(motor_friction_r,
                                      FRICTION_RUN_LOOP_ON, target_speed,
                                      MOTOR_ENALBED, true);
        }
    } else {
        if (motor_friction_l) {
            (void)PublishMotorCommand(motor_friction_l,
                                      FRICTION_RUN_LOOP_STOP, 0.0f,
                                      MOTOR_STOP, true);
        }
        if (motor_friction_r) {
            (void)PublishMotorCommand(motor_friction_r,
                                      FRICTION_RUN_LOOP_STOP, 0.0f,
                                      MOTOR_STOP, true);
        }
    }
    
    if (motor_loader == NULL || !MinimalReferee_AllowLoader()) {
        if (motor_loader) {
            (void)PublishMotorCommand(motor_loader, LOADER_RUN_LOOP_STOP,
                                      0.0f, MOTOR_STOP, true);
        }
        single_shot_active = 0;
        pending_shots = 0;
        loader_speed_cmd = 0.0f;
        loader_speed_tick = now;
        loader_ref_last = 0.0f;
        last_continuous_mode = 0U;
        return;
    }
    
    if (shoot_state == SHOOT_CONTINUOUS) {
        float speed_target = LOADER_CONTINUOUS_SPEED;
        float dt_ms;
        float max_delta;

        if (loader_speed_tick == 0U) {
            loader_speed_tick = now;
            loader_speed_cmd = speed_target;
        } else {
            dt_ms = (float)(now - loader_speed_tick);
            loader_speed_tick = now;
            max_delta = LOADER_CONTINUOUS_SLEW_PER_MS * dt_ms;
            if (speed_target > loader_speed_cmd + max_delta) {
                loader_speed_cmd += max_delta;
            } else if (speed_target < loader_speed_cmd - max_delta) {
                loader_speed_cmd -= max_delta;
            } else {
                loader_speed_cmd = speed_target;
            }
        }
        (void)PublishMotorCommand(motor_loader,
                                  LOADER_RUN_LOOP_CONTINUOUS,
                                  loader_speed_cmd, MOTOR_ENALBED, true);
        loader_ref_last = loader_speed_cmd;
        if (!last_continuous_mode) {
            MDBG_SHT("enter continuous");
        }
        last_continuous_mode = 1U;
        single_shot_active = 0;
        pending_shots = 0;
    } else if (shoot_state == SHOOT_SINGLE) {
        if (last_continuous_mode) {
            MDBG_SHT("exit continuous");
        }
        last_continuous_mode = 0U;
        if ((now - last_shot_tick) >= SHOOT_INTERVAL_MS) {
            if (input->loader == LOADER_DOUBLE) {
                pending_shots = 2U;
            } else {
                pending_shots = 1U;
            }
            last_shot_tick = now;
        }

        if (!single_shot_active && pending_shots > 0U) {
            loader_start_angle = LoaderTotalAngle();
            loader_target_angle = loader_start_angle + LOADER_ANGLE_STEP;
            pending_shots--;
            single_shot_active = 1U;
            single_shot_start_ms = now;
            MDBG_SHT("single start target=%.1f pending=%u", (double)loader_target_angle, (unsigned)pending_shots);
        }

        if (single_shot_active) {
            (void)PublishMotorCommand(motor_loader, LOADER_RUN_LOOP_SINGLE,
                                      loader_target_angle, MOTOR_ENALBED, true);
            loader_ref_last = loader_target_angle;
            if (fabsf(LoaderTotalAngle() - loader_target_angle) <= LOADER_SINGLE_SETTLE_EPS ||
                (now - single_shot_start_ms) >= LOADER_SINGLE_TIMEOUT_MS) {
                if ((now - single_shot_start_ms) >= LOADER_SINGLE_TIMEOUT_MS) {
                    MDBG_SHT("single timeout");
                } else {
                    MDBG_SHT("single done");
                }
                single_shot_active = 0U;
            }
        } else {
            (void)PublishMotorCommand(motor_loader, LOADER_RUN_LOOP_STOP,
                                      0.0f, MOTOR_STOP, false);
            loader_ref_last = 0.0f;
        }
        loader_speed_cmd = 0.0f;
        loader_speed_tick = now;
    } else {
        if (last_continuous_mode) {
            MDBG_SHT("exit continuous");
        }
        last_continuous_mode = 0U;
        (void)PublishMotorCommand(motor_loader, LOADER_RUN_LOOP_STOP, 0.0f,
                                  MOTOR_STOP, true);
        single_shot_active = 0;
        pending_shots = 0;
        loader_speed_cmd = 0.0f;
        loader_speed_tick = now;
        loader_ref_last = 0.0f;
    }
}

void Shoot_Stop(void)
{
    shoot_state = SHOOT_OFF;
    single_shot_active = 0;
    pending_shots = 0;
    loader_speed_cmd = 0.0f;
    loader_speed_tick = 0U;
    single_shot_start_ms = 0U;
    loader_ref_last = 0.0f;
    last_continuous_mode = 0U;
    g_robot.shoot.control_mode = CTRL_ZERO_FORCE;
    g_robot.shoot.ref_type = REF_SPEED;
    
    if (motor_friction_l) {
        (void)PublishMotorCommand(motor_friction_l,
                                  FRICTION_RUN_LOOP_STOP, 0.0f,
                                  MOTOR_STOP, true);
    }
    if (motor_friction_r) {
        (void)PublishMotorCommand(motor_friction_r,
                                  FRICTION_RUN_LOOP_STOP, 0.0f,
                                  MOTOR_STOP, true);
    }
    if (motor_loader) {
        (void)PublishMotorCommand(motor_loader, LOADER_RUN_LOOP_STOP, 0.0f,
                                  MOTOR_STOP, true);
    }
}

bool Shoot_IsHealthy(void)
{
    return DJIMotorIsOnline(motor_friction_l) &&
           DJIMotorIsOnline(motor_friction_r) &&
           DJIMotorIsOnline(motor_loader);
}

ShootState_e Shoot_GetState(void)
{
    return shoot_state;
}

float Shoot_GetLoaderRef(void)
{
    return loader_ref_last;
}

float Shoot_GetLoaderFeedback(void)
{
    if (motor_loader == NULL) {
        return 0.0f;
    }
    if (g_robot.shoot.ref_type == REF_ANGLE) {
        return LoaderTotalAngle();
    }
    return LoaderSpeed();
}

uint8_t Shoot_IsSingleActive(void)
{
    return single_shot_active;
}

uint8_t Shoot_GetPendingShots(void)
{
    return pending_shots;
}
