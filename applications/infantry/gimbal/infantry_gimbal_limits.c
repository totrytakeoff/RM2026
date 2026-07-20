#include "infantry_gimbal_limits.h"

#include <math.h>
#include <stddef.h>

static float Clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

bool GimbalPitchLimit_IsConfigValid(const GimbalPitchLimitConfig *config)
{
    if (config == NULL || !isfinite(config->min_angle_deg) ||
        !isfinite(config->max_angle_deg) ||
        !isfinite(config->soft_margin_deg) ||
        !isfinite(config->command_to_imu_sign) ||
        config->max_angle_deg <= config->min_angle_deg ||
        config->soft_margin_deg <= 0.0f ||
        config->soft_margin_deg >
            (config->max_angle_deg - config->min_angle_deg) * 0.5f) {
        return false;
    }
    return config->command_to_imu_sign == 1.0f ||
           config->command_to_imu_sign == -1.0f;
}

static float MotionScale(float command_direction,
                         float imu_pitch_deg,
                         const GimbalPitchLimitConfig *config)
{
    const float imu_direction =
        command_direction * config->command_to_imu_sign;

    if ((imu_direction > 0.0f &&
         imu_pitch_deg >= config->max_angle_deg) ||
        (imu_direction < 0.0f &&
         imu_pitch_deg <= config->min_angle_deg)) {
        return 0.0f;
    }
    if (imu_direction > 0.0f &&
        imu_pitch_deg >
            config->max_angle_deg - config->soft_margin_deg) {
        return Clamp01((config->max_angle_deg - imu_pitch_deg) /
                       config->soft_margin_deg);
    }
    if (imu_direction < 0.0f &&
        imu_pitch_deg <
            config->min_angle_deg + config->soft_margin_deg) {
        return Clamp01((imu_pitch_deg - config->min_angle_deg) /
                       config->soft_margin_deg);
    }
    return 1.0f;
}

float GimbalPitchLimit_ClampSpeed(
    float requested_speed,
    bool imu_angle_valid,
    float imu_pitch_deg,
    const GimbalPitchLimitConfig *config)
{
    if (!isfinite(requested_speed) || !imu_angle_valid ||
        !isfinite(imu_pitch_deg) ||
        !GimbalPitchLimit_IsConfigValid(config)) {
        return 0.0f;
    }
    return requested_speed *
           MotionScale(requested_speed, imu_pitch_deg, config);
}

float GimbalPitchLimit_ClampAngleReference(
    float requested_motor_angle_deg,
    float current_motor_angle_deg,
    bool imu_angle_valid,
    float imu_pitch_deg,
    const GimbalPitchLimitConfig *config)
{
    float delta;
    float scaled_delta;
    float target_imu_angle;

    if (!isfinite(current_motor_angle_deg)) {
        return 0.0f;
    }
    if (!isfinite(requested_motor_angle_deg) || !imu_angle_valid ||
        !isfinite(imu_pitch_deg) ||
        !GimbalPitchLimit_IsConfigValid(config)) {
        return current_motor_angle_deg;
    }

    delta = requested_motor_angle_deg - current_motor_angle_deg;
    if (!isfinite(delta)) {
        return current_motor_angle_deg;
    }

    scaled_delta = delta * MotionScale(delta, imu_pitch_deg, config);
    target_imu_angle =
        imu_pitch_deg + scaled_delta * config->command_to_imu_sign;
    if (target_imu_angle > config->max_angle_deg) {
        target_imu_angle = config->max_angle_deg;
    } else if (target_imu_angle < config->min_angle_deg) {
        target_imu_angle = config->min_angle_deg;
    }

    /* 将硬限位后的 IMU 角增量换回电机角参考增量。 */
    return current_motor_angle_deg +
           (target_imu_angle - imu_pitch_deg) /
               config->command_to_imu_sign;
}
