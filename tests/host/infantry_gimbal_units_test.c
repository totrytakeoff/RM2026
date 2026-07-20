#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "infantry_gimbal_units.h"

static unsigned failures;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,          \
                    #condition);                                               \
            failures++;                                                        \
        }                                                                      \
    } while (false)

static bool Near(float lhs, float rhs)
{
    return fabsf(lhs - rhs) <= 0.001f;
}

int main(void)
{
    CHECK(Near(InfantryGimbal_RadPerSecToDegPerSec(0.0f), 0.0f));
    CHECK(Near(InfantryGimbal_RadPerSecToDegPerSec(1.0f), 57.2957795f));
    CHECK(Near(InfantryGimbal_RadPerSecToDegPerSec(-3.14159265358979323846f),
               -180.0f));
    CHECK(Near(InfantryGimbal_GravityFeedforward(0.0f, 0.0f,
                                                 2500.0f, 4000.0f),
               2500.0f));
    CHECK(Near(InfantryGimbal_GravityFeedforward(90.0f, 0.0f,
                                                 2500.0f, 4000.0f),
               0.0f));
    CHECK(Near(InfantryGimbal_GravityFeedforward(0.0f, 0.0f,
                                                 5000.0f, 4000.0f),
               4000.0f));
    CHECK(Near(InfantryGimbal_GravityFeedforward(NAN, 0.0f,
                                                 2500.0f, 4000.0f),
               0.0f));

    if (failures != 0U) {
        fprintf(stderr, "%u gimbal-unit checks failed\n", failures);
        return 1;
    }

    puts("gimbal-unit checks passed");
    return 0;
}
