/**
 * @file    simple_led_demo.cpp
 * @brief   简单LED颜色循环切换演示
 * @author  myself 
 * @date    2025/11/27
 * 
 * 功能：按KEY按钮切换红橙黄绿青蓝紫颜色
 */

#include "../../drivers/btn/button.hpp"
#include "../../drivers/led/led.hpp"
extern "C" {
#include "hal/gpio.h"
#include "hal/tim.h"
#include "main.h"

}

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

// 颜色枚举
enum ColorMode {
    COLOR_RED = 0,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_COUNT  // 颜色总数
};

// 当前颜色模式
static ColorMode current_color = COLOR_RED;

// 函数声明
extern "C" void SystemClock_Config(void);
extern "C" void Error_Handler(void);

/**
 * @brief 设置RGB颜色
 */
void set_color(ColorMode color) {
    switch (color) {
        case COLOR_RED:
            rgb_led.setColorRGB(255, 0, 0);      // 红色
            break;
        case COLOR_ORANGE:
            rgb_led.setColorRGB(255, 165, 0);    // 橙色
            break;
        case COLOR_YELLOW:
            rgb_led.setColorRGB(255, 255, 0);    // 黄色
            break;
        case COLOR_GREEN:
            rgb_led.setColorRGB(0, 255, 0);      // 绿色
            break;
        case COLOR_CYAN:
            rgb_led.setColorRGB(0, 255, 255);    // 青色
            break;
        case COLOR_BLUE:
            rgb_led.setColorRGB(0, 0, 255);      // 蓝色
            break;
        case COLOR_PURPLE:
            rgb_led.setColorRGB(128, 0, 128);    // 紫色
            break;
        default:
            rgb_led.off();
            break;
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
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    // 初始化LED为红色
    set_color(COLOR_RED);
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
            // 切换到下一个颜色
            current_color = static_cast<ColorMode>((current_color + 1) % COLOR_COUNT);
            set_color(current_color);
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
    rgb_led.off();
    
    while (1) {
        // 错误指示：快速闪烁红色
        rgb_led.setColorRGB(255, 0, 0);
        HAL_Delay(100);
        rgb_led.off();
        HAL_Delay(100);
    }
}
