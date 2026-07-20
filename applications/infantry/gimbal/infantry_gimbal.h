/**
 * @file infantry_gimbal.h
 * @brief 云台执行控制模块
 */

#ifndef INFANTRY_GIMBAL_H
#define INFANTRY_GIMBAL_H

#include <stdbool.h>

#include "infantry_types.h"

typedef struct {
    uint16_t encoder_ecd;
    float encoder_single_deg;
    float target_total_deg;
    float imu_total_deg;
    float imu_single_deg;
    float imu_gyro_z_rad_s;
    float offset_raw_deg;
    float offset_logic_deg;
    float relative_speed_rad_s;
} GimbalYawTuningSnapshot;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 云台初始化
 */
bool Gimbal_Init(void);

/** Refresh high-rate IMU feedback from the motor task before PID execution. */
void Gimbal_MotorStep(void);

/**
 * @brief 云台更新
 * @param input 输入数据
 */
void Gimbal_Update(Input_Data_t *input);

/**
 * @brief 云台停止
 */
void Gimbal_Stop(void);
/** True only when both yaw and pitch motor feedback links are online. */
bool Gimbal_AreMotorsHealthy(void);
bool Gimbal_IsHealthy(void);

/**
 * @brief 获取当前云台模式
 * @return 云台模式
 */
InfantryControlMode_e Gimbal_GetMode(void);
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
float Gimbal_GetPitchGravityFeedforward(void);
float Gimbal_GetYawOffsetRawDeg(void);
float Gimbal_GetYawOffsetLogicDeg(void);
float Gimbal_GetYawOffsetLogicRad(void);
float Gimbal_GetYawRelativeSpeedRadS(void);
float Gimbal_GetYawLogicAngle(void);
AxisCtrlMode_e Gimbal_GetPitchCtrlMode(void);

/** Copy the Yaw calibration and follow feedback needed by tuning telemetry. */
bool Gimbal_GetYawTuningSnapshot(GimbalYawTuningSnapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_GIMBAL_H */
