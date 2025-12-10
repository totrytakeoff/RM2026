#include "dm8009p.h"
#include "stdlib.h"
#include "string.h"
#include "general_def.h"
#include "bsp_dwt.h"

#define DM8009P_P_MIN (-12.5f)
#define DM8009P_P_MAX (12.5f)
#define DM8009P_V_MIN (-45.0f)
#define DM8009P_V_MAX (45.0f)
#define DM8009P_T_MIN (-18.0f)
#define DM8009P_T_MAX (18.0f)
#define DM8009P_KP_MIN 0.0f
#define DM8009P_KP_MAX 500.0f
#define DM8009P_KD_MIN 0.0f
#define DM8009P_KD_MAX 5.0f

typedef enum
{
    DM8009P_CMD_MOTOR_MODE = 0xfc,
    DM8009P_CMD_RESET_MODE = 0xfd,
    DM8009P_CMD_ZERO_POSITION = 0xfe,
    DM8009P_CMD_CLEAR_ERROR = 0xfb,
} DM8009P_ModeCommand;

struct DM8009P_Handle
{
    CANInstance *can_instance;
    DM8009P_InitConfig config;
    DM8009P_Feedback feedback;
};

static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    float value = (x - offset) * ((float)((1U << bits) - 1U)) / span;
    if (value < 0.0f)
        value = 0.0f;
    float max_value = (float)((1U << bits) - 1U);
    if (value > max_value)
        value = max_value;
    return (uint16_t)(value);
}

static float uint_to_float(int value, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float converted = ((float)value) * span / ((float)((1U << bits) - 1U)) + x_min;
    return converted;
}

static float clampf(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static void DM8009P_SendMode(DM8009P_Handle *motor, DM8009P_ModeCommand cmd)
{
    uint8_t *tx = motor->can_instance->tx_buff;
    memset(tx, 0xff, 7);
    tx[7] = (uint8_t)cmd;
    uint32_t original_id = motor->can_instance->txconf.StdId;
    motor->can_instance->txconf.StdId = motor->config.command_id;
    CANTransmit(motor->can_instance, 1);
    motor->can_instance->txconf.StdId = original_id;
}

static void DM8009P_Decode(CANInstance *instance)
{
    DM8009P_Handle *motor = (DM8009P_Handle *)instance->id;
    uint8_t *rx = instance->rx_buff;
    uint16_t raw_position = ((uint16_t)rx[1] << 8) | rx[2];
    uint16_t raw_velocity = ((uint16_t)rx[3] << 4) | (rx[4] >> 4);
    uint16_t raw_torque = ((uint16_t)(rx[4] & 0x0F) << 8) | rx[5];

    motor->feedback.motor_id = rx[1] & 0x0F;
    motor->feedback.error = (rx[1] >> 4);
    motor->feedback.position_rad = uint_to_float(raw_position, DM8009P_P_MIN, DM8009P_P_MAX, 16);
    motor->feedback.velocity_rad_s = uint_to_float(raw_velocity, DM8009P_V_MIN, DM8009P_V_MAX, 12);
    motor->feedback.torque = uint_to_float(raw_torque, DM8009P_T_MIN, DM8009P_T_MAX, 12);
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

    CAN_Init_Config_s can_conf = {
        .can_handle = config->can_handle,
        .tx_id = config->command_id,
        .rx_id = config->master_id,
        .can_module_callback = DM8009P_Decode,
        .id = motor,
    };

    motor->can_instance = CANRegister(&can_conf);

    if (config->auto_enable)
    {
        DM8009P_ClearError(motor);
        DWT_Delay(0.02f);
        if (config->auto_zero)
        {
            DM8009P_ZeroPosition(motor);
            DWT_Delay(0.02f);
        }
        DM8009P_Enable(motor);
    }

    return motor;
}

void DM8009P_ClearError(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    DM8009P_SendMode(motor, DM8009P_CMD_CLEAR_ERROR);
}

void DM8009P_Enable(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    DM8009P_SendMode(motor, DM8009P_CMD_MOTOR_MODE);
}

void DM8009P_Disable(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    DM8009P_SendMode(motor, DM8009P_CMD_RESET_MODE);
}

void DM8009P_ZeroPosition(DM8009P_Handle *motor)
{
    if (!motor)
        return;
    DM8009P_SendMode(motor, DM8009P_CMD_ZERO_POSITION);
}

void DM8009P_SendMIT(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float kp, float kd, float torque)
{
    if (!motor)
        return;

    position_rad = clampf(position_rad, DM8009P_P_MIN, DM8009P_P_MAX);
    velocity_rad_s = clampf(velocity_rad_s, DM8009P_V_MIN, DM8009P_V_MAX);
    kp = clampf(kp, DM8009P_KP_MIN, DM8009P_KP_MAX);
    kd = clampf(kd, DM8009P_KD_MIN, DM8009P_KD_MAX);
    torque = clampf(torque, DM8009P_T_MIN, DM8009P_T_MAX);

    uint16_t p_uint = float_to_uint(position_rad, DM8009P_P_MIN, DM8009P_P_MAX, 16);
    uint16_t v_uint = float_to_uint(velocity_rad_s, DM8009P_V_MIN, DM8009P_V_MAX, 12);
    uint16_t kp_uint = float_to_uint(kp, DM8009P_KP_MIN, DM8009P_KP_MAX, 12);
    uint16_t kd_uint = float_to_uint(kd, DM8009P_KD_MIN, DM8009P_KD_MAX, 12);
    uint16_t torque_uint = float_to_uint(torque, DM8009P_T_MIN, DM8009P_T_MAX, 12);

    uint8_t *tx = motor->can_instance->tx_buff;
    tx[0] = (uint8_t)(p_uint >> 8);
    tx[1] = (uint8_t)(p_uint);
    tx[2] = (uint8_t)(v_uint >> 4);
    tx[3] = (uint8_t)(((v_uint & 0x0F) << 4) | (kp_uint >> 8));
    tx[4] = (uint8_t)(kp_uint);
    tx[5] = (uint8_t)(kd_uint >> 4);
    tx[6] = (uint8_t)(((kd_uint & 0x0F) << 4) | (torque_uint >> 8));
    tx[7] = (uint8_t)(torque_uint);

    uint32_t original_id = motor->can_instance->txconf.StdId;
    motor->can_instance->txconf.StdId = motor->config.command_id;
    CANTransmit(motor->can_instance, 1);
    motor->can_instance->txconf.StdId = original_id;
}

void DM8009P_SetSpeed(DM8009P_Handle *motor, float speed_rad_s, float damping)
{
    if (!motor)
        return;
    speed_rad_s = clampf(speed_rad_s, DM8009P_V_MIN, DM8009P_V_MAX);
    damping = clampf(damping, DM8009P_KD_MIN + 0.01f, DM8009P_KD_MAX);
    DM8009P_SendMIT(motor, 0.0f, speed_rad_s, 0.0f, damping, 0.0f);
}

void DM8009P_SetSpeedMode(DM8009P_Handle *motor, float speed_rad_s)
{
    if (!motor)
        return;

    speed_rad_s = clampf(speed_rad_s, DM8009P_V_MIN, DM8009P_V_MAX);
    uint8_t frame[8] = {0};
    memcpy(frame, &speed_rad_s, sizeof(float));

    uint8_t *tx = motor->can_instance->tx_buff;
    memcpy(tx, frame, 4);
    memset(tx + 4, 0, 4);

    uint32_t original_id = motor->can_instance->txconf.StdId;
    motor->can_instance->txconf.StdId = 0x200u + motor->config.command_id;
    CANTransmit(motor->can_instance, 1);
    motor->can_instance->txconf.StdId = original_id;
}

void DM8009P_SetPositionMode(DM8009P_Handle *motor, float position_rad, float max_speed_rad_s)
{
    if (!motor)
        return;

    uint8_t frame[8] = {0};
    memcpy(frame, &position_rad, sizeof(float));
    memcpy(frame + 4, &max_speed_rad_s, sizeof(float));

    uint8_t *tx = motor->can_instance->tx_buff;
    memcpy(tx, frame, 8);

    uint32_t original_id = motor->can_instance->txconf.StdId;
    motor->can_instance->txconf.StdId = 0x100u + motor->config.command_id;
    CANTransmit(motor->can_instance, 1);
    motor->can_instance->txconf.StdId = original_id;
}

const DM8009P_Feedback *DM8009P_GetFeedback(const DM8009P_Handle *motor)
{
    if (!motor)
        return NULL;
    return &motor->feedback;
}
