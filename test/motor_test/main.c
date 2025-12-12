/**
 * @file main.c
 * @brief Standalone firmware that exercises DJI motors from the RM2026
 *        framework. It exposes helper functions so you can attach a debugger
 *        and call the speed/position-loop tests manually.
 */
#include "main.h"

#include "can.h"
#include "dma.h"
#include "gpio.h"
#include "tim.h"

#include "bsp_init.h"
#include "bsp_log.h"
#include "daemon.h"
#include "dm8009p.h"
#include "dji_motor.h"

#define M3508_MOTOR_COUNT 4U
static const uint8_t M3508_CAN_IDS[M3508_MOTOR_COUNT] = {1U, 2U, 3U, 4U};
#define GM6020_CAN_ID 5U

#define M3508_SPEED_MAX 7200.0f   // deg/s, ~20 rps
#define M3508_SPEED_MIN (-M3508_SPEED_MAX)
#define M3508_ANGLE_MAX 36000.0f  // deg, allow ~100 turns for demos
#define M3508_ANGLE_MIN (-M3508_ANGLE_MAX)

#define GM6020_SPEED_MAX 3600.0f  // deg/s, conservative limit
#define GM6020_SPEED_MIN (-GM6020_SPEED_MAX)
#define GM6020_ANGLE_MAX 1440.0f  // deg
#define GM6020_ANGLE_MIN (-GM6020_ANGLE_MAX)

#define DM8009P_CAN_CMD_ID 0x01U
#define DM8009P_MASTER_ID 0x000U
#define DM8009P_TARGET_SPEED_RAD_S 6.0f
#define DM8009P_DEFAULT_DAMPING 4.0f

static DJIMotorInstance *m3508_motors[M3508_MOTOR_COUNT] = {NULL}; // Lazily created M3508 instances
static DJIMotorInstance *gm6020_motor = NULL; // Lazily created GM6020 instance
static DM8009P_Handle *dm8009p_motor = NULL;
static uint8_t dm8009p_speed_mode_enabled = 0;

void SystemClock_Config(void);
static void EnsureM3508MotorReady(void);
static void EnsureGM6020MotorReady(void);
static void EnsureDM8009PMotorReady(void);
static float ClampFloat(float value, float min, float max);

void MotorTest_StopAll(void);
void MotorTest_M3508_SpeedLoop(float target_speed_deg_s);
void MotorTest_M3508_PositionLoop(float target_angle_deg);
void MotorTest_M3508_PeriodicAngleStep(float step_deg, uint32_t interval_ms);
void MotorTest_GM6020_SpeedLoop(float target_speed_deg_s);
void MotorTest_GM6020_PositionLoop(float target_angle_deg);
void MotorTest_GM6020_PeriodicAngleStep(float step_deg, uint32_t interval_ms);
void MotorTest_DM8009P_SpeedLoop(float target_speed_rad_s);


/**
 * @brief Bare-metal entry: initialise HAL/BSP, bring up CAN1 and continuously
 *        pump the motor + daemon tasks so the test helpers can be invoked from
 *        a debugger or GDB.
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CAN1_Init();

    BSPInit();
    LOGINFO("[motor_test] core init finished");



    MotorTest_M3508_SpeedLoop(7200.0f);


    while (1)
    {
        // MotorTest_M3508_PeriodicAngleStep(60.0f, 1000U);
        // MotorTest_GM6020_PeriodicAngleStep(60.0f, 1000U);
        // MotorTest_DM8009P_SpeedLoop(DM8009P_TARGET_SPEED_RAD_S);
        DJIMotorControl();
        DaemonTask();
        HAL_Delay(2);
    }
}

/**
 * @brief Force every registered motor into stop mode. Useful when finishing a
 *        test or recovering from an unexpected state.
 */
void MotorTest_StopAll(void)
{
    for (uint8_t i = 0; i < M3508_MOTOR_COUNT; ++i)
    {
        if (m3508_motors[i] != NULL)
        {
            DJIMotorStop(m3508_motors[i]);
        }
    }
    if (gm6020_motor != NULL)
    {
        DJIMotorStop(gm6020_motor);
    }
    if (dm8009p_motor != NULL)
    {
        DM8009P_Disable(dm8009p_motor, DM8009P_MODE_SPEED);
        dm8009p_speed_mode_enabled = 0;
    }
    LOGINFO("[motor_test] all motors stopped");
}

/**
 * @brief Run the M3508 in a speed-loop (speed+current) on CAN1.
 *
 * @param target_speed_deg_s Desired speed in deg/s. The helper clamps it to a
 *                           safe range before forwarding to the motor module.
 */
void MotorTest_M3508_SpeedLoop(float target_speed_deg_s)
{
    EnsureM3508MotorReady();
    target_speed_deg_s = ClampFloat(target_speed_deg_s, M3508_SPEED_MIN, M3508_SPEED_MAX);
    for (uint8_t i = 0; i < M3508_MOTOR_COUNT; ++i)
    {
        if (m3508_motors[i] == NULL)
        {
            continue;
        }
        DJIMotorOuterLoop(m3508_motors[i], SPEED_LOOP);
        DJIMotorEnable(m3508_motors[i]);
        DJIMotorSetRef(m3508_motors[i], target_speed_deg_s);
    }
    LOGINFO("[motor_test] %u x M3508 speed ref %d deg/s", (unsigned)M3508_MOTOR_COUNT, (int)target_speed_deg_s);
}

/**
 * @brief Run the M3508 in a cascaded position loop (angle -> speed -> current).
 *
 * @param target_angle_deg Desired total angle. Uses the module’s total-angle
 *                         accumulator so you can step multiple turns.
 */
void MotorTest_M3508_PositionLoop(float target_angle_deg)
{
    EnsureM3508MotorReady();
    target_angle_deg = ClampFloat(target_angle_deg, M3508_ANGLE_MIN, M3508_ANGLE_MAX);
    for (uint8_t i = 0; i < M3508_MOTOR_COUNT; ++i)
    {
        if (m3508_motors[i] == NULL)
        {
            continue;
        }
        DJIMotorOuterLoop(m3508_motors[i], ANGLE_LOOP);
        DJIMotorEnable(m3508_motors[i]);
        DJIMotorSetRef(m3508_motors[i], target_angle_deg);
    }
    LOGINFO("[motor_test] %u x M3508 angle ref %d deg", (unsigned)M3508_MOTOR_COUNT, (int)target_angle_deg);
}

/**
 * @brief Demo helper: periodically increase the M3508角度参考值，每隔 interval_ms 毫秒累加 step_deg。
 *        用于验证减速箱电机按恒定角速度(例如1s 60°)运行时的表现。
 *
 * @param step_deg    Angle increment (deg) applied every period. Positive values rotate forward.
 * @param interval_ms Interval between increments in milliseconds.
 */
void MotorTest_M3508_PeriodicAngleStep(float step_deg, uint32_t interval_ms)
{
    EnsureM3508MotorReady();

    static uint8_t step_state_initialized[M3508_MOTOR_COUNT] = {0};
    static float current_target[M3508_MOTOR_COUNT] = {0.0f};
    static uint32_t last_step_tick[M3508_MOTOR_COUNT] = {0};

    uint32_t now = HAL_GetTick();
    for (uint8_t i = 0; i < M3508_MOTOR_COUNT; ++i)
    {
        DJIMotorInstance *motor = m3508_motors[i];
        if (motor == NULL)
        {
            continue;
        }

        if (!step_state_initialized[i])
        {
            current_target[i] = ClampFloat(motor->measure.total_angle, M3508_ANGLE_MIN, M3508_ANGLE_MAX);
            last_step_tick[i] = now;
            step_state_initialized[i] = 1;
            LOGINFO("[motor_test] M3508[%d] step demo start angle %d deg", (int)M3508_CAN_IDS[i], (int)current_target[i]);
        }

        uint32_t elapsed = now - last_step_tick[i];
        if (interval_ms != 0U && elapsed >= interval_ms)
        {
            uint32_t steps = elapsed / interval_ms;
            last_step_tick[i] += steps * interval_ms;
            current_target[i] += step_deg * (float)steps;
            current_target[i] = ClampFloat(current_target[i], M3508_ANGLE_MIN, M3508_ANGLE_MAX);
            LOGINFO("[motor_test] M3508[%d] step target -> %d deg", (int)M3508_CAN_IDS[i], (int)current_target[i]);
        }

        DJIMotorOuterLoop(motor, ANGLE_LOOP);
        DJIMotorEnable(motor);
        DJIMotorSetRef(motor, current_target[i]);
    }
}

/**
 * @brief GM6020 角度阶跃：类似于 3508 的 demo，每隔 interval_ms ms 将位置参考累加 step_deg。
 *
 * @param step_deg    每次递增角度（deg）。
 * @param interval_ms 每次递增之间的间隔（ms）。
 */
void MotorTest_GM6020_PeriodicAngleStep(float step_deg, uint32_t interval_ms)
{
    EnsureGM6020MotorReady();

    static uint8_t step_state_initialized = 0;
    static float current_target = 0.0f;
    static uint32_t last_step_tick = 0;

    uint32_t now = HAL_GetTick();
    if (!step_state_initialized)
    {
        current_target = gm6020_motor->measure.total_angle;
        last_step_tick = now;
        step_state_initialized = 1;
        LOGINFO("[motor_test] GM6020 step demo start angle %d deg", (int)current_target);
    }

    uint32_t elapsed = now - last_step_tick;
    if (interval_ms != 0U && elapsed >= interval_ms)
    {
        uint32_t steps = elapsed / interval_ms;
        last_step_tick += steps * interval_ms;
        current_target += step_deg * (float)steps;
        current_target = ClampFloat(current_target, GM6020_ANGLE_MIN, GM6020_ANGLE_MAX);
        LOGINFO("[motor_test] GM6020 target -> %d deg", (int)current_target);
    }

    MotorTest_GM6020_PositionLoop(current_target);
}

/**
 * @brief 简易速度环（PI）：读取 DM8009P 反馈转速(rad/s)，输出 MIT 力矩给定。
 *
 * @param target_speed_rad_s 期望速度，单位 rad/s，取值范围需落在 DM_V_MIN~DM_V_MAX。
 */
void MotorTest_DM8009P_SpeedLoop(float target_speed_rad_s)
{
    EnsureDM8009PMotorReady();
    if (!dm8009p_speed_mode_enabled)
    {
        DM8009P_ClearError(dm8009p_motor, DM8009P_MODE_SPEED);
        DM8009P_Enable(dm8009p_motor, DM8009P_MODE_SPEED);
        dm8009p_speed_mode_enabled = 1;
    }
    DM8009P_SendSpeedCommand(dm8009p_motor, target_speed_rad_s);
    const DM8009P_Feedback *fb = DM8009P_GetFeedback(dm8009p_motor);
    if (fb && fb->error_state)
    {
        LOGWARNING("[motor_test] DM8009P error state=0x%02x", fb->error_state);
    }
}


/**
 * @brief Execute a GM6020 speed test (ANGLE->SPEED control disabled, only
 *        speed loop) using CAN1.
 *
 * @param target_speed_deg_s Reference in deg/s, clamped internally.
 */
void MotorTest_GM6020_SpeedLoop(float target_speed_deg_s)
{
    EnsureGM6020MotorReady();
    target_speed_deg_s = ClampFloat(target_speed_deg_s, GM6020_SPEED_MIN, GM6020_SPEED_MAX);
    DJIMotorOuterLoop(gm6020_motor, SPEED_LOOP);
    DJIMotorEnable(gm6020_motor);
    DJIMotorSetRef(gm6020_motor, target_speed_deg_s);
    LOGINFO("[motor_test] GM6020 speed ref %d deg/s", (int)target_speed_deg_s);
}

/**
 * @brief Execute a GM6020 position test (ANGLE->SPEED cascade).
 *
 * @param target_angle_deg Reference angle in deg; helper clamps to a reasonable
 *                         range for safety.
 */
void MotorTest_GM6020_PositionLoop(float target_angle_deg)
{
    EnsureGM6020MotorReady();
    target_angle_deg = ClampFloat(target_angle_deg, GM6020_ANGLE_MIN, GM6020_ANGLE_MAX);
    DJIMotorOuterLoop(gm6020_motor, ANGLE_LOOP);
    DJIMotorEnable(gm6020_motor);
    DJIMotorSetRef(gm6020_motor, target_angle_deg);
    LOGINFO("[motor_test] GM6020 angle ref %d deg", (int)target_angle_deg);
}

/**
 * @brief Lazy-register the M3508 motor with CAN1 and copy the PID configuration
 *        from the production chassis module for realistic behaviour.
 */
static void EnsureM3508MotorReady(void)
{
    for (uint8_t i = 0; i < M3508_MOTOR_COUNT; ++i)
    {
        if (m3508_motors[i] != NULL)
        {
            continue;
        }

        Motor_Init_Config_s config = {
            .can_init_config =
                {
                    .can_handle = &hcan1,
                    .tx_id = M3508_CAN_IDS[i],
                },
            .controller_param_init_config =
                {
                    .angle_PID =
                        {
                            .Kp = 5.0f,
                            .Ki = 0.0f,
                            .Kd = 0.0f,
                            .MaxOut = M3508_SPEED_MAX,
                            .IntegralLimit = 500.0f,
                            .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                        },
                    .speed_PID =
                        {
                            .Kp = 10.0f,
                            .Ki = 0.0f,
                            .Kd = 0.0f,
                            .IntegralLimit = 3000.0f,

                            .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                            .MaxOut = 12000.0f,
                        },
                    .current_PID =
                        {
                            .Kp = 0.5f,
                            .Ki = 0.0f,
                            .Kd = 0.0f,
                            .IntegralLimit = 3000.0f,
                            .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                            .MaxOut = 15000.0f,
                        },
                },
            .controller_setting_init_config =
                {
                    .angle_feedback_source = MOTOR_FEED,
                    .speed_feedback_source = MOTOR_FEED,
                    .outer_loop_type = SPEED_LOOP,
                    .close_loop_type = ANGLE_LOOP | SPEED_LOOP | CURRENT_LOOP,
                    .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
                },
            .motor_type = M3508,
        };

        m3508_motors[i] = DJIMotorInit(&config);
        LOGINFO("[motor_test] M3508 index %u registered on CAN1 id %d", (unsigned)i, (int)M3508_CAN_IDS[i]);
    }
}

/**
 * @brief Lazy-register the GM6020 motor with CAN1. PID gains are borrowed from
 *        the gimbal module so the test mimics the real control stack.
 */
static void EnsureGM6020MotorReady(void)
{
    if (gm6020_motor != NULL)
    {
        return;
    }

    Motor_Init_Config_s config = {
        .can_init_config =
            {
                .can_handle = &hcan1,
                .tx_id = GM6020_CAN_ID,
            },
        .controller_param_init_config =
            {
                .angle_PID =
                    {
                        .Kp = 8.0f,
                        .Ki = 0.0f,
                        .Kd = 0.0f,
                        .DeadBand = 0.1f,
                        .IntegralLimit = 100.0f,
                        .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                        .MaxOut = 500.0f,
                    },
                .speed_PID =
                    {
                        .Kp = 10.0f,
                        .Ki = 40.0f,
                        .Kd = 0.0f,
                        .IntegralLimit = 3000.0f,
                        .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                        .MaxOut = 20000.0f,
                    },
            },
        .controller_setting_init_config =
            {
                .angle_feedback_source = MOTOR_FEED,
                .speed_feedback_source = MOTOR_FEED,
                .outer_loop_type = ANGLE_LOOP,
                .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
                .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
            },
        .motor_type = GM6020,
    };

    gm6020_motor = DJIMotorInit(&config);
    LOGINFO("[motor_test] GM6020 registered on CAN1 id %d", (int)GM6020_CAN_ID);
}

static void EnsureDM8009PMotorReady(void)
{
    if (dm8009p_motor != NULL)
        return;

    DM8009P_InitConfig config = {
        .can_handle = &hcan1,
        .motor_id = DM8009P_CAN_CMD_ID,
        .master_id = DM8009P_MASTER_ID,
        .auto_clear_error = true,
        .auto_enable_mit = false,
        .auto_zero_position = true,
    };

    dm8009p_motor = DM8009P_Init(&config);
    LOGINFO("[motor_test] DM8009P init done, cmd_id=%d master_id=%d", (int)DM8009P_CAN_CMD_ID, (int)DM8009P_MASTER_ID);
}

/**
 * @brief Simple helper to keep references within the specified limit.
 */
static float ClampFloat(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14)
    {
        HAL_IncTick();
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

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
