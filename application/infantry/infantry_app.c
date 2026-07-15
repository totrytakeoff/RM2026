#include "infantry_app.h"

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

static uint8_t safe_stop_latched;

void InfantryApp_ForceSafeStop(void)
{
    Chassis_Stop();
    Gimbal_Stop();
    Shoot_Stop();
}

void InfantryApp_Init(void)
{
    memset(&g_robot, 0, sizeof(g_robot));
    safe_stop_latched = 0U;

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

    (void)now_ms;
    Input_GetData(&input);
    MinimalReferee_Update();
    g_robot.referee = *MinimalReferee_GetData();

    if (input.emergency_stop || !input.online) {
        if (!safe_stop_latched) {
            MDBG_SYS("safe stop: estop=%u online=%u",
                     (unsigned)input.emergency_stop,
                     (unsigned)input.online);
            safe_stop_latched = 1U;
        }
        InfantryApp_ForceSafeStop();
        g_robot.input = input;
        return;
    }

    if (safe_stop_latched) {
        MDBG_SYS("safe stop released");
        safe_stop_latched = 0U;
    }

    /* Keep the minimal baseline's execution order unchanged. */
    Gimbal_Update(&input, Chassis_GetWz());
    Chassis_Update(&input);
    Shoot_Update(&input);
    g_robot.input = input;
}

void InfantryApp_DiagnosticsStep(uint32_t now_ms)
{
    MinimalDebug_UpdatePeriodic(now_ms);
}

const Robot_Context_t *InfantryApp_GetContext(void)
{
    return &g_robot;
}
