#include "infantry_app.h"

#include <stdatomic.h>
#include <string.h>

#include "infantry_chassis.h"
#include "infantry_config.h"
#include "infantry_debug.h"
#include "infantry_gimbal.h"
#include "infantry_input.h"
#include "infantry_referee.h"
#include "infantry_shoot.h"
#include "main.h"

Robot_Context_t g_robot;

static RmSafetyManager safety_manager;
static atomic_bool task_health_ok;

void InfantryApp_ForceSafeStop(void)
{
    Chassis_Stop();
    Gimbal_Stop();
    Shoot_Stop();
}

void InfantryApp_Init(void)
{
    memset(&g_robot, 0, sizeof(g_robot));
    RmSafety_Init(&safety_manager);
    atomic_init(&task_health_ok, true);

    MinimalDebug_Init();

    /* Preserve the proven bare-metal startup delay during migration. */
    HAL_Delay(MOTOR_STABILIZE_TIME_MS);

    Input_Init();
    MinimalReferee_Init();
    Chassis_Init();
    Gimbal_Init();
    Shoot_Init();

    g_robot.initialized = 1U;
    MDBG_SYS("infantry application initialized");
}

void InfantryApp_ControlStep(uint32_t now_ms)
{
    Input_Data_t input;
    RmSafetyInputs safety_inputs;

    (void)now_ms;
    Input_GetData(&input);
    MinimalReferee_Update();
    g_robot.referee = *MinimalReferee_GetData();

    safety_inputs.initialization_complete = (g_robot.initialized != 0U);
    safety_inputs.input_online = (input.online != 0U);
    safety_inputs.emergency_stop = (input.emergency_stop != 0U);
    safety_inputs.task_health_ok =
        atomic_load_explicit(&task_health_ok, memory_order_relaxed);

    if (RmSafety_Update(&safety_manager, &safety_inputs)) {
        MDBG_SYS("safety state=%u reasons=0x%08lx",
                 (unsigned)safety_manager.state,
                 (unsigned long)safety_manager.reasons);
    }

    g_robot.input = input;
    g_robot.emergency_stop = RmSafety_OutputPermitted(&safety_manager) ? 0U : 1U;
    if (!RmSafety_OutputPermitted(&safety_manager)) {
        InfantryApp_ForceSafeStop();
        return;
    }

    /* Keep the minimal baseline's execution order unchanged. */
    Gimbal_Update(&input, Chassis_GetWz());
    Chassis_Update(&input);
    Shoot_Update(&input);
}

void InfantryApp_DiagnosticsStep(uint32_t now_ms)
{
    MinimalDebug_UpdatePeriodic(now_ms);
}

void InfantryApp_SetTaskHealth(bool healthy)
{
    atomic_store_explicit(&task_health_ok, healthy, memory_order_relaxed);
}

RmSafetyState InfantryApp_GetSafetyState(void)
{
    return safety_manager.state;
}

uint32_t InfantryApp_GetSafetyReasons(void)
{
    return safety_manager.reasons;
}

const Robot_Context_t *InfantryApp_GetContext(void)
{
    return &g_robot;
}
