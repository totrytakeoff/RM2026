#include "dm8009p.h"
#include "stdlib.h"
#include "string.h"
#include "general_def.h"
#include "bsp_dwt.h"

#define DM8009P_DEFAULT_P_MAX 12.5f
#define DM8009P_DEFAULT_V_MAX 45.0f
#define DM8009P_DEFAULT_T_MAX 18.0f
#define DM8009P_DEFAULT_KP_MAX 500.0f
#define DM8009P_DEFAULT_KD_MAX 5.0f

struct DM8009P_Handle
{
    CANInstance *command_can;
    DM8009P_InitConfig config;
    DM8009P_Feedback feedback;
};

static float clampf(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    float scaled = (x - offset) * ((float)((1U << bits) - 1U)) / span;
    if (scaled < 0.0f)
        scaled = 0.0f;
    float max_val = (float)((1U << bits) - 1U);
    if (scaled > max_val)
        scaled = max_val;
    return (uint16_t)scaled;
}

static float uint_to_float(int value, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    return ((float)value) * span / ((float)((1U << bits) - 1U)) + x_min;
}

static void DM8009P_SendFrame(DM8009P_Handle *motor, uint32_t id, const uint8_t *payload, uint8_t len)
{
    uint8_t backup_dlc = motor->command_can->txconf.DLC;
    uint32_t backup_id = motor->command_can->txconf.StdId;

    CANSetDLC(motor->command_can, len);
    motor->command_can->txconf.StdId = id;
    memcpy(motor->command_can->tx_buff, payload, len);
    CANTransmit(motor->command_can, 1);

    motor->command_can->txconf.StdId = backup_id;
    CANSetDLC(motor->command_can, backup_dlc);
}

static void DM8009P_SendModeCommand(DM8009P_Handle *motor, DM8009P_Mode mode, uint8_t command_byte)
{
    uint8_t frame[8];
    memset(frame, 0xFF, sizeof(frame));
    frame[7] = command_byte;
    DM8009P_SendFrame(motor, motor->config.motor_id + mode, frame, sizeof(frame));
}

static void DM8009P_Decode(CANInstance *instance)
{
    DM8009P_Handle *motor = (DM8009P_Handle *)instance->id;
    uint8_t *rx = instance->rx_buff;

    motor->feedback.motor_id = rx[0] & 0x0F;
    motor->feedback.error_state = rx[0] >> 4;

    uint16_t pos_raw = ((uint16_t)rx[1] << 8) | rx[2];
    uint16_t vel_raw = ((uint16_t)rx[3] << 4) | (rx[4] >> 4);
    uint16_t tor_raw = ((uint16_t)(rx[4] & 0x0F) << 8) | rx[5];

    motor->feedback.position_rad = uint_to_float(pos_raw, -motor->config.position_range, motor->config.position_range, 16);
    motor->feedback.velocity_rad_s = uint_to_float(vel_raw, -motor->config.velocity_range, motor->config.velocity_range, 12);
    motor->feedback.torque = uint_to_float(tor_raw, -motor->config.torque_range, motor->config.torque_range, 12);
    motor->feedback.mos_temp = (float)rx[6];
    motor->feedback.rotor_temp = (float)rx[7];
}

DM8009P_Handle *DM8009P_Init(const DM8009P_InitConfig *config)
{
    if (config == NULL || config->can_handle == NULL)
        return NULL;

    DM8009P_Handle *motor = (DM8009P_Handle *)malloc(sizeof(DM8009P_Handle));
    memset(motor, 0, sizeof(DM8009P_Handle));
    motor->config = *config;

    if (motor->config.position_range <= 0.0f)
        motor->config.position_range = DM8009P_DEFAULT_P_MAX;
    if (motor->config.velocity_range <= 0.0f)
        motor->config.velocity_range = DM8009P_DEFAULT_V_MAX;
    if (motor->config.torque_range <= 0.0f)
        motor->config.torque_range = DM8009P_DEFAULT_T_MAX;
    if (motor->config.kp_max <= 0.0f)
        motor->config.kp_max = DM8009P_DEFAULT_KP_MAX;
    if (motor->config.kd_max <= 0.0f)
        motor->config.kd_max = DM8009P_DEFAULT_KD_MAX;

    CAN_Init_Config_s can_cfg = {
        .can_handle = config->can_handle,
        .tx_id = config->motor_id,
        .rx_id = config->master_id,
        .can_module_callback = DM8009P_Decode,
        .id = motor,
    };
    motor->command_can = CANRegister(&can_cfg);

    if (config->auto_clear_error)
    {
        DM8009P_ClearError(motor, DM8009P_MODE_MIT);
        DWT_Delay(0.01f);
    }
    if (config->auto_zero_position)
    {
        DM8009P_SaveZero(motor, DM8009P_MODE_MIT);
        DWT_Delay(0.01f);
    }
    if (config->auto_enable_mit)
    {
        DM8009P_Enable(motor, DM8009P_MODE_MIT);
    }

    return motor;
}

void DM8009P_DeInit(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    free(motor);
}

void DM8009P_ClearError(DM8009P_Handle *motor, DM8009P_Mode mode)
{
    if (!motor)
        return;
    DM8009P_SendModeCommand(motor, mode, 0xFB);
}

void DM8009P_Enable(DM8009P_Handle *motor, DM8009P_Mode mode)
{
    if (!motor)
        return;
    DM8009P_SendModeCommand(motor, mode, 0xFC);
}

void DM8009P_Disable(DM8009P_Handle *motor, DM8009P_Mode mode)
{
    if (!motor)
        return;
    DM8009P_SendModeCommand(motor, mode, 0xFD);
}

void DM8009P_SaveZero(DM8009P_Handle *motor, DM8009P_Mode mode)
{
    if (!motor)
        return;
    DM8009P_SendModeCommand(motor, mode, 0xFE);
}

void DM8009P_SendMITCommand(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float kp, float kd, float torque)
{
    if (!motor)
        return;

    position_rad = clampf(position_rad, -motor->config.position_range, motor->config.position_range);
    velocity_rad_s = clampf(velocity_rad_s, -motor->config.velocity_range, motor->config.velocity_range);
    torque = clampf(torque, -motor->config.torque_range, motor->config.torque_range);
    kp = clampf(kp, 0.0f, motor->config.kp_max);
    kd = clampf(kd, 0.0f, motor->config.kd_max);

    uint16_t pos_uint = float_to_uint(position_rad, -motor->config.position_range, motor->config.position_range, 16);
    uint16_t vel_uint = float_to_uint(velocity_rad_s, -motor->config.velocity_range, motor->config.velocity_range, 12);
    uint16_t torque_uint = float_to_uint(torque, -motor->config.torque_range, motor->config.torque_range, 12);
    uint16_t kp_uint = float_to_uint(kp, 0.0f, motor->config.kp_max, 12);
    uint16_t kd_uint = float_to_uint(kd, 0.0f, motor->config.kd_max, 12);

    uint8_t frame[8];
    frame[0] = (uint8_t)(pos_uint >> 8);
    frame[1] = (uint8_t)(pos_uint);
    frame[2] = (uint8_t)(vel_uint >> 4);
    frame[3] = (uint8_t)(((vel_uint & 0x0F) << 4) | (kp_uint >> 8));
    frame[4] = (uint8_t)(kp_uint);
    frame[5] = (uint8_t)(kd_uint >> 4);
    frame[6] = (uint8_t)(((kd_uint & 0x0F) << 4) | (torque_uint >> 8));
    frame[7] = (uint8_t)(torque_uint);

    DM8009P_SendFrame(motor, motor->config.motor_id + DM8009P_MODE_MIT, frame, sizeof(frame));
}

void DM8009P_SendSpeedCommand(DM8009P_Handle *motor, float speed_rad_s)
{
    if (!motor)
        return;

    speed_rad_s = clampf(speed_rad_s, -motor->config.velocity_range, motor->config.velocity_range);
    uint8_t frame[4];
    memcpy(frame, &speed_rad_s, sizeof(float));
    DM8009P_SendFrame(motor, motor->config.motor_id + DM8009P_MODE_SPEED, frame, sizeof(frame));
}

void DM8009P_SendPositionCommand(DM8009P_Handle *motor, float position_rad, float max_speed_rad_s)
{
    if (!motor)
        return;

    uint8_t frame[8];
    memcpy(frame, &position_rad, sizeof(float));
    memcpy(frame + 4, &max_speed_rad_s, sizeof(float));
    DM8009P_SendFrame(motor, motor->config.motor_id + DM8009P_MODE_POSITION, frame, sizeof(frame));
}

void DM8009P_SendMixedCommand(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float current)
{
    if (!motor)
        return;

    uint8_t frame[8];
    memcpy(frame, &position_rad, sizeof(float));
    uint16_t vel_u16 = (uint16_t)(velocity_rad_s * 100.0f);
    uint16_t cur_u16 = (uint16_t)(current * 10000.0f);
    frame[4] = (uint8_t)(vel_u16 & 0xFF);
    frame[5] = (uint8_t)(vel_u16 >> 8);
    frame[6] = (uint8_t)(cur_u16 & 0xFF);
    frame[7] = (uint8_t)(cur_u16 >> 8);

    DM8009P_SendFrame(motor, motor->config.motor_id + DM8009P_MODE_MIXED, frame, sizeof(frame));
}

void DM8009P_RequestRegister(DM8009P_Handle *motor, uint8_t reg)
{
    if (!motor)
        return;
    uint8_t can_id_l = motor->config.motor_id & 0xFF;
    uint8_t can_id_h = (motor->config.motor_id >> 8) & 0x07;
    uint8_t frame[4] = {can_id_l, can_id_h, 0x33, reg};
    DM8009P_SendFrame(motor, 0x7FF, frame, sizeof(frame));
}

void DM8009P_WriteRegister(DM8009P_Handle *motor, uint8_t reg, const uint8_t value[4])
{
    if (!motor || value == NULL)
        return;
    uint8_t can_id_l = motor->config.motor_id & 0x0F;
    uint8_t can_id_h = (motor->config.motor_id >> 4) & 0x0F;
    uint8_t frame[8] = {can_id_l, can_id_h, 0x55, reg, value[0], value[1], value[2], value[3]};
    DM8009P_SendFrame(motor, 0x7FF, frame, sizeof(frame));
}

void DM8009P_SaveRegisters(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    uint8_t can_id_l = motor->config.motor_id & 0xFF;
    uint8_t can_id_h = (motor->config.motor_id >> 8) & 0x07;
    uint8_t frame[4] = {can_id_l, can_id_h, 0xAA, 0x01};
    DM8009P_SendFrame(motor, 0x7FF, frame, sizeof(frame));
}

const DM8009P_Feedback *DM8009P_GetFeedback(const DM8009P_Handle *motor)
{
    if (!motor)
        return NULL;
    return &motor->feedback;
}
