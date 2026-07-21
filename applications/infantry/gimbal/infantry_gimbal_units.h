#ifndef INFANTRY_GIMBAL_UNITS_H
#define INFANTRY_GIMBAL_UNITS_H

#include <math.h>

/** Convert the INS angular-rate contract (rad/s) to motor-loop deg/s. */
static inline float InfantryGimbal_RadPerSecToDegPerSec(float rate_rad_s)
{
    return rate_rad_s * 57.29577951308232f;
}

/** 世界系云台角速度 = 底盘基座角速度 + 云台相对底盘角速度。 */
static inline float InfantryGimbal_EstimateBaseRateRadS(
    float imu_yaw_rate_rad_s,
    float motor_relative_rate_rad_s)
{
    if (!isfinite(imu_yaw_rate_rad_s) ||
        !isfinite(motor_relative_rate_rad_s)) {
        return 0.0f;
    }
    return imu_yaw_rate_rad_s - motor_relative_rate_rad_s;
}

/**
 * Calculate gravity current feedforward from the gravity-referenced IMU pitch.
 * horizontal_deg is the IMU reading when the barrel is mechanically level.
 */
static inline float InfantryGimbal_GravityFeedforward(float pitch_imu_deg,
                                                       float horizontal_deg,
                                                       float gain,
                                                       float max_abs)
{
    float output;

    if (!isfinite(pitch_imu_deg) || !isfinite(horizontal_deg) ||
        !isfinite(gain) || !isfinite(max_abs) || max_abs < 0.0f) {
        return 0.0f;
    }
    output = gain * cosf((pitch_imu_deg - horizontal_deg) *
                         0.01745329251994329577f);
    if (output > max_abs) {
        return max_abs;
    }
    if (output < -max_abs) {
        return -max_abs;
    }
    return output;
}

/**
 * 根据已估算的底盘基座角速度计算扰动补偿电流。
 * 正基座角速度需要负电流前馈抵消。
 */
static inline float InfantryGimbal_BaseRateCurrentFeedforward(
    float base_rate_rad_s,
    float gain,
    float deadband_rad_s,
    float max_abs)
{
    float output;

    if (!isfinite(base_rate_rad_s) ||
        !isfinite(gain) || gain < 0.0f ||
        !isfinite(deadband_rad_s) || deadband_rad_s < 0.0f ||
        !isfinite(max_abs) || max_abs < 0.0f) {
        return 0.0f;
    }
    if (fabsf(base_rate_rad_s) <= deadband_rad_s) {
        return 0.0f;
    }
    output = -gain * base_rate_rad_s;
    if (output > max_abs) {
        return max_abs;
    }
    if (output < -max_abs) {
        return -max_abs;
    }
    return output;
}

#endif /* INFANTRY_GIMBAL_UNITS_H */
