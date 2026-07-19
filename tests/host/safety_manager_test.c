#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "safety_manager.h"

static unsigned failures;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,           \
                    #condition);                                               \
            failures++;                                                        \
        }                                                                      \
    } while (false)

static RmSafetyInputs HealthyInputs(void)
{
    const RmSafetyInputs inputs = {
        .initialization_complete = true,
        .input_online = true,
        .emergency_stop = false,
        .task_health_ok = true,
        .device_health_ok = true,
    };
    return inputs;
}

static void TestBootAndActivation(void)
{
    RmSafetyManager manager;
    RmSafetyInputs inputs = HealthyInputs();

    RmSafety_Init(&manager);
    CHECK(manager.state == RM_SAFETY_STATE_BOOT);
    CHECK(manager.reasons == RM_SAFETY_REASON_NOT_READY);
    CHECK(manager.transition_count == 0U);
    CHECK(!RmSafety_OutputPermitted(&manager));

    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_ACTIVE);
    CHECK(manager.reasons == RM_SAFETY_REASON_NONE);
    CHECK(manager.transition_count == 1U);
    CHECK(RmSafety_OutputPermitted(&manager));

    CHECK(!RmSafety_Update(&manager, &inputs));
    CHECK(manager.transition_count == 1U);
}

static void TestFaultReasonsAndRecovery(void)
{
    RmSafetyManager manager;
    RmSafetyInputs inputs = HealthyInputs();

    RmSafety_Init(&manager);
    CHECK(RmSafety_Update(&manager, &inputs));

    inputs.input_online = false;
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_STOPPED);
    CHECK(manager.reasons == RM_SAFETY_REASON_INPUT_OFFLINE);
    CHECK(!RmSafety_OutputPermitted(&manager));

    inputs.emergency_stop = true;
    inputs.task_health_ok = false;
    inputs.device_health_ok = false;
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.reasons == (RM_SAFETY_REASON_INPUT_OFFLINE |
                              RM_SAFETY_REASON_EMERGENCY_STOP |
                              RM_SAFETY_REASON_TASK_UNHEALTHY |
                              RM_SAFETY_REASON_DEVICE_UNHEALTHY));

    inputs = HealthyInputs();
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_ACTIVE);
    CHECK(manager.reasons == RM_SAFETY_REASON_NONE);
    CHECK(RmSafety_OutputPermitted(&manager));
}

static void TestNotReadyIsDisarmed(void)
{
    RmSafetyManager manager;
    RmSafetyInputs inputs = HealthyInputs();

    RmSafety_Init(&manager);
    inputs.initialization_complete = false;
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_DISARMED);
    CHECK(manager.reasons == RM_SAFETY_REASON_NOT_READY);
    CHECK(!RmSafety_OutputPermitted(&manager));
}

static void TestDeviceFaultStopsAndRecovers(void)
{
    RmSafetyManager manager;
    RmSafetyInputs inputs = HealthyInputs();

    RmSafety_Init(&manager);
    CHECK(RmSafety_Update(&manager, &inputs));

    inputs.device_health_ok = false;
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_STOPPED);
    CHECK(manager.reasons == RM_SAFETY_REASON_DEVICE_UNHEALTHY);
    CHECK(!RmSafety_OutputPermitted(&manager));

    inputs.device_health_ok = true;
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(manager.state == RM_SAFETY_STATE_ACTIVE);
    CHECK(manager.reasons == RM_SAFETY_REASON_NONE);
    CHECK(RmSafety_OutputPermitted(&manager));
}

static void TestNullArgumentsRemainSafe(void)
{
    RmSafetyManager manager;
    RmSafetyInputs inputs = HealthyInputs();

    RmSafety_Init(NULL);
    RmSafety_Init(&manager);
    CHECK(!RmSafety_Update(NULL, &inputs));
    CHECK(RmSafety_Update(&manager, &inputs));
    CHECK(RmSafety_OutputPermitted(&manager));
    CHECK(RmSafety_Update(&manager, NULL));
    CHECK(manager.state == RM_SAFETY_STATE_STOPPED);
    CHECK(manager.reasons == RM_SAFETY_REASON_INVALID_INPUT);
    CHECK(!RmSafety_OutputPermitted(&manager));
    CHECK(!RmSafety_Update(&manager, NULL));
    CHECK(!RmSafety_OutputPermitted(NULL));
}

int main(void)
{
    TestBootAndActivation();
    TestFaultReasonsAndRecovery();
    TestNotReadyIsDisarmed();
    TestDeviceFaultStopsAndRecovers();
    TestNullArgumentsRemainSafe();

    if (failures != 0U) {
        fprintf(stderr, "%u safety-manager checks failed\n", failures);
        return 1;
    }

    puts("safety-manager checks passed");
    return 0;
}
