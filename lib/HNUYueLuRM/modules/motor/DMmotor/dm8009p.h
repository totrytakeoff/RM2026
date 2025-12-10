#ifndef DM8009P_H
#define DM8009P_H

#include "bsp_can.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    CAN_HandleTypeDef *can_handle;
    uint16_t command_id;  // 电机拨码设定的ID
    uint16_t master_id;   // 反馈帧ID(即Master ID)
    uint8_t auto_enable;  // 初始化后自动Clear Error并进入Motor Mode
    uint8_t auto_zero;    // 初始化后执行零位校准
} DM8009P_InitConfig;

typedef struct
{
    float position_rad;
    float velocity_rad_s;
    float torque;
    uint8_t error;
    uint8_t motor_id;
    float mos_temp;
    float rotor_temp;
} DM8009P_Feedback;

typedef struct DM8009P_Handle DM8009P_Handle;

DM8009P_Handle *DM8009P_Init(const DM8009P_InitConfig *config);
void DM8009P_ClearError(DM8009P_Handle *motor);
void DM8009P_Enable(DM8009P_Handle *motor);
void DM8009P_Disable(DM8009P_Handle *motor);
void DM8009P_ZeroPosition(DM8009P_Handle *motor);
void DM8009P_SendMIT(DM8009P_Handle *motor, float position_rad, float velocity_rad_s, float kp, float kd, float torque);
void DM8009P_SetSpeed(DM8009P_Handle *motor, float speed_rad_s, float damping);
void DM8009P_SetSpeedMode(DM8009P_Handle *motor, float speed_rad_s);
void DM8009P_SetPositionMode(DM8009P_Handle *motor, float position_rad, float max_speed_rad_s);
const DM8009P_Feedback *DM8009P_GetFeedback(const DM8009P_Handle *motor);

#ifdef __cplusplus
}
#endif

#endif
