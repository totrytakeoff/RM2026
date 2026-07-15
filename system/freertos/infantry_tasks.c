#include "infantry_tasks.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_dwt.h"
#include "daemon.h"
#include "dji_motor.h"
#include "infantry_app.h"
#include "infantry_config.h"
#include "ins_task.h"

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

static void InfantryInsTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        INS_Task();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_INS_TASK_PERIOD_MS));
    }
}

static void InfantryMotorTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        DJIMotorControl();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_MOTOR_TASK_PERIOD_MS));
    }
}

static void InfantryControlTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        InfantryApp_ControlStep(DWT_GetTimeline_ms());
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS));
    }
}

static void InfantryHealthTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        DaemonTask();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_HEALTH_TASK_PERIOD_MS));
    }
}

static void InfantryDiagnosticsTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    (void)argument;

    for (;;) {
        InfantryApp_DiagnosticsStep(DWT_GetTimeline_ms());
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(INFANTRY_DIAGNOSTICS_TASK_PERIOD_MS));
    }
}

bool InfantryTasks_Create(void)
{
    TaskHandle_t handle;

    handle = xTaskCreateStatic(InfantryInsTask,
                               "ins",
                               INS_TASK_STACK_WORDS,
                               NULL,
                               tskIDLE_PRIORITY + 4U,
                               ins_task_stack,
                               &ins_task_tcb);
    if (handle == NULL) {
        return false;
    }

    handle = xTaskCreateStatic(InfantryMotorTask,
                               "motor",
                               MOTOR_TASK_STACK_WORDS,
                               NULL,
                               tskIDLE_PRIORITY + 3U,
                               motor_task_stack,
                               &motor_task_tcb);
    if (handle == NULL) {
        return false;
    }

    handle = xTaskCreateStatic(InfantryHealthTask,
                               "health",
                               HEALTH_TASK_STACK_WORDS,
                               NULL,
                               tskIDLE_PRIORITY + 3U,
                               health_task_stack,
                               &health_task_tcb);
    if (handle == NULL) {
        return false;
    }

    handle = xTaskCreateStatic(InfantryControlTask,
                               "control",
                               CONTROL_TASK_STACK_WORDS,
                               NULL,
                               tskIDLE_PRIORITY + 2U,
                               control_task_stack,
                               &control_task_tcb);
    if (handle == NULL) {
        return false;
    }

    handle = xTaskCreateStatic(InfantryDiagnosticsTask,
                               "diagnostics",
                               DIAGNOSTICS_TASK_STACK_WORDS,
                               NULL,
                               tskIDLE_PRIORITY + 1U,
                               diagnostics_task_stack,
                               &diagnostics_task_tcb);
    return handle != NULL;
}
