#include "infantry_gimbal_axis_state.h"

#include <math.h>
#include <stddef.h>

static bool ConfigIsValid(const InfantryGimbalAxisStateConfig *config)
{
    return config != NULL && isfinite(config->brake_speed_epsilon) &&
           config->brake_speed_epsilon >= 0.0f &&
           config->brake_stable_count_required > 0U &&
           config->brake_timeout_ms > 0U;
}

void InfantryGimbalAxisState_Init(InfantryGimbalAxisState *state)
{
    if (state == NULL) {
        return;
    }
    state->mode = AXIS_CTRL_ANGLE;
    state->brake_stable_count = 0U;
    state->brake_start_ms = 0U;
}

bool InfantryGimbalAxisState_Update(
    InfantryGimbalAxisState *state,
    const InfantryGimbalAxisStateConfig *config,
    bool command_active,
    float speed_feedback,
    uint32_t now_ms,
    InfantryGimbalAxisTransition *transition)
{
    AxisCtrlMode_e previous_mode;

    if (state == NULL) {
        return false;
    }
    if (!ConfigIsValid(config) || !isfinite(speed_feedback)) {
        InfantryGimbalAxisState_Init(state);
        return false;
    }
    if (state->mode != AXIS_CTRL_ANGLE &&
        state->mode != AXIS_CTRL_SPEED &&
        state->mode != AXIS_CTRL_BRAKE) {
        InfantryGimbalAxisState_Init(state);
        return false;
    }

    previous_mode = state->mode;
    if (command_active) {
        state->mode = AXIS_CTRL_SPEED;
        state->brake_stable_count = 0U;
        state->brake_start_ms = 0U;
    } else if (state->mode == AXIS_CTRL_SPEED) {
        state->mode = AXIS_CTRL_BRAKE;
        state->brake_stable_count = 0U;
        state->brake_start_ms = now_ms;
    } else if (state->mode == AXIS_CTRL_BRAKE) {
        if (fabsf(speed_feedback) <= config->brake_speed_epsilon) {
            if (state->brake_stable_count < UINT8_MAX) {
                state->brake_stable_count++;
            }
        } else {
            state->brake_stable_count = 0U;
        }

        if (state->brake_stable_count >=
                config->brake_stable_count_required ||
            (uint32_t)(now_ms - state->brake_start_ms) >=
                config->brake_timeout_ms) {
            state->mode = AXIS_CTRL_ANGLE;
            state->brake_stable_count = 0U;
            state->brake_start_ms = 0U;
        }
    }

    if (transition != NULL) {
        transition->previous_mode = previous_mode;
        transition->current_mode = state->mode;
        transition->changed = previous_mode != state->mode;
    }
    return true;
}
