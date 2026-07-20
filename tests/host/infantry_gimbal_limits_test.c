#include <stdbool.h>
#include <math.h>
#include <stdio.h>

#include "infantry_gimbal_limits.h"

static unsigned failures;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,           \
                    #condition);                                               \
            failures++;                                                        \
        }                                                                      \
    } while (false)

static bool Near(float lhs, float rhs)
{
    float error = lhs - rhs;
    if (error < 0.0f) {
        error = -error;
    }
    return error <= 0.0001f;
}

static const GimbalPitchLimitConfig limits = {
    .min_angle_deg = -25.0f,
    .max_angle_deg = 35.0f,
    .soft_margin_deg = 3.0f,
    .command_to_imu_sign = 1.0f,
};

static void TestConfigurationAndInvalidFeedback(void)
{
    GimbalPitchLimitConfig invalid = limits;

    CHECK(GimbalPitchLimit_IsConfigValid(&limits));
    CHECK(!GimbalPitchLimit_IsConfigValid(NULL));
    invalid.command_to_imu_sign = 0.0f;
    CHECK(!GimbalPitchLimit_IsConfigValid(&invalid));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(10.0f, false, 0.0f, &limits),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   20.0f, 10.0f, false, 0.0f, &limits),
               10.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(NAN, true, 0.0f, &limits),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(10.0f, true, NAN, &limits),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   NAN, 10.0f, true, 0.0f, &limits),
               10.0f));
}

static void TestSpeedHardAndSoftLimits(void)
{
    CHECK(Near(GimbalPitchLimit_ClampSpeed(30.0f, true, 0.0f, &limits),
               30.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(30.0f, true, 33.5f, &limits),
               15.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(30.0f, true, 35.0f, &limits),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(-30.0f, true, 35.0f, &limits),
               -30.0f));

    CHECK(Near(GimbalPitchLimit_ClampSpeed(-30.0f, true, -23.5f, &limits),
               -15.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(-30.0f, true, -25.0f, &limits),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(30.0f, true, -25.0f, &limits),
               30.0f));
}

static void TestAngleReferenceUsesSameBoundary(void)
{
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   120.0f, 100.0f, true, 33.5f, &limits),
               101.5f));
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   120.0f, 100.0f, true, 35.0f, &limits),
               100.0f));
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   80.0f, 100.0f, true, 35.0f, &limits),
               80.0f));
    CHECK(Near(GimbalPitchLimit_ClampAngleReference(
                   200.0f, 100.0f, true, 0.0f, &limits),
               135.0f));
}

static void TestDirectionSignCanBeCalibrated(void)
{
    GimbalPitchLimitConfig reversed = limits;
    reversed.command_to_imu_sign = -1.0f;

    CHECK(Near(GimbalPitchLimit_ClampSpeed(-20.0f, true, 35.0f,
                                           &reversed),
               0.0f));
    CHECK(Near(GimbalPitchLimit_ClampSpeed(20.0f, true, 35.0f,
                                           &reversed),
               20.0f));
}

int main(void)
{
    TestConfigurationAndInvalidFeedback();
    TestSpeedHardAndSoftLimits();
    TestAngleReferenceUsesSameBoundary();
    TestDirectionSignCanBeCalibrated();

    if (failures != 0U) {
        fprintf(stderr, "%u gimbal-limit checks failed\n", failures);
        return 1;
    }
    puts("gimbal-limit checks passed");
    return 0;
}
