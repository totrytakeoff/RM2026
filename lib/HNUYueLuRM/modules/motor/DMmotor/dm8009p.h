#ifndef DM8009P_H
#define DM8009P_H

#include "bsp_can.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DM8009P_MODE_MIT = 0x000,
    DM8009P_MODE_POSITION = 0x100,
    DM8009P_MODE_SPEED = 0x200,
    DM8009P_MODE_MIXED = 0x300,
} DM8009P_Mode;

typedef struct
{
    CAN_HandleTypeDef *can_handle;
    uint16_t motor_id;   // 拨码设定的CAN ID
    uint16_t master_id;  // 反馈帧ID (Master ID)

    float position_range;
    float velocity_range;
    float torque_range;
    float kp_max;
    float kd_max;

    bool auto_clear_error;
    bool auto_enable_mit;
    bool auto_zero_position;
} DM8009P_InitConfig;

typedef struct
{
    uint8_t motor_id;
    uint8_t error_state;
    float position_rad;
    float velocity_rad_s;
    float torque;
    float mos_temp;
    float rotor_temp;
} DM8009P_Feedback;

typedef struct DM8009P_Handle DM8009P_Handle;

DM8009P_Handle *DM8009P_Init(const DM8009P_InitConfig *config);
void DM8009P_DeInit(DM8009P_Handle *motor);

void DM8009P_ClearError(DM8009P_Handle *motor, DM8009P_Mode mode);
void DM8009P_Enable(DM8009P_Handle *motor, DM8009P_Mode mode);
void DM8009P_Disable(DM8009P_Handle *motor, DM8009P_Mode mode);
void DM8009P_SaveZero(DM8009P_Handle *motor, DM8009P_Mode mode);

void DM8009P_SendMITCommand(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float kp, float kd, float torque);
void DM8009P_SendPositionCommand(DM8009P_Handle *motor, float position_rad, float max_speed_rad_s);
void DM8009P_SendSpeedCommand(DM8009P_Handle *motor, float speed_rad_s);
void DM8009P_SendMixedCommand(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float current);

void DM8009P_RequestRegister(DM8009P_Handle *motor, uint8_t reg);
void DM8009P_WriteRegister(DM8009P_Handle *motor, uint8_t reg, const uint8_t value[4]);
void DM8009P_SaveRegisters(DM8009P_Handle *motor);

const DM8009P_Feedback *DM8009P_GetFeedback(const DM8009P_Handle *motor);

#ifdef __cplusplus
}
#endif

#endif
