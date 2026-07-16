#include "safety_manager.h"

#include <stddef.h>

void RmSafety_Init(RmSafetyManager *manager)
{
    if (manager == NULL) {
        return;
    }

    manager->state = RM_SAFETY_STATE_BOOT;
    manager->reasons = RM_SAFETY_REASON_NOT_READY;
    manager->transition_count = 0U;
    manager->output_permitted = false;
}

bool RmSafety_Update(RmSafetyManager *manager, const RmSafetyInputs *inputs)
{
    RmSafetyState next_state;
    uint32_t next_reasons = RM_SAFETY_REASON_NONE;
    bool next_output_permitted;
    bool changed;

    if (manager == NULL) {
        return false;
    }

    if (inputs == NULL) {
        changed = (manager->state != RM_SAFETY_STATE_STOPPED) ||
                  (manager->reasons != RM_SAFETY_REASON_INVALID_INPUT) ||
                  manager->output_permitted;
        if (changed) {
            manager->state = RM_SAFETY_STATE_STOPPED;
            manager->reasons = RM_SAFETY_REASON_INVALID_INPUT;
            manager->output_permitted = false;
            manager->transition_count++;
        }
        return changed;
    }

    if (!inputs->initialization_complete) {
        next_reasons |= RM_SAFETY_REASON_NOT_READY;
    }
    if (!inputs->input_online) {
        next_reasons |= RM_SAFETY_REASON_INPUT_OFFLINE;
    }
    if (inputs->emergency_stop) {
        next_reasons |= RM_SAFETY_REASON_EMERGENCY_STOP;
    }
    if (!inputs->task_health_ok) {
        next_reasons |= RM_SAFETY_REASON_TASK_UNHEALTHY;
    }
    if (!inputs->device_health_ok) {
        next_reasons |= RM_SAFETY_REASON_DEVICE_UNHEALTHY;
    }

    if (next_reasons == RM_SAFETY_REASON_NONE) {
        next_state = RM_SAFETY_STATE_ACTIVE;
    } else if ((next_reasons & RM_SAFETY_REASON_NOT_READY) != 0U) {
        next_state = RM_SAFETY_STATE_DISARMED;
    } else {
        next_state = RM_SAFETY_STATE_STOPPED;
    }
    next_output_permitted = (next_state == RM_SAFETY_STATE_ACTIVE);

    changed = (manager->state != next_state) ||
              (manager->reasons != next_reasons) ||
              (manager->output_permitted != next_output_permitted);
    if (changed) {
        manager->state = next_state;
        manager->reasons = next_reasons;
        manager->output_permitted = next_output_permitted;
        manager->transition_count++;
    }

    return changed;
}

bool RmSafety_OutputPermitted(const RmSafetyManager *manager)
{
    return (manager != NULL) && manager->output_permitted;
}
