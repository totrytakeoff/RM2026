/**
 * @file    button_demo.cpp
 * @brief   Button类演示（支持轮询和中断模式）
 * @author  myself 
 * @date    2025/11/27
 */

#include "../drivers/button.hpp"
#include "../drivers/led.hpp"
#include "../drivers/button_manager.hpp"
#include "hal/gpio.h"
#include "hal/tim.h"
#include "main.h"

// 外部定时器句柄声明
extern TIM_HandleTypeDef htim5;

// 外部函数声明
extern "C" void MX_GPIO_Init(void);
extern "C" void MX_TIM5_Init(void);

// 创建LED对象用于指示
LED status_led(GPIOA, GPIO_PIN_0);  // 状态指示LED
LED interrupt_led(LED_R_GPIO_Port, LED_R_Pin, &htim5, TIM_CHANNEL_3);  // 中断模式指示LED
LED polling_led(LED_G_GPIO_Port, LED_G_Pin, &htim5, TIM_CHANNEL_2);   // 轮询模式指示LED

// 创建按钮对象
// 轮询模式按钮（KEY按钮）
Button polling_button(KEY_GPIO_Port, KEY_Pin, ButtonMode::PULL_UP, ButtonWorkMode::POLLING, 50);

// 中断模式按钮（TRIG按钮）
Button interrupt_button(BUTTON_TRIG_GPIO_Port, BUTTON_TRIG_Pin, ButtonMode::PULL_UP, ButtonWorkMode::INTERRUPT, 50);

// 状态变量
static uint32_t last_poll_time = 0;
static uint8_t interrupt_count = 0;
static uint8_t polling_count = 0;

// 函数声明
extern "C" void SystemClock_Config(void);

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

    // 注册中断按钮到中断管理器
    interrupt_button.enableInterrupt();

    // 初始化LED状态
    status_led.off();
    interrupt_led.off();
    polling_led.off();
}

/**
 * @brief 按钮测试主函数
 */
void button_demo_main() {
    uint32_t current_time = HAL_GetTick();
    
    // 轮询模式检测（每10ms检测一次）
    if (current_time - last_poll_time >= 10) {
        last_poll_time = current_time;
        
        // 检测轮询按钮
        if (polling_button.isPressed()) {
            polling_count++;
            polling_led.toggle(1, 100);  // 闪烁一次表示按下
            
            // 长按检测
            if (polling_button.isLongPressed(2000)) {
                // 长按2秒，快速闪烁
                polling_led.toggle(5, 50);
            }
        }
        
        if (polling_button.isReleased()) {
            status_led.toggle(1, 50);  // 释放时状态灯闪烁
        }
    }
    
    // 中断模式的状态检查（在中断中已经处理了状态更新）
    // 这里只需要检查事件标志
    static bool last_interrupt_state = false;
    bool current_interrupt_state = interrupt_button.read();
    
    // 检测中断按钮的边沿（基于中断更新后的状态）
    if (current_interrupt_state && !last_interrupt_state) {
        interrupt_count++;
        interrupt_led.toggle(1, 100);  // 闪烁一次表示按下
    }
    
    if (!current_interrupt_state && last_interrupt_state) {
        status_led.toggle(2, 50);  // 释放时状态灯闪烁两次
    }
    
    last_interrupt_state = current_interrupt_state;
    
    // 每5秒显示一次统计信息
    static uint32_t last_stats_time = 0;
    if (current_time - last_stats_time >= 5000) {
        last_stats_time = current_time;
        
        // 通过LED闪烁次数显示统计
        // 中断按钮计数：红色LED闪烁
        for (int i = 0; i < interrupt_count && i < 10; i++) {
            interrupt_led.on();
            HAL_Delay(100);
            interrupt_led.off();
            HAL_Delay(100);
        }
        
        HAL_Delay(500);
        
        // 轮询按钮计数：绿色LED闪烁
        for (int i = 0; i < polling_count && i < 10; i++) {
            polling_led.on();
            HAL_Delay(100);
            polling_led.off();
            HAL_Delay(100);
        }
        
        HAL_Delay(500);
        
        // 重置计数器
        interrupt_count = 0;
        polling_count = 0;
    }
}

/**
 * @brief 简单的按钮功能演示
 */
void simple_button_demo() {
    while (1) {
        // 轮询按钮：控制绿色LED
        if (polling_button.isPressed()) {
            polling_led.on();
        } else {
            polling_led.off();
        }
        
        // 中断按钮：控制红色LED（状态由中断更新）
        if (interrupt_button.read()) {
            interrupt_led.on();
        } else {
            interrupt_led.off();
        }
        
        // 状态LED：系统运行指示
        static uint32_t last_blink = 0;
        if (HAL_GetTick() - last_blink >= 1000) {
            last_blink = HAL_GetTick();
            status_led.toggle();
        }
        
        HAL_Delay(10);
    }
}

/**
 * @brief 按钮功能对比测试
 */
void button_comparison_demo() {
    while (1) {
        uint32_t start_time = HAL_GetTick();
        
        // 测试轮询模式的响应时间
        while (HAL_GetTick() - start_time < 3000) {  // 测试3秒
            if (polling_button.isPressed()) {
                polling_led.on();
                break;
            }
            HAL_Delay(1);  // 1ms轮询间隔
        }
        
        HAL_Delay(1000);
        polling_led.off();
        
        // 测试中断模式的响应时间
        start_time = HAL_GetTick();
        while (HAL_GetTick() - start_time < 3000) {  // 测试3秒
            if (interrupt_button.read()) {  // 直接读取状态（由中断更新）
                interrupt_led.on();
                break;
            }
            HAL_Delay(10);  // 10ms检查间隔
        }
        
        HAL_Delay(1000);
        interrupt_led.off();
        
        // 状态指示
        status_led.toggle(3, 200);  // 完成一个周期，闪烁3次
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    // 硬件初始化
    hardware_init();
    
    // 运行按钮演示
    while (1) {
        button_demo_main();
        HAL_Delay(10);
    }
    
    return 0;
}

// 系统时钟配置（简化版）
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
    // 错误处理：快速闪烁状态LED
    while (1) {
        status_led.toggle(5, 50);
        HAL_Delay(500);
    }
}
