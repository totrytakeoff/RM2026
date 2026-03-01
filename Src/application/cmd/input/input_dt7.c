#include "cmd_input.h"

#include "remote_control.h"
#include "usart.h"

static RC_ctrl_t *rc_data;
static float dt7_yaw_ref;
static float dt7_pitch_ref;

void RobotCMDInputDT7Init(void)
{
    rc_data = RemoteControlInit(&ROBOT_CMD_UART_DT7_HANDLE);
    dt7_yaw_ref = 0.0f;
    dt7_pitch_ref = 0.0f;
}

void RobotCMDInputDT7Update(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data)
{
    (void)ctx;

    if (data == NULL || rc_data == NULL || !RemoteControlIsOnline())
    {
        if (data != NULL)
            data->request_stop = 1u;
        return;
    }

    data->input_online = 1u;
    data->request_recover = switch_is_up(rc_data[TEMP].rc.switch_right);
    data->shoot_cmd.shoot_mode = SHOOT_ON;
    data->shoot_cmd.shoot_rate = CMD_DT7_SHOOT_RATE;
    data->shoot_cmd.bullet_speed = CMD_DEFAULT_BULLET_SPEED;
    data->shoot_cmd.lid_mode = CMD_DEFAULT_LID_MODE;

    // 控制底盘和云台运行模式
    if (switch_is_down(rc_data[TEMP].rc.switch_right))
    {
        data->chassis_cmd.chassis_mode = CHASSIS_ROTATE;
        data->gimbal_cmd.gimbal_mode = GIMBAL_GYRO_MODE;
    }
    else if (switch_is_mid(rc_data[TEMP].rc.switch_right))
    {
        data->chassis_cmd.chassis_mode = CHASSIS_NO_FOLLOW;
        data->gimbal_cmd.gimbal_mode = GIMBAL_FREE_MODE;
    }
    else
    {
        data->chassis_cmd.chassis_mode = CHASSIS_NO_FOLLOW;
        data->gimbal_cmd.gimbal_mode = GIMBAL_GYRO_MODE;
    }

    dt7_yaw_ref += CMD_DT7_GIMBAL_YAW_STEP_SCALE * (float)rc_data[TEMP].rc.rocker_l_;
    dt7_pitch_ref += CMD_DT7_GIMBAL_PITCH_STEP_SCALE * (float)rc_data[TEMP].rc.rocker_l1;
    data->gimbal_cmd.yaw = dt7_yaw_ref;
    data->gimbal_cmd.pitch = dt7_pitch_ref;

    data->chassis_cmd.vx = CMD_DT7_CHASSIS_VEL_SCALE * (float)rc_data[TEMP].rc.rocker_r_;
    data->chassis_cmd.vy = CMD_DT7_CHASSIS_VEL_SCALE * (float)rc_data[TEMP].rc.rocker_r1;

    if (rc_data[TEMP].rc.dial < CMD_DT7_DIAL_FRICTION_ON_THRESHOLD)
        data->shoot_cmd.friction_mode = FRICTION_ON;
    else
        data->shoot_cmd.friction_mode = FRICTION_OFF;

    if (rc_data[TEMP].rc.dial < CMD_DT7_DIAL_BURST_THRESHOLD)
        data->shoot_cmd.load_mode = LOAD_BURSTFIRE;
    else
        data->shoot_cmd.load_mode = LOAD_STOP;
}
