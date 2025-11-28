/**
 * @file    led_debug_analysis.cpp
 * @brief   LED调试分析程序
 * @author  myself 
 * @date    2025/11/27
 * 
 * 功能：详细分析LED颜色混合问题，逐个通道测试
 */

#include "../drivers/button.hpp"
#include "../drivers/led.hpp"
#include "hal/gpio.h"
#include "hal/tim.h"
#include "main.h"

// 外部定时器句柄声明
extern TIM_HandleTypeDef htim5;

// 外部函数声明
extern "C" void MX_GPIO_Init(void);
extern "C" void MX_TIM5_Init(void);

// PWM模式的LED（使用TIM5的不同通道）
LED red_led(LED_R_GPIO_Port, LED_R_Pin, &htim5, TIM_CHANNEL_3);    // PH12, TIM5_CH3
LED green_led(LED_G_GPIO_Port, LED_G_Pin, &htim5, TIM_CHANNEL_2);  // PH11, TIM5_CH2
LED blue_led(LED_B_GPIO_Port, LED_B_Pin, &htim5, TIM_CHANNEL_1);   // PH10, TIM5_CH1

// 创建RGB LED对象
RGBLED rgb_led(red_led, green_led, blue_led);

// 创建按钮对象
Button key_button(KEY_GPIO_Port, KEY_Pin, ButtonMode::PULL_UP, 50);  // KEY按钮

// 测试模式枚举
enum TestMode {
    DEBUG_RED_PWM_0 = 0,      // 红色PWM=0
    DEBUG_RED_PWM_50,          // 红色PWM=50%
    DEBUG_RED_PWM_100,         // 红色PWM=100%
    DEBUG_GREEN_PWM_0,         // 绿色PWM=0
    DEBUG_GREEN_PWM_50,        // 绿色PWM=50%
    DEBUG_GREEN_PWM_100,       // 绿色PWM=100%
    DEBUG_BLUE_PWM_0,          // 蓝色PWM=0
    DEBUG_BLUE_PWM_50,         // 蓝色PWM=50%
    DEBUG_BLUE_PWM_100,        // 蓝色PWM=100%
    DEBUG_RED_ONLY,            // 仅红色
    DEBUG_GREEN_ONLY,          // 仅绿色
    DEBUG_BLUE_ONLY,           // 仅蓝色
    DEBUG_RED_GREEN,           // 红色+绿色
    DEBUG_RED_BLUE,            // 红色+蓝色
    DEBUG_GREEN_BLUE,          // 绿色+蓝色
    DEBUG_ALL_ON,              // 全部开启
    DEBUG_OFF,                 // 全部关闭
    DEBUG_COUNT
};

// 当前测试模式
static TestMode current_mode = DEBUG_RED_PWM_0;

// 函数声明
extern "C" void SystemClock_Config(void);
extern "C" void Error_Handler(void);

/**
 * @brief 直接设置PWM值进行调试
 */
void debug_set_pwm(uint16_t red_pwm, uint16_t green_pwm, uint16_t blue_pwm) {
    // 直接使用HAL函数设置PWM值
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_3, red_pwm);   // 红色
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_2, green_pwm); // 绿色
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_1, blue_pwm);  // 蓝色
}

/**
 * @brief 测试PWM值设置
 */
void test_pwm_values() {
    switch (current_mode) {
        case DEBUG_RED_PWM_0:
            debug_set_pwm(0, 0, 0);
            break;
        case DEBUG_RED_PWM_50:
            debug_set_pwm(32767, 0, 0);  // 50% = 65535/2
            break;
        case DEBUG_RED_PWM_100:
            debug_set_pwm(65535, 0, 0);  // 100%
            break;
        case DEBUG_GREEN_PWM_0:
            debug_set_pwm(0, 0, 0);
            break;
        case DEBUG_GREEN_PWM_50:
            debug_set_pwm(0, 32767, 0);  // 50%
            break;
        case DEBUG_GREEN_PWM_100:
            debug_set_pwm(0, 65535, 0);  // 100%
            break;
        case DEBUG_BLUE_PWM_0:
            debug_set_pwm(0, 0, 0);
            break;
        case DEBUG_BLUE_PWM_50:
            debug_set_pwm(0, 0, 32767);  // 50%
            break;
        case DEBUG_BLUE_PWM_100:
            debug_set_pwm(0, 0, 65535);  // 100%
            break;
        case DEBUG_RED_ONLY:
            debug_set_pwm(65535, 0, 0);
            break;
        case DEBUG_GREEN_ONLY:
            debug_set_pwm(0, 65535, 0);
            break;
        case DEBUG_BLUE_ONLY:
            debug_set_pwm(0, 0, 65535);
            break;
        case DEBUG_RED_GREEN:
            debug_set_pwm(65535, 65535, 0);
            break;
        case DEBUG_RED_BLUE:
            debug_set_pwm(65535, 0, 65535);
            break;
        case DEBUG_GREEN_BLUE:
            debug_set_pwm(0, 65535, 65535);
            break;
        case DEBUG_ALL_ON:
            debug_set_pwm(65535, 65535, 65535);
            break;
        case DEBUG_OFF:
            debug_set_pwm(0, 0, 0);
            break;
        default:
            break;
    }
}

/**
 * @brief 使用LED类测试
 */
void test_led_class() {
    switch (current_mode) {
        case DEBUG_RED_ONLY:
            red_led.on();
            green_led.off();
            blue_led.off();
            break;
        case DEBUG_GREEN_ONLY:
            red_led.off();
            green_led.on();
            blue_led.off();
            break;
        case DEBUG_BLUE_ONLY:
            red_led.off();
            green_led.off();
            blue_led.on();
            break;
        case DEBUG_RED_GREEN:
            red_led.on();
            green_led.on();
            blue_led.off();
            break;
        case DEBUG_RED_BLUE:
            red_led.on();
            green_led.off();
            blue_led.on();
            break;
        case DEBUG_GREEN_BLUE:
            red_led.off();
            green_led.on();
            blue_led.on();
            break;
        case DEBUG_ALL_ON:
            red_led.on();
            green_led.on();
            blue_led.on();
            break;
        case DEBUG_OFF:
            red_led.off();
            green_led.off();
            blue_led.off();
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
        case DEBUG_RED_PWM_0: return "RED_PWM_0";
        case DEBUG_RED_PWM_50: return "RED_PWM_50";
        case DEBUG_RED_PWM_100: return "RED_PWM_100";
        case DEBUG_GREEN_PWM_0: return "GREEN_PWM_0";
        case DEBUG_GREEN_PWM_50: return "GREEN_PWM_50";
        case DEBUG_GREEN_PWM_100: return "GREEN_PWM_100";
        case DEBUG_BLUE_PWM_0: return "BLUE_PWM_0";
        case DEBUG_BLUE_PWM_50: return "BLUE_PWM_50";
        case DEBUG_BLUE_PWM_100: return "BLUE_PWM_100";
        case DEBUG_RED_ONLY: return "RED_ONLY";
        case DEBUG_GREEN_ONLY: return "GREEN_ONLY";
        case DEBUG_BLUE_ONLY: return "BLUE_ONLY";
        case DEBUG_RED_GREEN: return "RED_GREEN";
        case DEBUG_RED_BLUE: return "RED_BLUE";
        case DEBUG_GREEN_BLUE: return "GREEN_BLUE";
        case DEBUG_ALL_ON: return "ALL_ON";
        case DEBUG_OFF: return "OFF";
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

    // 启动PWM - 按照官方demo的方式
    HAL_TIM_Base_Start(&htim5);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    // 初始状态：关闭所有LED
    test_pwm_values();
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
            current_mode = static_cast<TestMode>((current_mode + 1) % DEBUG_COUNT);
            
            // 根据模式选择测试方法
            if (current_mode <= DEBUG_BLUE_PWM_100) {
                test_pwm_values();  // 直接PWM测试
            } else {
                test_led_class();   // LED类测试
            }
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
    debug_set_pwm(0, 0, 0);
    
    while (1) {
        // 错误指示：快速闪烁红色
        debug_set_pwm(65535, 0, 0);
        HAL_Delay(100);
        debug_set_pwm(0, 0, 0);
        HAL_Delay(100);
    }
}
