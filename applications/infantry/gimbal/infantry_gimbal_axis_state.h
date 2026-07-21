#ifndef INFANTRY_GIMBAL_AXIS_STATE_H
#define INFANTRY_GIMBAL_AXIS_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "infantry_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float brake_speed_epsilon;
    uint8_t brake_stable_count_required;
    uint32_t brake_timeout_ms;
} InfantryGimbalAxisStateConfig;

typedef struct {
    AxisCtrlMode_e mode;
    uint8_t brake_stable_count;
    uint32_t brake_start_ms;
} InfantryGimbalAxisState;

typedef struct {
    AxisCtrlMode_e previous_mode;
    AxisCtrlMode_e current_mode;
    bool changed;
} InfantryGimbalAxisTransition;

void InfantryGimbalAxisState_Init(InfantryGimbalAxisState *state);

bool InfantryGimbalAxisState_Update(
    InfantryGimbalAxisState *state,
    const InfantryGimbalAxisStateConfig *config,
    bool command_active,
    float speed_feedback,
    uint32_t now_ms,
    InfantryGimbalAxisTransition *transition);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_GIMBAL_AXIS_STATE_H */
