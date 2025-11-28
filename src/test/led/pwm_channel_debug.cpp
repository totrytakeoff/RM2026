/**
 * @file    pwm_channel_debug.cpp
 * @brief   PWM通道调试程序
 * @author  myself 
 * @date    2025/11/27
 * 
 * 功能：逐个调试PWM通道，找出蓝色LED常亮问题
 */

#include "../drivers/button.hpp"
#include "hal/gpio.h"
#include "hal/tim.h"
#include "main.h"

// 外部定时器句柄声明
extern TIM_HandleTypeDef htim5;

// 外部函数声明
extern "C" void MX_GPIO_Init(void);
extern "C" void MX_TIM5_Init(void);

// 创建按钮对象
Button key_button(KEY_GPIO_Port, KEY_Pin, ButtonMode::PULL_UP, 50);  // KEY按钮

// 测试模式枚举
enum TestMode {
    TEST_CH1_0 = 0,        // TIM5_CH1 (蓝色) = 0
    TEST_CH1_50,           // TIM5_CH1 (蓝色) = 50%
    TEST_CH1_100,          // TIM5_CH1 (蓝色) = 100%
    TEST_CH2_0,            // TIM5_CH2 (绿色) = 0
    TEST_CH2_50,           // TIM5_CH2 (绿色) = 50%
    TEST_CH2_100,          // TIM5_CH2 (绿色) = 100%
    TEST_CH3_0,            // TIM5_CH3 (红色) = 0
    TEST_CH3_50,           // TIM5_CH3 (红色) = 50%
    TEST_CH3_100,          // TIM5_CH3 (红色) = 100%
    TEST_ALL_0,            // 所有通道 = 0
    TEST_ALL_50,           // 所有通道 = 50%
    TEST_ALL_100,          // 所有通道 = 100%
    TEST_REINIT,           // 重新初始化PWM
    TEST_COUNT
};

// 当前测试模式
static TestMode current_mode = TEST_CH1_0;

// 函数声明
extern "C" void SystemClock_Config(void);
extern "C" void Error_Handler(void);

/**
 * @brief 重新初始化PWM通道，清除初始值
 */
void reinit_pwm_channels() {
    // 停止所有PWM通道
    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3);
    
    // 重新配置PWM通道，初始值设为0
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  // 关键：初始值设为0！
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2);
    HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_3);
    
    // 重新启动PWM通道
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
}

/**
 * @brief 测试PWM通道
 */
void test_pwm_channels() {
    switch (current_mode) {
        case TEST_CH1_0:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH1_50:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 32767);  // 蓝色 = 50%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH1_100:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 65535);  // 蓝色 = 100%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH2_0:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH2_50:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 32767);  // 绿色 = 50%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH2_100:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 65535);  // 绿色 = 100%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH3_0:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_CH3_50:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 32767);  // 红色 = 50%
            break;
        case TEST_CH3_100:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 65535);  // 红色 = 100%
            break;
        case TEST_ALL_0:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);      // 蓝色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);      // 绿色 = 0
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);      // 红色 = 0
            break;
        case TEST_ALL_50:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 32767);  // 蓝色 = 50%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 32767);  // 绿色 = 50%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 32767);  // 红色 = 50%
            break;
        case TEST_ALL_100:
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 65535);  // 蓝色 = 100%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 65535);  // 绿色 = 100%
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 65535);  // 红色 = 100%
            break;
        case TEST_REINIT:
            reinit_pwm_channels();
            break;
        default:
            break;
    }
}

/**
 * @brief 获取测试模式名称
 */
const char* get_mode_name(TestMode mode) {
    switch (mode) {
        case TEST_CH1_0: return "CH1_0 (Blue=0%)";
        case TEST_CH1_50: return "CH1_50 (Blue=50%)";
        case TEST_CH1_100: return "CH1_100 (Blue=100%)";
        case TEST_CH2_0: return "CH2_0 (Green=0%)";
        case TEST_CH2_50: return "CH2_50 (Green=50%)";
        case TEST_CH2_100: return "CH2_100 (Green=100%)";
        case TEST_CH3_0: return "CH3_0 (Red=0%)";
        case TEST_CH3_50: return "CH3_50 (Red=50%)";
        case TEST_CH3_100: return "CH3_100 (Red=100%)";
        case TEST_ALL_0: return "ALL_0";
        case TEST_ALL_50: return "ALL_50";
        case TEST_ALL_100: return "ALL_100";
        case TEST_REINIT: return "REINIT_PWM";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 硬件初始化
 */
void hardware_init() {
    // 初始化系统时钟
    HAL_Init();
    SystemClock_Config();

    // 初始化GPIO
    MX_GPIO_Init();

    // 初始化定时器
    MX_TIM5_Init();

    // 启动PWM
    HAL_TIM_Base_Start(&htim5);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    // 初始状态：重新初始化PWM通道，清除初始值
    reinit_pwm_channels();
}

/**
 * @brief 主函数
 */
int main(void) {
    // 硬件初始化
    hardware_init();

    // 主循环
    while (1) {
        // 检测按钮按下
        if (key_button.isPressed()) {
            // 切换到下一个测试模式
            current_mode = static_cast<TestMode>((current_mode + 1) % TEST_COUNT);
            test_pwm_channels();
        }

        // 短暂延时
        HAL_Delay(10);
    }

    return 0;
}

// 系统时钟配置
extern "C" void SystemClock_Config(void) {
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
    RCC_OscInitStruct.PLL.PLLQ = 4;
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

extern "C" void Error_Handler(void) {
    // 错误处理：关闭所有LED
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
    
    while (1) {
        // 错误指示：快速闪烁蓝色
        __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 65535);
        HAL_Delay(100);
        __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);
        HAL_Delay(100);
    }
}
