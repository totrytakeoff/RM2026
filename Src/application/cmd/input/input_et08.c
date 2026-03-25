#include "cmd_input.h"

#include "et08_remote.h"
#include "bsp_dwt.h"
#include "usart.h"
#include "stdlib.h"

typedef enum
{
    ET08_POS_UP = 0,
    ET08_POS_MID = 1,
    ET08_POS_DOWN = 2,
    ET08_POS_INVALID = 0xFF
} ET08_SwitchPos_t;

static ET08_Ctrl_t *et08_ctrl;

// 云台设定值在输入后端中保留状态,实现增量控制
static float et08_yaw_ref;
static float et08_pitch_ref;

// 单发触发去抖状态
static uint8_t et08_sb_last_pos = ET08_POS_INVALID;
static uint8_t et08_sb_pos = ET08_POS_MID;
static uint8_t et08_sb_down_armed = 1u;
static uint32_t et08_sb_last_change_ms;
static uint32_t et08_single_shot_until;

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

static uint8_t ET08_GetUpperSwitchPos(uint8_t state)
{
    if (state <= 2u)
        return state;
    return ET08_POS_INVALID;
}

static uint8_t ET08_GetLowerSwitchPos(uint8_t state)
{
    if (state >= 3u && state <= 5u)
        return (uint8_t)(state - 3u);
    return ET08_POS_INVALID;
}

void RobotCMDInputET08Init(void)
{
    et08_ctrl = ET08_Init(&ROBOT_CMD_UART_ET08_HANDLE);
    et08_yaw_ref = 0.0f;
    et08_pitch_ref = 0.0f;
    et08_sb_last_pos = ET08_POS_INVALID;
    et08_sb_pos = ET08_POS_MID;
    et08_sb_down_armed = 1u;
    et08_sb_last_change_ms = 0u;
    et08_single_shot_until = 0u;
}

void RobotCMDInputET08Update(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data)
{
    (void)ctx;

    if (data == NULL)
        return;

    if (et08_ctrl == NULL)
    {
        LOGWARNING("[et08] ctrl NULL");
        data->request_stop = 1u;
        return;
    }

    et08_ctrl = ET08_GetCtrl();
    if (et08_ctrl == NULL || !ET08_IsOnline())
    {
        LOGWARNING("[et08] offline: online=%d, failsafe=%d, frame_lost=%d", 
            ET08_IsOnline(), et08_ctrl->failsafe, et08_ctrl->frame_lost);
        et08_sb_down_armed = 1u;
        et08_sb_last_pos = ET08_POS_INVALID;
        data->request_stop = 1u;
        return;
    }

    // 调试：打印摇杆值和原始通道值
    LOGINFO("[et08] L:(%d,%d) R:(%d,%d) SA_SB:%d SD_SC:%d raw_SA_SB:%d raw_SD_SC:%d", 
        et08_ctrl->left.x, et08_ctrl->left.y,
        et08_ctrl->right.x, et08_ctrl->right.y,
        et08_ctrl->switch_sa_sb_state, et08_ctrl->switch_sd_sc_state,
        et08_ctrl->switch_sa_sb_raw, et08_ctrl->switch_sd_sc_raw);

    data->input_online = 1u;
    data->request_recover = 1u;

    data->chassis_cmd.chassis_mode = CMD_ET08_SD_DEFAULT_CHASSIS_MODE;
    data->gimbal_cmd.gimbal_mode = CMD_ET08_SD_DEFAULT_GIMBAL_MODE;
    data->shoot_cmd.shoot_mode = SHOOT_ON;
    data->shoot_cmd.shoot_rate = CMD_DEFAULT_SHOOT_RATE;
    data->shoot_cmd.bullet_speed = CMD_DEFAULT_BULLET_SPEED;
    data->shoot_cmd.lid_mode = CMD_DEFAULT_LID_MODE;
    data->shoot_cmd.load_mode = LOAD_STOP;
    data->shoot_cmd.friction_mode = FRICTION_OFF;

    uint8_t sd_pos = ET08_GetUpperSwitchPos(et08_ctrl->switch_sd_sc_state);
    if (sd_pos == ET08_POS_UP)
    {
        data->chassis_cmd.chassis_mode = CMD_ET08_SD_UP_CHASSIS_MODE;
        data->gimbal_cmd.gimbal_mode = CMD_ET08_SD_UP_GIMBAL_MODE;
    }

    uint8_t sc_pos = ET08_GetLowerSwitchPos(et08_ctrl->switch_sd_sc_state);
    if (sc_pos == ET08_POS_UP)
        et08_yaw_ref += CMD_ET08_GIMBAL_YAW_STEP;
    else if (sc_pos == ET08_POS_DOWN)
        et08_yaw_ref -= CMD_ET08_GIMBAL_YAW_STEP;

    if (abs(et08_ctrl->right.y) >= CMD_ET08_GIMBAL_RC_DEADZONE)
        et08_pitch_ref += CMD_ET08_GIMBAL_PITCH_SCALE * (float)et08_ctrl->right.y;

#if GIMBAL_PITCH_LIMIT_ENABLE
    et08_pitch_ref = ClampFloat(et08_pitch_ref, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);
#endif
    data->gimbal_cmd.yaw = et08_yaw_ref;
    data->gimbal_cmd.pitch = et08_pitch_ref;

    if (abs(et08_ctrl->left.x) >= CMD_ET08_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.vx = CMD_ET08_CHASSIS_VEL_SCALE * (float)et08_ctrl->left.x / CMD_ET08_STICK_SCALE_DEN;
    if (abs(et08_ctrl->left.y) >= CMD_ET08_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.vy = -CMD_ET08_CHASSIS_VEL_SCALE * (float)et08_ctrl->left.y / CMD_ET08_STICK_SCALE_DEN;
    if (abs(et08_ctrl->right.x) >= CMD_ET08_CHASSIS_RC_DEADZONE)
        data->chassis_cmd.wz = CMD_ET08_CHASSIS_WZ_SCALE * (float)et08_ctrl->right.x / CMD_ET08_STICK_SCALE_DEN;

    uint8_t sa_sb_state = et08_ctrl->switch_sa_sb_state;
    uint8_t raw_state = ET08_MapSwitchState(et08_ctrl->switch_sa_sb_raw);
    if (raw_state != ET08_POS_INVALID)
        sa_sb_state = raw_state;

    uint8_t sa_pos = ET08_GetUpperSwitchPos(sa_sb_state);
    if (sa_pos == ET08_POS_UP)
        data->shoot_cmd.friction_mode = FRICTION_ON;

    et08_sb_pos = ET08_GetLowerSwitchPos(sa_sb_state);
    if (et08_sb_pos == ET08_POS_INVALID)
        et08_sb_pos = ET08_POS_MID;

    uint32_t now = DWT_GetTimeline_ms();
    if (et08_sb_pos != ET08_POS_DOWN)
        et08_sb_down_armed = 1u;

    if (et08_sb_pos != et08_sb_last_pos && (now - et08_sb_last_change_ms) >= CMD_ET08_SWITCH_DEBOUNCE_MS)
    {
        et08_sb_last_change_ms = now;
        if (et08_sb_pos == ET08_POS_DOWN && et08_sb_down_armed)
        {
            et08_single_shot_until = now + CMD_ET08_SINGLE_SHOT_HOLD_MS;
            et08_sb_down_armed = 0u;
        }
        et08_sb_last_pos = et08_sb_pos;
    }

    if (now < et08_single_shot_until)
        data->shoot_cmd.load_mode = LOAD_1_BULLET;
    else if (et08_sb_pos == ET08_POS_UP)
        data->shoot_cmd.load_mode = LOAD_BURSTFIRE;
    else
        data->shoot_cmd.load_mode = LOAD_STOP;
}
