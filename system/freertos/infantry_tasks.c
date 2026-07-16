#include "infantry_tasks.h"

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_dwt.h"
#include "bsp_can.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "dji_motor.h"
#include "infantry_app.h"
#include "infantry_config.h"
#include "ins_task.h"
#include "rm_time.h"

enum {
    INS_TASK_STACK_WORDS = 1024,
    MOTOR_TASK_STACK_WORDS = 384,
    CONTROL_TASK_STACK_WORDS = 768,
    HEALTH_TASK_STACK_WORDS = 256,
    DIAGNOSTICS_TASK_STACK_WORDS = 384,
};

static StaticTask_t ins_task_tcb;
static StaticTask_t motor_task_tcb;
static StaticTask_t control_task_tcb;
static StaticTask_t health_task_tcb;
static StaticTask_t diagnostics_task_tcb;

static StackType_t ins_task_stack[INS_TASK_STACK_WORDS];
static StackType_t motor_task_stack[MOTOR_TASK_STACK_WORDS];
static StackType_t control_task_stack[CONTROL_TASK_STACK_WORDS];
static StackType_t health_task_stack[HEALTH_TASK_STACK_WORDS];
static StackType_t diagnostics_task_stack[DIAGNOSTICS_TASK_STACK_WORDS];

static TaskHandle_t task_handles[INFANTRY_TASK_COUNT];
static volatile InfantryTaskRuntime task_runtime[INFANTRY_TASK_COUNT];
static volatile uint32_t unhealthy_mask;
static uint32_t task_last_start_cycles[INFANTRY_TASK_COUNT];
static uint32_t task_started_mask;
static bool task_release_late[INFANTRY_TASK_COUNT];

static const uint32_t task_period_us[INFANTRY_TASK_COUNT] = {
    [INFANTRY_TASK_INS] = INFANTRY_INS_TASK_PERIOD_MS * 1000U,
    [INFANTRY_TASK_MOTOR] = INFANTRY_MOTOR_TASK_PERIOD_MS * 1000U,
    [INFANTRY_TASK_CONTROL] = MAIN_LOOP_PERIOD_MS * 1000U,
    [INFANTRY_TASK_HEALTH] = INFANTRY_HEALTH_TASK_PERIOD_MS * 1000U,
    [INFANTRY_TASK_DIAGNOSTICS] = INFANTRY_DIAGNOSTICS_TASK_PERIOD_MS * 1000U,
};

static const uint32_t task_stack_words[INFANTRY_TASK_COUNT] = {
    [INFANTRY_TASK_INS] = INS_TASK_STACK_WORDS,
    [INFANTRY_TASK_MOTOR] = MOTOR_TASK_STACK_WORDS,
    [INFANTRY_TASK_CONTROL] = CONTROL_TASK_STACK_WORDS,
    [INFANTRY_TASK_HEALTH] = HEALTH_TASK_STACK_WORDS,
    [INFANTRY_TASK_DIAGNOSTICS] = DIAGNOSTICS_TASK_STACK_WORDS,
};

static uint32_t TaskRuntime_CyclesToUs(uint32_t cycles)
{
    return (cycles + INFANTRY_CPU_FREQUENCY_MHZ - 1U) /
           INFANTRY_CPU_FREQUENCY_MHZ;
}

static uint32_t TaskRuntime_Begin(InfantryTaskId id)
{
    volatile InfantryTaskRuntime *runtime = &task_runtime[id];
    uint32_t now_cycles = DWT->CYCCNT;
    uint32_t id_mask = 1UL << (uint32_t)id;

    task_release_late[id] = false;
    if ((task_started_mask & id_mask) != 0U) {
        uint32_t interval_us =
            TaskRuntime_CyclesToUs(now_cycles - task_last_start_cycles[id]);
        uint32_t interval_limit_us =
            runtime->period_us +
            ((runtime->period_us *
              INFANTRY_TASK_DEADLINE_TOLERANCE_PERCENT) /
             100U);

        runtime->last_start_interval_us = interval_us;
        if (interval_us > runtime->max_start_interval_us) {
            runtime->max_start_interval_us = interval_us;
        }
        task_release_late[id] = interval_us > interval_limit_us;
    } else {
        task_started_mask |= id_mask;
    }
    task_last_start_cycles[id] = now_cycles;
    return now_cycles;
}

static void TaskRuntime_Record(InfantryTaskId id, uint32_t start_cycles)
{
    volatile InfantryTaskRuntime *runtime = &task_runtime[id];
    uint32_t elapsed_cycles = DWT->CYCCNT - start_cycles;
    uint32_t execution_us = TaskRuntime_CyclesToUs(elapsed_cycles);

    runtime->last_execution_us = execution_us;
    if (execution_us > runtime->max_execution_us) {
        runtime->max_execution_us = execution_us;
    }

    runtime->run_count++;
    if ((execution_us >= runtime->period_us) || task_release_late[id]) {
        runtime->deadline_miss_count++;
        runtime->consecutive_deadline_misses++;
    } else {
        runtime->consecutive_deadline_misses = 0U;
    }

    if ((runtime->run_count == 1U) ||
        ((runtime->run_count % INFANTRY_TASK_STACK_SAMPLE_INTERVAL) == 0U)) {
        uint32_t free_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
        if (free_words < runtime->stack_high_water_words) {
            runtime->stack_high_water_words = free_words;
        }
    }

    runtime->last_run_tick = (uint32_t)xTaskGetTickCount();
}

static bool TaskRuntime_IsCritical(InfantryTaskId id)
{
    return id != INFANTRY_TASK_DIAGNOSTICS;
}

static bool TaskRuntime_AreCriticalTasksHealthy(TickType_t now)
{
    uint32_t mask = 0U;
    InfantryTaskId id;

    for (id = INFANTRY_TASK_INS; id < INFANTRY_TASK_COUNT; id++) {
        const volatile InfantryTaskRuntime *runtime = &task_runtime[id];
        TickType_t heartbeat_limit;

        if (!TaskRuntime_IsCritical(id)) {
            continue;
        }

        heartbeat_limit = pdMS_TO_TICKS(
            (runtime->period_us / 1000U) * INFANTRY_TASK_HEARTBEAT_PERIODS);
        if ((now > pdMS_TO_TICKS(INFANTRY_TASK_STARTUP_GRACE_MS)) &&
            ((now - (TickType_t)runtime->last_run_tick) > heartbeat_limit)) {
            mask |= (1UL << (uint32_t)id);
        }
        if (runtime->consecutive_deadline_misses >=
            INFANTRY_TASK_MAX_CONSECUTIVE_OVERRUNS) {
            mask |= (1UL << (uint32_t)id);
        }
        if (runtime->stack_high_water_words <
            INFANTRY_TASK_MIN_STACK_FREE_WORDS) {
            mask |= (1UL << (uint32_t)id);
        }
    }

    unhealthy_mask = mask;
    return mask == 0U;
}

static void TaskRuntime_Init(void)
{
    InfantryTaskId id;

    memset(task_handles, 0, sizeof(task_handles));
    memset((void *)task_runtime, 0, sizeof(task_runtime));
    memset(task_last_start_cycles, 0, sizeof(task_last_start_cycles));
    memset(task_release_late, 0, sizeof(task_release_late));
    task_started_mask = 0U;
    unhealthy_mask = 0U;

    for (id = INFANTRY_TASK_INS; id < INFANTRY_TASK_COUNT; id++) {
        task_runtime[id].period_us = task_period_us[id];
        task_runtime[id].stack_high_water_words = task_stack_words[id];
    }
}

static void InfantryInsTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        uint32_t start_cycles = TaskRuntime_Begin(INFANTRY_TASK_INS);
        INS_Task();
        TaskRuntime_Record(INFANTRY_TASK_INS, start_cycles);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_INS_TASK_PERIOD_MS));
    }
}

static void InfantryMotorTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        uint32_t start_cycles = TaskRuntime_Begin(INFANTRY_TASK_MOTOR);
        /* Parse bounded ingress snapshots before consuming motor feedback. */
        (void)CANDispatchPending(0U);
        (void)USARTDispatchPending(0U);
        DJIMotorControl();
        TaskRuntime_Record(INFANTRY_TASK_MOTOR, start_cycles);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_MOTOR_TASK_PERIOD_MS));
    }
}

static void InfantryControlTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        uint32_t start_cycles = TaskRuntime_Begin(INFANTRY_TASK_CONTROL);
        InfantryApp_ControlStep(RmTime_NowMs());
        TaskRuntime_Record(INFANTRY_TASK_CONTROL, start_cycles);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS));
    }
}

static void InfantryHealthTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        uint32_t start_cycles = TaskRuntime_Begin(INFANTRY_TASK_HEALTH);
        DaemonTask();
        TaskRuntime_Record(INFANTRY_TASK_HEALTH, start_cycles);
        InfantryApp_SetTaskHealth(
            TaskRuntime_AreCriticalTasksHealthy(xTaskGetTickCount()));
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_HEALTH_TASK_PERIOD_MS));
    }
}

static void InfantryDiagnosticsTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        uint32_t start_cycles = TaskRuntime_Begin(INFANTRY_TASK_DIAGNOSTICS);
        InfantryApp_DiagnosticsStep(RmTime_NowMs());
        TaskRuntime_Record(INFANTRY_TASK_DIAGNOSTICS, start_cycles);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_DIAGNOSTICS_TASK_PERIOD_MS));
    }
}

bool InfantryTasks_Create(void)
{
    TaskRuntime_Init();

    task_handles[INFANTRY_TASK_INS] = xTaskCreateStatic(InfantryInsTask,
                                                        "ins",
                                                        INS_TASK_STACK_WORDS,
                                                        NULL,
                                                        tskIDLE_PRIORITY + 4U,
                                                        ins_task_stack,
                                                        &ins_task_tcb);
    if (task_handles[INFANTRY_TASK_INS] == NULL) {
        return false;
    }

    task_handles[INFANTRY_TASK_MOTOR] = xTaskCreateStatic(InfantryMotorTask,
                                                          "motor",
                                                          MOTOR_TASK_STACK_WORDS,
                                                          NULL,
                                                          tskIDLE_PRIORITY + 3U,
                                                          motor_task_stack,
                                                          &motor_task_tcb);
    if (task_handles[INFANTRY_TASK_MOTOR] == NULL) {
        return false;
    }

    task_handles[INFANTRY_TASK_HEALTH] = xTaskCreateStatic(InfantryHealthTask,
                                                           "health",
                                                           HEALTH_TASK_STACK_WORDS,
                                                           NULL,
                                                           tskIDLE_PRIORITY + 3U,
                                                           health_task_stack,
                                                           &health_task_tcb);
    if (task_handles[INFANTRY_TASK_HEALTH] == NULL) {
        return false;
    }

    task_handles[INFANTRY_TASK_CONTROL] = xTaskCreateStatic(InfantryControlTask,
                                                            "control",
                                                            CONTROL_TASK_STACK_WORDS,
                                                            NULL,
                                                            tskIDLE_PRIORITY + 2U,
                                                            control_task_stack,
                                                            &control_task_tcb);
    if (task_handles[INFANTRY_TASK_CONTROL] == NULL) {
        return false;
    }

    task_handles[INFANTRY_TASK_DIAGNOSTICS] =
        xTaskCreateStatic(InfantryDiagnosticsTask,
                          "diagnostics",
                          DIAGNOSTICS_TASK_STACK_WORDS,
                          NULL,
                          tskIDLE_PRIORITY + 1U,
                          diagnostics_task_stack,
                          &diagnostics_task_tcb);
    return task_handles[INFANTRY_TASK_DIAGNOSTICS] != NULL;
}

bool InfantryTasks_GetHealthSnapshot(InfantryTaskHealthSnapshot *snapshot)
{
    InfantryTaskId id;

    if (snapshot == NULL) {
        return false;
    }

    taskENTER_CRITICAL();
    for (id = INFANTRY_TASK_INS; id < INFANTRY_TASK_COUNT; id++) {
        snapshot->tasks[id].period_us = task_runtime[id].period_us;
        snapshot->tasks[id].run_count = task_runtime[id].run_count;
        snapshot->tasks[id].deadline_miss_count =
            task_runtime[id].deadline_miss_count;
        snapshot->tasks[id].consecutive_deadline_misses =
            task_runtime[id].consecutive_deadline_misses;
        snapshot->tasks[id].last_start_interval_us =
            task_runtime[id].last_start_interval_us;
        snapshot->tasks[id].max_start_interval_us =
            task_runtime[id].max_start_interval_us;
        snapshot->tasks[id].last_execution_us =
            task_runtime[id].last_execution_us;
        snapshot->tasks[id].max_execution_us =
            task_runtime[id].max_execution_us;
        snapshot->tasks[id].stack_high_water_words =
            task_runtime[id].stack_high_water_words;
        snapshot->tasks[id].last_run_tick = task_runtime[id].last_run_tick;
    }
    snapshot->unhealthy_mask = unhealthy_mask;
    taskEXIT_CRITICAL();

    return true;
}
