/**
 * @file infantry_gimbal.h
 * @brief 云台控制模块 - 支持跟随/分离模式
 */

#ifndef INFANTRY_GIMBAL_H
#define INFANTRY_GIMBAL_H

#include <stdbool.h>

#include "infantry_types.h"

/**
 * @brief 云台初始化
 */
bool Gimbal_Init(void);

/** Refresh high-rate IMU feedback from the motor task before PID execution. */
void Gimbal_MotorStep(void);

/**
 * @brief 云台更新
 * @param input 输入数据
 * @param chassis_wz 底盘旋转角速度(用于跟随模式补偿)
 */
void Gimbal_Update(Input_Data_t *input, float chassis_wz);

/**
 * @brief 云台停止
 */
void Gimbal_Stop(void);
bool Gimbal_IsHealthy(void);

/**
 * @brief 获取当前云台模式
 * @return 云台模式
 */
GimbalMode_e Gimbal_GetMode(void);
float Gimbal_GetYawSpeedRef(void);
float Gimbal_GetYawSpeedFdb(void);
float Gimbal_GetPitchSpeedRef(void);
float Gimbal_GetPitchSpeedFdb(void);
float Gimbal_GetPitchTargetAngle(void);
float Gimbal_GetYawTargetAngle(void);
float Gimbal_GetYawEncoderAngle(void);
float Gimbal_GetPitchEncoderAngle(void);
float Gimbal_GetYawIMUAngle(void);
float Gimbal_GetPitchIMUAngle(void);
float Gimbal_GetYawOffsetRawDeg(void);
float Gimbal_GetYawOffsetLogicDeg(void);
float Gimbal_GetYawRelativeSpeedDeg(void);
float Gimbal_GetYawLogicAngle(void);
AxisCtrlMode_e Gimbal_GetPitchCtrlMode(void);

#endif /* INFANTRY_GIMBAL_H */
