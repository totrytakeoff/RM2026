#include "cmd_input.h"

#include "vt_remote.h"
#include "stdlib.h"
#include "usart.h"

static VT_Ctrl_t *vt_ctrl;
static float vt_yaw_ref;
static float vt_pitch_ref;

#if GIMBAL_PITCH_LIMIT_ENABLE
static float ClampFloat(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}
#endif

void RobotCMDInputDatalinkInit(void)
{
    vt_ctrl = VT_Init(&ROBOT_CMD_UART_DATALINK_HANDLE);
    vt_yaw_ref = 0.0f;
    vt_pitch_ref = 0.0f;
}

void RobotCMDInputDatalinkUpdate(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data)
{
    (void)ctx;

    if (data == NULL)
        return;

    if (vt_ctrl == NULL)
    {
        data->request_stop = 1u;
        return;
    }

    vt_ctrl = VT_GetCtrl();
    if (vt_ctrl == NULL || !VT_IsOnline() || !vt_ctrl->crc_ok)
    {
        data->request_stop = 1u;
        return;
    }

    data->input_online = 1u;
    data->request_stop = vt_ctrl->pause_pressed ? 1u : 0u;
    data->request_recover = (vt_ctrl->gear == VT_GEAR_C) ? 1u : 0u;

    data->chassis_cmd.chassis_mode = CMD_VT_GEAR_DEFAULT_CHASSIS_MODE;
    data->gimbal_cmd.gimbal_mode = CMD_VT_GEAR_DEFAULT_GIMBAL_MODE;
    data->shoot_cmd.shoot_mode = SHOOT_ON;
    data->shoot_cmd.shoot_rate = CMD_DEFAULT_SHOOT_RATE;
    data->shoot_cmd.bullet_speed = CMD_DEFAULT_BULLET_SPEED;
    data->shoot_cmd.lid_mode = CMD_DEFAULT_LID_MODE;
    data->shoot_cmd.load_mode = LOAD_STOP;
    data->shoot_cmd.friction_mode = FRICTION_OFF;

    if (vt_ctrl->gear == VT_GEAR_S)
    {
        data->chassis_cmd.chassis_mode = CMD_VT_GEAR_S_CHASSIS_MODE;
        data->gimbal_cmd.gimbal_mode = CMD_VT_GEAR_S_GIMBAL_MODE;
    }
    else if (vt_ctrl->gear == VT_GEAR_N)
    {
        data->chassis_cmd.chassis_mode = CMD_VT_GEAR_N_CHASSIS_MODE;
        data->gimbal_cmd.gimbal_mode = CMD_VT_GEAR_N_GIMBAL_MODE;
    }

    if (abs(vt_ctrl->ch3_left_x.centered) >= CMD_VT_GIMBAL_RC_DEADZONE)
        vt_yaw_ref += CMD_VT_GIMBAL_YAW_STEP_SCALE * (float)vt_ctrl->ch3_left_x.centered;
    if (abs(vt_ctrl->ch2_left_y.centered) >= CMD_VT_GIMBAL_RC_DEADZONE)
        vt_pitch_ref += CMD_VT_GIMBAL_PITCH_STEP_SCALE * (float)vt_ctrl->ch2_left_y.centered;

#if GIMBAL_PITCH_LIMIT_ENABLE
    vt_pitch_ref = ClampFloat(vt_pitch_ref, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);
#endif
    data->gimbal_cmd.yaw = vt_yaw_ref;
    data->gimbal_cmd.pitch = vt_pitch_ref;

    if (abs(vt_ctrl->ch0_right_x.centered) >= CMD_VT_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.vx = CMD_VT_CHASSIS_VEL_SCALE * (float)vt_ctrl->ch0_right_x.centered / CMD_VT_STICK_SCALE_DEN;
    if (abs(vt_ctrl->ch1_right_y.centered) >= CMD_VT_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.vy = CMD_VT_CHASSIS_VEL_SCALE * (float)vt_ctrl->ch1_right_y.centered / CMD_VT_STICK_SCALE_DEN;

    if (abs(vt_ctrl->mouse_x) >= CMD_VT_MOUSE_WZ_DEADZONE)
        data->chassis_cmd.wz = CMD_VT_CHASSIS_WZ_MOUSE_SCALE * (float)vt_ctrl->mouse_x;
    else if (abs(vt_ctrl->ch3_left_x.centered) >= CMD_VT_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.wz = CMD_VT_CHASSIS_WZ_SCALE * (float)vt_ctrl->ch3_left_x.centered / CMD_VT_STICK_SCALE_DEN;

    if (vt_ctrl->dial.centered < CMD_VT_DIAL_FRICTION_ON_THRESHOLD || vt_ctrl->custom_left_pressed)
        data->shoot_cmd.friction_mode = FRICTION_ON;

    if (vt_ctrl->dial.centered < CMD_VT_DIAL_BURST_THRESHOLD || vt_ctrl->trigger_pressed || vt_ctrl->mouse_left_pressed)
        data->shoot_cmd.load_mode = LOAD_BURSTFIRE;
}
