#ifndef INFANTRY_APP_H
#define INFANTRY_APP_H

#include <stdint.h>

#include "infantry_types.h"

/** Initialize the infantry modules. Call before the scheduler starts. */
void InfantryApp_Init(void);

/** Execute one deterministic high-level control step. */
void InfantryApp_ControlStep(uint32_t now_ms);

/** Execute non-critical, rate-limited diagnostics. */
void InfantryApp_DiagnosticsStep(uint32_t now_ms);

/** Immediately place every actuator module in its stopped state. */
void InfantryApp_ForceSafeStop(void);

/** Read-only access for transitional diagnostics. */
const Robot_Context_t *InfantryApp_GetContext(void);

#endif /* INFANTRY_APP_H */
