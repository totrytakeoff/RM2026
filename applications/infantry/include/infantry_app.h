#ifndef INFANTRY_APP_H
#define INFANTRY_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "infantry_types.h"
#include "safety_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the infantry modules. Call before the scheduler starts. */
bool InfantryApp_Init(void);

/** Execute the 5 ms actuator stage from the dedicated motor task. */
void InfantryApp_MotorStep(void);

/** Execute one deterministic high-level control step. */
void InfantryApp_ControlStep(uint32_t now_ms);

/** Execute non-critical, rate-limited diagnostics. */
void InfantryApp_DiagnosticsStep(uint32_t now_ms);

/** Send one dedicated, non-blocking tuning telemetry frame. */
void InfantryApp_TuningTelemetryStep(uint32_t now_ms);

/** Immediately place every actuator module in its stopped state. */
void InfantryApp_ForceSafeStop(void);

/** Publish scheduler health; the control task owns the safety transition. */
void InfantryApp_SetTaskHealth(bool healthy);

/** Current safety state for telemetry and hardware acceptance checks. */
RmSafetyState InfantryApp_GetSafetyState(void);

/** Active safety reasons as an RmSafetyReason bit mask. */
uint32_t InfantryApp_GetSafetyReasons(void);

/** Read-only access for transitional diagnostics. */
const Robot_Context_t *InfantryApp_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_APP_H */
