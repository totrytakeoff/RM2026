/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Hero shooter remote control demo
 ******************************************************************************
 * @attention
 *
 * 控制逻辑：
 * - 左拨杆上：开启摩擦轮
 * - 右拨杆：下单发、中双连发、上连发
 * - 拨轮向上（顺时针）：触发拨弹
 *
 * 电机配置：
 * - 摩擦轮：CAN2 ID 1/2
 * - 拨弹：CAN1 ID 5
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "cmsis_os.h"
#include "crc.h"
#include "dac.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp_init.h"
#include "bsp_log.h"
#include "daemon.h"
#include "remote_control.h"
#include "dji_motor.h"
#include "user_lib.h"

/* Private define ------------------------------------------------------------*/
#define SHOOT_UPDATE_INTERVAL_MS 20U  // 50Hz

#define FRICTION_MOTOR_COUNT 2U
static const uint8_t FRICTION_CAN_IDS[FRICTION_MOTOR_COUNT] = {1U, 2U};
#define LOADER_CAN_ID 5U

#define FRICTION_SPEED_MAX 30000.0f  // deg/s
#define FRICTION_SPEED_MIN (-FRICTION_SPEED_MAX)
#define FRICTION_SPEED_TARGET 15000.0f

#define LOADER_SPEED_MAX 12000.0f
#define LOADER_SPEED_MIN (-LOADER_SPEED_MAX)
#define LOADER_ANGLE_MAX 36000.0f
#define LOADER_ANGLE_MIN (-LOADER_ANGLE_MAX)
#define LOADER_GEAR_RATIO 19.0f
#define LOADER_OUTPUT_STEP_DEG 60.0f
#define LOADER_ANGLE_STEP_DEG (-LOADER_OUTPUT_STEP_DEG * LOADER_GEAR_RATIO)

#define SHOOT_INTERVAL_MS 2000U
#define LOADER_CONTINUOUS_SPEED (-12000.0f)

/* Private variables ---------------------------------------------------------*/
static RC_ctrl_t *rc_data = NULL;
static DJIMotorInstance *friction_motors[FRICTION_MOTOR_COUNT] = {NULL};
static DJIMotorInstance *loader_motor = NULL;

static uint8_t friction_enabled = 0;
static uint8_t fire_mode = 0; // 0: single, 1: double, 2: continuous

static uint8_t pending_shots = 0;
static uint8_t loader_initialized = 0;
static float loader_target_angle = 0.0f;
static uint32_t last_shot_tick = 0;
static uint32_t last_step_tick = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void Debug_DisableWatchdogs(void);
static void EnsureFrictionMotorsReady(void);
static void EnsureLoaderMotorReady(void);
static void ProcessRemoteControl(void);
static void UpdateFrictionControl(void);
static void UpdateLoaderControl(void);
static float ClampFloat(float value, float min, float max);

/* Private user code ---------------------------------------------------------*/

static void Debug_DisableWatchdogs(void)
{
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}

static void EnsureFrictionMotorsReady(void)
{
    for (uint8_t i = 0; i < FRICTION_MOTOR_COUNT; ++i) {
        if (friction_motors[i] != NULL) {
            continue;
        }

        Motor_Init_Config_s config = {
            .can_init_config = {
                .can_handle = &hcan2,
                .tx_id = FRICTION_CAN_IDS[i],
            },
            .controller_param_init_config = {
                .angle_PID = {
                    .Kp = 5.0f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .MaxOut = FRICTION_SPEED_MAX,
                    .IntegralLimit = 500.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                },
                .speed_PID = {
                    .Kp = 10.0f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .IntegralLimit = 3000.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit |
                               PID_Derivative_On_Measurement,
                    .MaxOut = 12000.0f,
                },
                .current_PID = {
                    .Kp = 0.5f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .IntegralLimit = 3000.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit |
                               PID_Derivative_On_Measurement,
                    .MaxOut = 15000.0f,
                },
            },
            .controller_setting_init_config = {
                .angle_feedback_source = MOTOR_FEED,
                .speed_feedback_source = MOTOR_FEED,
                .outer_loop_type = SPEED_LOOP,
                .close_loop_type = SPEED_LOOP | CURRENT_LOOP,
                .motor_reverse_flag = (i == 1U) ? MOTOR_DIRECTION_REVERSE
                                                : MOTOR_DIRECTION_NORMAL,
            },
            .motor_type = M3508,
        };

        friction_motors[i] = DJIMotorInit(&config);
    }
}

static void EnsureLoaderMotorReady(void)
{
    if (loader_motor != NULL) {
        return;
    }

    Motor_Init_Config_s config = {
        .can_init_config = {
            .can_handle = &hcan1,
            .tx_id = LOADER_CAN_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = 5.0f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .MaxOut = LOADER_SPEED_MAX,
                .IntegralLimit = 500.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
            },
            .speed_PID = {
                .Kp = 12.0f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .IntegralLimit = 3000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit |
                           PID_Derivative_On_Measurement,
                .MaxOut = 16000.0f,
            },
            .current_PID = {
                .Kp = 0.6f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .IntegralLimit = 3000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit |
                           PID_Derivative_On_Measurement,
                .MaxOut = 20000.0f,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP | CURRENT_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
        .motor_type = M3508,
    };

    loader_motor = DJIMotorInit(&config);
}

static void ProcessRemoteControl(void)
{
    if (rc_data == NULL || !RemoteControlIsOnline()) {
        friction_enabled = 0;
        return;
    }

    const RC_ctrl_t *rc = &rc_data[TEMP];

    friction_enabled = switch_is_up(rc->rc.switch_left);

    if (switch_is_down(rc->rc.switch_right)) {
        fire_mode = 0;
    } else if (switch_is_mid(rc->rc.switch_right)) {
        fire_mode = 1;
    } else {
        fire_mode = 2;
    }

    if (!friction_enabled) {
        return;
    }

    if (fire_mode == 2) {
        pending_shots = 0;
        return;
    }

    if (pending_shots == 0) {
        uint32_t now = HAL_GetTick();
        if (now - last_shot_tick >= SHOOT_INTERVAL_MS) {
            pending_shots = (fire_mode == 0) ? 1 : 2;
            last_shot_tick = now;
            last_step_tick = now - SHOOT_INTERVAL_MS;
        }
    }
}

static void UpdateFrictionControl(void)
{
    EnsureFrictionMotorsReady();

    if (!friction_enabled) {
        for (uint8_t i = 0; i < FRICTION_MOTOR_COUNT; ++i) {
            if (friction_motors[i] != NULL) {
                DJIMotorStop(friction_motors[i]);
            }
        }
        return;
    }

    float target_speed = ClampFloat(FRICTION_SPEED_TARGET, FRICTION_SPEED_MIN, FRICTION_SPEED_MAX);
    for (uint8_t i = 0; i < FRICTION_MOTOR_COUNT; ++i) {
        DJIMotorInstance *motor = friction_motors[i];
        if (motor == NULL) {
            continue;
        }
        DJIMotorOuterLoop(motor, SPEED_LOOP);
        DJIMotorEnable(motor);
        DJIMotorSetRef(motor, target_speed);
    }
}

static void UpdateLoaderControl(void)
{
    EnsureLoaderMotorReady();

    if (!friction_enabled || loader_motor == NULL) {
        if (loader_motor != NULL) {
            DJIMotorStop(loader_motor);
        }
        pending_shots = 0;
        loader_initialized = 0;
        return;
    }

    uint32_t now = HAL_GetTick();
    if (fire_mode == 2) {
        pending_shots = 0;
        loader_initialized = 0;
        float speed_ref = ClampFloat(LOADER_CONTINUOUS_SPEED, LOADER_SPEED_MIN, LOADER_SPEED_MAX);
        DJIMotorOuterLoop(loader_motor, SPEED_LOOP);
        DJIMotorEnable(loader_motor);
        DJIMotorSetRef(loader_motor, speed_ref);
        return;
    }

    if (!loader_initialized) {
        loader_target_angle = ClampFloat(loader_motor->measure.total_angle, LOADER_ANGLE_MIN, LOADER_ANGLE_MAX);
        if (pending_shots > 0) {
            last_step_tick = now - SHOOT_INTERVAL_MS;
        } else {
            last_step_tick = now;
        }
        loader_initialized = 1;
    }

    if (pending_shots > 0) {
        uint32_t elapsed = now - last_step_tick;
        if (elapsed >= SHOOT_INTERVAL_MS) {
            uint32_t steps = elapsed / SHOOT_INTERVAL_MS;
            if (steps > pending_shots) {
                steps = pending_shots;
            }
            last_step_tick += steps * SHOOT_INTERVAL_MS;
            loader_target_angle += LOADER_ANGLE_STEP_DEG * (float)steps;
            loader_target_angle = ClampFloat(loader_target_angle, LOADER_ANGLE_MIN, LOADER_ANGLE_MAX);
            pending_shots -= (uint8_t)steps;
        }
    } else {
        DJIMotorStop(loader_motor);
        return;
    }

    DJIMotorOuterLoop(loader_motor, ANGLE_LOOP);
    DJIMotorEnable(loader_motor);
    DJIMotorSetRef(loader_motor, loader_target_angle);
}

static float ClampFloat(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

int main(void)
{
    HAL_Init();
    Debug_DisableWatchdogs();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    MX_USART3_UART_Init();
    MX_USART6_UART_Init();

    BSPInit();

    rc_data = RemoteControlInit(&huart3);

    LOGINFO("[hero_shoot] demo initialized");
    LOGINFO("[hero_shoot] left switch up -> friction on");
    LOGINFO("[hero_shoot] right switch: down single, mid double, up continuous");
    LOGINFO("[hero_shoot] loader runs continuously with the selected mode");

    uint32_t last_update_tick = 0;

    while (1) {
        DaemonTask();
        DJIMotorControl();

        uint32_t now = HAL_GetTick();
        if (now - last_update_tick >= SHOOT_UPDATE_INTERVAL_MS) {
            last_update_tick = now;

            ProcessRemoteControl();
            UpdateFrictionControl();
            UpdateLoaderControl();
        }

        HAL_Delay(5);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        HAL_IncTick();
    }
}
