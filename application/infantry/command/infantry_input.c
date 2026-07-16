/**
 * @file minimal_input.c
 * @brief 双输入统一仲裁(VT主控 + ET08热备)
 */

#include "infantry_input.h"

#include <math.h>
#include <string.h>

#include "infantry_config.h"
#include "infantry_shoot.h"
#include "infantry_debug.h"
#include "rm_time.h"
#include "et08_remote.h"
#include "vt_remote.h"
#include "usart.h"

typedef enum {
    ET08_POS_UP = 0U,
    ET08_POS_MID = 1U,
    ET08_POS_DOWN = 2U,
    ET08_POS_INVALID = 0xFFU
} ET08SwitchPos_e;

static GimbalMode_e vt_gimbal_mode_state = GIMBAL_FOLLOW_CHASSIS;
static FrictionMode_e vt_friction_state = FRICTION_OFF;
static uint8_t vt_f_last = 0U;
static uint8_t vt_r_last = 0U;
static uint32_t last_any_online_tick = 0U;
static uint8_t vt_allowed = 1U;
static InputActive_e last_active_input = INPUT_ACTIVE_NONE;
static uint8_t last_et08_online = 0U;
static uint8_t last_vt_online = 0U;

static uint8_t Input_ShouldDisableVTForDebug(void)
{
#if MINIMAL_DEBUG_ENABLE && MINIMAL_DEBUG_DISABLE_VT_ON_UART_CONFLICT
#if (MINIMAL_DEBUG_UART_PORT == VT_UART_PORT)
    return 1U;
#else
    return 0U;
#endif
#else
    return 0U;
#endif
}

static uint8_t ET08_MapUpperSwitchPos(uint8_t state)
{
    if (state == ET08_POS_INVALID || state > 5U) {
        return ET08_POS_INVALID;
    }
    return (state <= 2U) ? ET08_POS_UP : ET08_POS_DOWN;
}

static uint8_t ET08_MapLowerSwitchPos(uint8_t state)
{
    if (state == ET08_POS_INVALID || state > 5U) {
        return ET08_POS_INVALID;
    }
    return (state <= 2U) ? state : (uint8_t)(state - 3U);
}

static void FillShootState(Input_Data_t *data)
{
    if (data->friction == FRICTION_OFF) {
        data->shoot_state = SHOOT_OFF;
    } else if (data->loader == LOADER_CONTINUOUS) {
        data->shoot_state = SHOOT_CONTINUOUS;
    } else if (data->loader == LOADER_SINGLE || data->loader == LOADER_DOUBLE) {
        data->shoot_state = SHOOT_SINGLE;
    } else {
        data->shoot_state = SHOOT_FRICTION_ON;
    }
}

static uint8_t ET08_RequestTakeover(const Input_Data_t *et08_data)
{
    if (et08_data == NULL || !et08_data->online) {
        return 0U;
    }
#if INPUT_ET08_TAKEOVER_SD_UP
    if (et08_data->rc_raw.sd_pos == ET08_POS_UP) {
        return 1U;
    }
#endif
    if (fabsf(et08_data->vx) > 0.01f || fabsf(et08_data->vy) > 0.01f || fabsf(et08_data->wz) > 0.01f ||
        fabsf(et08_data->yaw_speed) > 0.01f || fabsf(et08_data->pitch_speed) > 0.01f) {
        return 1U;
    }
    if (et08_data->friction == FRICTION_ON || et08_data->loader != LOADER_STOP || et08_data->spin_enable != 0U) {
        return 1U;
    }
    return 0U;
}

bool Input_Init(void)
{
    bool et08_ready =
        ET08_InitWithTimeout(&RC_UART, RC_ONLINE_TIMEOUT_MS) != NULL;
    bool vt_ready = false;

    vt_allowed = (Input_ShouldDisableVTForDebug() == 0U) ? 1U : 0U;
    if (vt_allowed) {
        vt_ready = VT_Init(&VT_UART) != NULL;
    } else {
        MDBG_IN("VT disabled due to uart conflict(debug uart=%u vt uart=%u)", MINIMAL_DEBUG_UART_PORT, VT_UART_PORT);
    }
    last_active_input = INPUT_ACTIVE_NONE;
    last_et08_online = 0U;
    last_vt_online = 0U;
    last_any_online_tick = RmTime_NowMs();
    return et08_ready || vt_ready;
}

void Input_UpdateET08(Input_Data_t *et08_data)
{
    ET08_Ctrl_t et08_snapshot;
    const ET08_Ctrl_t *et08 = &et08_snapshot;
    uint8_t sa_sb_state;
    uint8_t sd_sc_state;
    uint8_t sa_pos;
    uint8_t sb_pos;
    uint8_t sd_pos;
    uint8_t sc_pos;

    if (et08_data == NULL) {
        return;
    }
    memset(et08_data, 0, sizeof(*et08_data));

    if (!ET08_Read(&et08_snapshot) || et08->frame_lost || et08->failsafe) {
        et08_data->online = 0U;
        et08_data->active_input = INPUT_ACTIVE_ET08;
        return;
    }

    et08_data->online = 1U;
    et08_data->active_input = INPUT_ACTIVE_ET08;

#if RC_MAPPING_MODE == 0
    et08_data->vx = -(float)et08->left.x / RC_STICK_SCALE * CHASSIS_MAX_VX;
    et08_data->vy = (float)et08->left.y / RC_STICK_SCALE * CHASSIS_MAX_VY;
#else
    et08_data->vx = -(float)et08->right.x / RC_STICK_SCALE * CHASSIS_MAX_VX;
    et08_data->vy = (float)et08->right.y / RC_STICK_SCALE * CHASSIS_MAX_VY;
#endif
    et08_data->wz = 0.0f;

    sa_sb_state = et08->switch_sa_sb_state;
    sd_sc_state = et08->switch_sd_sc_state;
    {
        uint8_t raw_state = ET08_MapSwitchState(et08->switch_sa_sb_raw);
        if (raw_state != ET08_POS_INVALID) {
            sa_sb_state = raw_state;
        }
    }
    {
        uint8_t raw_state = ET08_MapSwitchState(et08->switch_sd_sc_raw);
        if (raw_state != ET08_POS_INVALID) {
            sd_sc_state = raw_state;
        }
    }

    sa_pos = ET08_MapUpperSwitchPos(sa_sb_state);
    sb_pos = ET08_MapLowerSwitchPos(sa_sb_state);
    sd_pos = ET08_MapUpperSwitchPos(sd_sc_state);
    sc_pos = ET08_MapLowerSwitchPos(sd_sc_state);

    if (sb_pos == ET08_POS_INVALID) {
        sb_pos = ET08_POS_MID;
    }
    if (sd_pos == ET08_POS_INVALID) {
        sd_pos = ET08_POS_DOWN;
    }
    if (sc_pos == ET08_POS_INVALID) {
        sc_pos = ET08_POS_MID;
    }

    et08_data->friction = (sa_pos == ET08_POS_UP) ? FRICTION_ON : FRICTION_OFF;
    if (sb_pos == ET08_POS_UP) {
        et08_data->loader = LOADER_CONTINUOUS;
    } else if (sb_pos == ET08_POS_DOWN) {
        et08_data->loader = LOADER_SINGLE;
    } else {
        et08_data->loader = LOADER_STOP;
    }

    et08_data->gimbal_mode = (sd_pos == ET08_POS_UP) ? GIMBAL_FOLLOW_CHASSIS : GIMBAL_SEPARATE;
    et08_data->spin_enable = (sc_pos == ET08_POS_UP) ? 1U : 0U;
    if (fabsf((float)et08->right.x) >= GIMBAL_RC_DEADZONE) {
        et08_data->yaw_speed =
            (float)et08->right.x / RC_STICK_SCALE * GM6020_SPEED_MAX * ET08_GIMBAL_YAW_SPEED_SCALE;
    } else {
        et08_data->yaw_speed = 0.0f;
    }
    if (fabsf((float)et08->right.y) >= GIMBAL_RC_DEADZONE) {
        et08_data->pitch_speed =
            (float)et08->right.y / RC_STICK_SCALE * GM6020_SPEED_MAX * ET08_PITCH_SPEED_SCALE;
    } else {
        et08_data->pitch_speed = 0.0f;
    }

    et08_data->rc_raw.left_x = et08->left.x;
    et08_data->rc_raw.left_y = et08->left.y;
    et08_data->rc_raw.right_x = et08->right.x;
    et08_data->rc_raw.right_y = et08->right.y;
    et08_data->rc_raw.sa_pos = sa_pos;
    et08_data->rc_raw.sb_pos = sb_pos;
    et08_data->rc_raw.sd_pos = sd_pos;
    et08_data->rc_raw.sc_pos = sc_pos;
    et08_data->rc_raw.online = 1U;
    FillShootState(et08_data);
}

void Input_UpdateVT(Input_Data_t *vt_data)
{
    VT_Ctrl_t vt_snapshot;
    const VT_Ctrl_t *vt = &vt_snapshot;
    uint8_t f_now;
    uint8_t r_now;

    if (vt_data == NULL) {
        return;
    }
    memset(vt_data, 0, sizeof(*vt_data));

    if (!vt_allowed || !VT_Read(&vt_snapshot) || !vt->crc_ok) {
        vt_data->online = 0U;
        vt_data->active_input = INPUT_ACTIVE_VT;
        return;
    }

    vt_data->online = 1U;
    vt_data->active_input = INPUT_ACTIVE_VT;
    vt_data->gear = vt->gear;
    vt_data->emergency_stop = vt->pause_pressed ? 1U : 0U;
    vt_data->vt_raw.ch0_c = vt->ch0_right_x.centered;
    vt_data->vt_raw.ch1_c = vt->ch1_right_y.centered;
    vt_data->vt_raw.ch2_c = vt->ch2_left_y.centered;
    vt_data->vt_raw.ch3_c = vt->ch3_left_x.centered;
    vt_data->vt_raw.dial_c = vt->dial.centered;
    vt_data->vt_raw.gear = vt->gear;
    vt_data->vt_raw.pause = vt->pause_pressed;
    vt_data->vt_raw.custom_l = vt->custom_left_pressed;
    vt_data->vt_raw.custom_r = vt->custom_right_pressed;
    vt_data->vt_raw.trigger = vt->trigger_pressed;
    vt_data->vt_raw.mouse_x = vt->mouse_x;
    vt_data->vt_raw.mouse_y = vt->mouse_y;
    vt_data->vt_raw.mouse_z = vt->mouse_z;
    vt_data->vt_raw.mouse_l = vt->mouse_left_pressed;
    vt_data->vt_raw.mouse_r = vt->mouse_right_pressed;
    vt_data->vt_raw.mouse_m = vt->mouse_middle_pressed;
    vt_data->vt_raw.keyboard = vt->keyboard_value;
    vt_data->vt_raw.online = 1U;

    if (vt_data->emergency_stop) {
        return;
    }

    vt_data->spin_enable = 0U;

    if (vt->gear != VT_GEAR_S) {
        vt_data->friction = FRICTION_OFF;
        vt_data->loader = LOADER_STOP;
        vt_data->gimbal_mode = vt_gimbal_mode_state;
        vt_data->spin_enable = 0U;
        FillShootState(vt_data);
        return;
    }

    {
        float base_speed = VT_CHASSIS_BASE_SPEED;
        if (vt->keyboard_value & VT_KEY_SHIFT) {
            base_speed *= VT_CHASSIS_FAST_MULT;
        } else if (vt->keyboard_value & VT_KEY_CTRL) {
            base_speed *= VT_CHASSIS_SLOW_MULT;
        }

        if (vt->keyboard_value & VT_KEY_W) vt_data->vy += base_speed;
        if (vt->keyboard_value & VT_KEY_S) vt_data->vy -= base_speed;
        if (vt->keyboard_value & VT_KEY_A) vt_data->vx += base_speed;
        if (vt->keyboard_value & VT_KEY_D) vt_data->vx -= base_speed;
        if (vt->keyboard_value & VT_KEY_Q) vt_data->wz -= VT_CHASSIS_ROTATE_SPEED;
        if (vt->keyboard_value & VT_KEY_E) vt_data->wz += VT_CHASSIS_ROTATE_SPEED;

        if (fabsf(vt_data->vx) < 0.1f && fabsf(vt_data->vy) < 0.1f) {
            vt_data->vx = (float)vt->ch0_right_x.centered / 660.0f * CHASSIS_MAX_VX;
            vt_data->vy = (float)vt->ch1_right_y.centered / 660.0f * CHASSIS_MAX_VY;
        }
        if (fabsf(vt_data->wz) < 0.1f) {
            vt_data->wz = (float)vt->ch3_left_x.centered / 660.0f * CHASSIS_MAX_WZ;
        }
    }

    f_now = (vt->keyboard_value & VT_KEY_F) ? 1U : 0U;
    if (f_now && !vt_f_last) {
        vt_gimbal_mode_state =
            (vt_gimbal_mode_state == GIMBAL_FOLLOW_CHASSIS) ? GIMBAL_SEPARATE : GIMBAL_FOLLOW_CHASSIS;
    }
    vt_f_last = f_now;
    vt_data->gimbal_mode = vt_gimbal_mode_state;

    if (vt->mouse_x != 0 || vt->mouse_y != 0) {
        vt_data->yaw_speed = (float)vt->mouse_x * VT_MOUSE_YAW_SENSITIVITY;
        vt_data->pitch_speed = -(float)vt->mouse_y * VT_MOUSE_PITCH_SENSITIVITY;
    } else {
        vt_data->yaw_speed = (float)vt->ch3_left_x.centered / 660.0f * VT_STICK_YAW_SPEED_SCALE;
        vt_data->pitch_speed = (float)vt->ch2_left_y.centered * VT_STICK_PITCH_SPEED_SCALE;
    }

    r_now = (vt->keyboard_value & VT_KEY_R) ? 1U : 0U;
    if (r_now && !vt_r_last) {
        vt_friction_state = (vt_friction_state == FRICTION_OFF) ? FRICTION_ON : FRICTION_OFF;
    }
    vt_r_last = r_now;
    vt_data->friction = vt_friction_state;

    if (vt->mouse_left_pressed) {
        vt_data->loader = LOADER_SINGLE;
    } else if (vt->mouse_middle_pressed) {
        vt_data->loader = LOADER_DOUBLE;
    } else if (vt->mouse_right_pressed) {
        vt_data->loader = LOADER_CONTINUOUS;
    } else {
        vt_data->loader = LOADER_STOP;
    }

    FillShootState(vt_data);
}

void Input_Arbitrate(const Input_Data_t *vt_data, const Input_Data_t *et08_data, Input_Data_t *out)
{
    uint8_t any_online;
    uint8_t et08_takeover;
    uint32_t now;

    if (out == NULL || vt_data == NULL || et08_data == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));

    now = RmTime_NowMs();
    any_online = (vt_data->online || et08_data->online) ? 1U : 0U;
    et08_takeover = ET08_RequestTakeover(et08_data);

    if (et08_data->online != last_et08_online) {
        MDBG_IN("ET08 %s", et08_data->online ? "online" : "offline");
        last_et08_online = et08_data->online;
    }
    if (vt_data->online != last_vt_online) {
        MDBG_IN("VT %s", vt_data->online ? "online" : "offline");
        last_vt_online = vt_data->online;
    }

    if (any_online) {
        last_any_online_tick = now;
    }

    if ((vt_data->online && vt_data->emergency_stop) ||
        (!any_online && (now - last_any_online_tick) >= INPUT_FAILSAFE_HOLD_MS)) {
        out->online = 0U;
        out->emergency_stop = 1U;
        out->active_input = INPUT_ACTIVE_NONE;
        MDBG_IN("failsafe triggered(any_online=%u vt_estop=%u)", (unsigned)any_online, (unsigned)vt_data->emergency_stop);
        return;
    }

    if (et08_takeover) {
        *out = *et08_data;
        out->active_input = INPUT_ACTIVE_ET08;
        if (last_active_input != INPUT_ACTIVE_ET08) {
            MDBG_IN("input switch -> ET08(takeover)");
        }
        last_active_input = INPUT_ACTIVE_ET08;
        return;
    }

    if (vt_data->online) {
        *out = *vt_data;
        out->active_input = INPUT_ACTIVE_VT;
        if (last_active_input != INPUT_ACTIVE_VT) {
            MDBG_IN("input switch -> VT");
        }
        last_active_input = INPUT_ACTIVE_VT;
        return;
    }

    if (et08_data->online) {
        *out = *et08_data;
        out->active_input = INPUT_ACTIVE_ET08;
        if (last_active_input != INPUT_ACTIVE_ET08) {
            MDBG_IN("input switch -> ET08");
        }
        last_active_input = INPUT_ACTIVE_ET08;
        return;
    }

    out->online = 0U;
    out->emergency_stop = 1U;
    out->active_input = INPUT_ACTIVE_NONE;
    if (last_active_input != INPUT_ACTIVE_NONE) {
        MDBG_IN("input switch -> NONE");
    }
    last_active_input = INPUT_ACTIVE_NONE;
}

uint8_t Input_IsOnline(void)
{
    return ((vt_allowed && VT_IsOnline()) || ET08_IsOnline()) ? 1U : 0U;
}

uint8_t Input_IsVTAllowed(void)
{
    return vt_allowed;
}

void Input_GetData(Input_Data_t *data)
{
    Input_Data_t vt_data;
    Input_Data_t et08_data;

    if (data == NULL) {
        return;
    }

    Input_UpdateVT(&vt_data);
    Input_UpdateET08(&et08_data);
    Input_Arbitrate(&vt_data, &et08_data, data);
}
