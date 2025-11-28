/**
 * @file    multi_button_interrupt_demo.cpp
 * @brief   多按钮中断模式演示程序
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

// 创建LED对象用于指示不同按钮的状态
LED button1_led(LED_R_GPIO_Port, LED_R_Pin, &htim5, TIM_CHANNEL_3);  // 按钮1对应红色LED
LED button2_led(LED_G_GPIO_Port, LED_G_Pin, &htim5, TIM_CHANNEL_2);  // 按钮2对应绿色LED
LED button3_led(LED_B_GPIO_Port, LED_B_Pin, &htim5, TIM_CHANNEL_1);  // 按钮3对应蓝色LED
LED status_led(GPIOA, GPIO_PIN_0);  // 系统状态指示LED

// 创建多个中断模式按钮
Button button1(KEY_GPIO_Port, KEY_Pin, ButtonMode::PULL_UP, ButtonWorkMode::INTERRUPT, 50);      // KEY按钮
Button button2(BUTTON_TRIG_GPIO_Port, BUTTON_TRIG_Pin, ButtonMode::PULL_UP, ButtonWorkMode::INTERRUPT, 50);  // TRIG按钮
Button button3(INT1_ACCEL_GPIO_Port, INT1_ACCEL_Pin, ButtonMode::PULL_UP, ButtonWorkMode::INTERRUPT, 50);  // 使用INT1_ACCEL作为第三个按钮

// 统计变量
static uint32_t button1_press_count = 0;
static uint32_t button2_press_count = 0;
static uint32_t button3_press_count = 0;
static uint32_t last_stats_time = 0;

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

    // 注册所有中断按钮到管理器
    bool success1 = button1.enableInterrupt();
    bool success2 = button2.enableInterrupt();
    bool success3 = button3.enableInterrupt();

    // 初始化LED状态
    status_led.off();
    button1_led.off();
    button2_led.off();
    button3_led.off();

    // 系统启动指示
    if (success1 && success2 && success3) {
        // 所有按钮注册成功，绿色闪烁3次
        status_led.toggle(3, 200);
    } else {
        // 部分失败，红色闪烁
        status_led.toggle(5, 100);
    }
}

/**
 * @brief 多按钮中断测试主函数
 */
void multi_button_interrupt_demo_main() {
    uint32_t current_time = HAL_GetTick();
    
    // 检查各按钮状态并更新对应LED
    // 按钮1 - 红色LED
    if (button1.isPressed()) {
        button1_led.on();
    } else {
        button1_led.off();
    }
    
    // 按钮2 - 绿色LED
    if (button2.isPressed()) {
        button2_led.on();
    } else {
        button2_led.off();
    }
    
    // 按钮3 - 蓝色LED
    if (button3.isPressed()) {
        button3_led.on();
    } else {
        button3_led.off();
    }
    
    // 检测按钮按下事件（边沿检测）
    static bool last_button1_state = false;
    static bool last_button2_state = false;
    static bool last_button3_state = false;
    
    bool current_button1_state = button1.read();
    bool current_button2_state = button2.read();
    bool current_button3_state = button3.read();
    
    // 按钮1按下检测
    if (current_button1_state && !last_button1_state) {
        button1_press_count++;
        button1_led.toggle(1, 100);  // 快速闪烁一次
    }
    
    // 按钮2按下检测
    if (current_button2_state && !last_button2_state) {
        button2_press_count++;
        button2_led.toggle(1, 100);  // 快速闪烁一次
    }
    
    // 按钮3按下检测
    if (current_button3_state && !last_button3_state) {
        button3_press_count++;
        button3_led.toggle(1, 100);  // 快速闪烁一次
    }
    
    // 更新上次状态
    last_button1_state = current_button1_state;
    last_button2_state = current_button2_state;
    last_button3_state = current_button3_state;
    
    // 每10秒显示一次统计信息
    if (current_time - last_stats_time >= 10000) {
        last_stats_time = current_time;
        
        // 通过LED闪烁次数显示各按钮的按下次数
        // 按钮1计数：红色LED闪烁
        for (int i = 0; i < button1_press_count && i < 10; i++) {
            button1_led.on();
            HAL_Delay(150);
            button1_led.off();
            HAL_Delay(150);
        }
        
        HAL_Delay(500);
        
        // 按钮2计数：绿色LED闪烁
        for (int i = 0; i < button2_press_count && i < 10; i++) {
            button2_led.on();
            HAL_Delay(150);
            button2_led.off();
            HAL_Delay(150);
        }
        
        HAL_Delay(500);
        
        // 按钮3计数：蓝色LED闪烁
        for (int i = 0; i < button3_press_count && i < 10; i++) {
            button3_led.on();
            HAL_Delay(150);
            button3_led.off();
            HAL_Delay(150);
        }
        
        HAL_Delay(500);
        
        // 显示注册的按钮数量：白色闪烁（所有LED同时）
        uint8_t button_count = ButtonManager::getInstance().getButtonCount();
        for (int i = 0; i < button_count; i++) {
            button1_led.on();
            button2_led.on();
            button3_led.on();
            HAL_Delay(200);
            button1_led.off();
            button2_led.off();
            button3_led.off();
            HAL_Delay(200);
        }
        
        HAL_Delay(500);
        
        // 重置计数器
        button1_press_count = 0;
        button2_press_count = 0;
        button3_press_count = 0;
    }
    
    // 系统运行指示：状态LED每秒闪烁一次
    static uint32_t last_blink = 0;
    if (current_time - last_blink >= 1000) {
        last_blink = current_time;
        status_led.toggle();
    }
}

/**
 * @brief 按钮管理器状态检查
 */
void check_button_manager_status() {
    uint8_t button_count = ButtonManager::getInstance().getButtonCount();
    
    // 如果注册的按钮数量不等于3，说明有问题
    if (button_count != 3) {
        // 错误指示：红色LED快速闪烁
        status_led.toggle(button_count, 100);
        HAL_Delay(1000);
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    // 硬件初始化
    hardware_init();
    
    // 检查按钮管理器状态
    check_button_manager_status();
    
    // 运行多按钮中断演示
    while (1) {
        multi_button_interrupt_demo_main();
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
    // 错误处理：所有LED快速闪烁
    while (1) {
        button1_led.toggle(2, 50);
        button2_led.toggle(2, 50);
        button3_led.toggle(2, 50);
        status_led.toggle(2, 50);
        HAL_Delay(500);
    }
}
