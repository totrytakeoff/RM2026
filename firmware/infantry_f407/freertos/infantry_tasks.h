#ifndef INFANTRY_TASKS_H
#define INFANTRY_TASKS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INFANTRY_TASK_INS = 0,
    INFANTRY_TASK_MOTOR,
    INFANTRY_TASK_CONTROL,
    INFANTRY_TASK_HEALTH,
    INFANTRY_TASK_DIAGNOSTICS,
    INFANTRY_TASK_TUNING_TELEMETRY,
    INFANTRY_TASK_COUNT,
} InfantryTaskId;

typedef struct {
    uint32_t period_us;
    uint32_t run_count;
    uint32_t deadline_miss_count;
    uint32_t consecutive_deadline_misses;
    uint32_t last_start_interval_us;
    uint32_t max_start_interval_us;
    uint32_t last_execution_us;
    uint32_t max_execution_us;
    uint32_t stack_high_water_words;
    uint32_t last_run_tick;
} InfantryTaskRuntime;

typedef struct {
    InfantryTaskRuntime tasks[INFANTRY_TASK_COUNT];
    uint32_t unhealthy_mask;
} InfantryTaskHealthSnapshot;

/** Create all application tasks using caller-owned static storage. */
bool InfantryTasks_Create(void);

/** Copy the latest runtime counters for telemetry or debugger inspection. */
bool InfantryTasks_GetHealthSnapshot(InfantryTaskHealthSnapshot *snapshot);

#endif /* INFANTRY_TASKS_H */
