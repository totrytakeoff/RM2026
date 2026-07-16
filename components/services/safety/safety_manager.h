#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RM_SAFETY_STATE_BOOT = 0,
    RM_SAFETY_STATE_DISARMED,
    RM_SAFETY_STATE_ACTIVE,
    RM_SAFETY_STATE_STOPPED,
} RmSafetyState;

typedef enum {
    RM_SAFETY_REASON_NONE = 0U,
    RM_SAFETY_REASON_NOT_READY = (1U << 0),
    RM_SAFETY_REASON_INPUT_OFFLINE = (1U << 1),
    RM_SAFETY_REASON_EMERGENCY_STOP = (1U << 2),
    RM_SAFETY_REASON_TASK_UNHEALTHY = (1U << 3),
    RM_SAFETY_REASON_INVALID_INPUT = (1U << 4),
    RM_SAFETY_REASON_DEVICE_UNHEALTHY = (1U << 5),
} RmSafetyReason;

typedef struct {
    bool initialization_complete;
    bool input_online;
    bool emergency_stop;
    bool task_health_ok;
    bool device_health_ok;
} RmSafetyInputs;

typedef struct {
    RmSafetyState state;
    uint32_t reasons;
    uint32_t transition_count;
    bool output_permitted;
} RmSafetyManager;

/** Initialize a manager in an output-disabled boot state. */
void RmSafety_Init(RmSafetyManager *manager);

/**
 * Evaluate all safety inputs. Returns true only when observable state changed.
 * A null input snapshot transitions a valid manager to a stopped fail-safe.
 *
 * Fault recovery is automatic once every input is healthy. This deliberately
 * preserves the current infantry baseline and can later be replaced by an
 * explicit re-arm policy without changing actuator modules.
 */
bool RmSafety_Update(RmSafetyManager *manager, const RmSafetyInputs *inputs);

/** Outputs may be updated only while the manager is active. */
bool RmSafety_OutputPermitted(const RmSafetyManager *manager);

#endif /* SAFETY_MANAGER_H */
