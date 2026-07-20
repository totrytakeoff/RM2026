#ifndef INFANTRY_GIMBAL_UNITS_H
#define INFANTRY_GIMBAL_UNITS_H

#include <math.h>

/** Convert the INS angular-rate contract (rad/s) to motor-loop deg/s. */
static inline float InfantryGimbal_RadPerSecToDegPerSec(float rate_rad_s)
{
    return rate_rad_s * 57.29577951308232f;
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

#endif /* INFANTRY_GIMBAL_UNITS_H */
