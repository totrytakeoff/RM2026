/**
 * @file main.c
 * @brief Infantry最小框架主入口
 * @note 单一主循环, 支持ET08遥控和VT图传键鼠
 */

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

#include "minimal_config.h"
#include "minimal_types.h"
#include "minimal_chassis.h"
#include "minimal_gimbal.h"
#include "minimal_shoot.h"
#include "minimal_input.h"
#include "minimal_referee.h"
#include "minimal_debug.h"
#include "bsp_dwt.h"
#include "bsp_log.h"
#include "daemon.h"
#include "dji_motor.h"
#include "ins_task.h"

/*============================================================================
 * 前置声明
 *============================================================================*/
void SystemClock_Config(void);
void Error_Handler(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

/*============================================================================
 * 全局上下文
 *============================================================================*/
Robot_Context_t g_robot = {0};

/*============================================================================
 * 私有变量
 *============================================================================*/
static uint32_t last_tick = 0;
static uint8_t safe_stop_latched = 0U;

static void ForceSafeStop(void)
{
    Chassis_Stop();
    Gimbal_Stop();
    Shoot_Stop();
}

/*============================================================================
 * 主函数
 *============================================================================*/
int main(void)
{
    // HAL初始化 (由CubeMX生成)
    HAL_Init();
    SystemClock_Config();
     MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    MX_SPI1_Init();
    MX_TIM4_Init();
    MX_TIM5_Init();
    MX_USART3_UART_Init();
    MX_RNG_Init();
    MX_RTC_Init();
    MX_TIM1_Init();
    MX_TIM10_Init();
    MX_USART1_UART_Init();
    MX_USART6_UART_Init();
    MX_TIM8_Init();
    MX_I2C2_Init();
    MX_I2C3_Init();
    MX_SPI2_Init();
    MX_CRC_Init();
    MX_DAC_Init();


    // BSP初始化
    DWT_Init(168);
#if MINIMAL_DEBUG_ENABLE && ((MINIMAL_DEBUG_MODE & MINIMAL_DEBUG_MODE_TEXT) != 0)
    BSPLogInit();
#endif
    MinimalDebug_Init();
    
    // 等待电机稳定上电
    HAL_Delay(MOTOR_STABILIZE_TIME_MS);
    
    // 模块初始化
    Input_Init();
    MinimalReferee_Init();
    Chassis_Init();
    Gimbal_Init();
    Shoot_Init();
    
    last_tick = DWT_GetTimeline_ms();
    g_robot.initialized = 1;
    MDBG_SYS("minimal initialized");
    
    // 主循环
    while (1) {
        INS_Task();
        DaemonTask();
        DJIMotorControl();

        uint32_t now = DWT_GetTimeline_ms();
        uint32_t dt = now - last_tick;
        
        // 周期控制
        if (dt >= MAIN_LOOP_PERIOD_MS) {
            last_tick = now;
            
            // 1. 获取输入数据 (ET08或VT键鼠)
            Input_Data_t input;
            Input_GetData(&input);
            MinimalReferee_Update();
            g_robot.referee = *MinimalReferee_GetData();
            
            // 2. 检查急停或离线
            if (input.emergency_stop || !input.online) {
                if (!safe_stop_latched) {
                    MDBG_SYS("ForceSafeStop trigger estop=%u online=%u", (unsigned)input.emergency_stop, (unsigned)input.online);
                    safe_stop_latched = 1U;
                }
                ForceSafeStop();
                g_robot.input = input;
                MinimalDebug_UpdatePeriodic(now);
                continue;
            }
            if (safe_stop_latched) {
                MDBG_SYS("ForceSafeStop release");
                safe_stop_latched = 0U;
            }
            
            // 3. 模块更新
            Gimbal_Update(&input, Chassis_GetWz());
            Chassis_Update(&input);
            Shoot_Update(&input);
            
            // 4. 保存状态到全局上下文
            g_robot.input = input;
        }
        MinimalDebug_UpdatePeriodic(now);

        /* 与omni_demo对齐: 限制电机控制发送频率，避免CAN邮箱打满 */
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


/*============================================================================
 * 错误处理
 *============================================================================*/
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        // 死循环
    }
}
