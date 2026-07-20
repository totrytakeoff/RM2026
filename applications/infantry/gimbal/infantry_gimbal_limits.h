#ifndef INFANTRY_GIMBAL_LIMITS_H
#define INFANTRY_GIMBAL_LIMITS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float min_angle_deg;
    float max_angle_deg;
    float soft_margin_deg;
    /** 执行指令正方向到 IMU Pitch 正方向的符号，只允许 +1 或 -1。 */
    float command_to_imu_sign;
} GimbalPitchLimitConfig;

bool GimbalPitchLimit_IsConfigValid(const GimbalPitchLimitConfig *config);

/** 对所有来源的 Pitch 速度请求实施机械软限位。 */
float GimbalPitchLimit_ClampSpeed(
    float requested_speed,
    bool imu_angle_valid,
    float imu_pitch_deg,
    const GimbalPitchLimitConfig *config);

/** 对角度保持/自动控制目标实施同一套机械软限位。 */
float GimbalPitchLimit_ClampAngleReference(
    float requested_motor_angle_deg,
    float current_motor_angle_deg,
    bool imu_angle_valid,
    float imu_pitch_deg,
    const GimbalPitchLimitConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_GIMBAL_LIMITS_H */
