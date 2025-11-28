#include <stdio.h>
#include "../drivers/buzzer/buzzer.hpp"
#include "pin_map.h"
#include "stm32f4xx_hal.h"

// TIM4句柄声明
TIM_HandleTypeDef htim4;

// 简单的延时函数
void delay_ms(uint32_t ms) {
    uint32_t i;
    for (i = 0; i < ms * 1000; i++) {
        __NOP();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

// 系统时钟初始化函数
void SystemClock_Config(void) {
    // 系统时钟配置代码
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // 配置外部高速时钟HSE
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // 配置系统时钟和总线时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  // APB1时钟为42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

// TIM4初始化函数
void MX_TIM4_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    // 定时器基础配置 - 与官方demo保持一致
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;  // 初始预分频器为0，将在运行时动态调整
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 20999;  // 官方demo中固定的ARR值
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;  // 与官方demo一致
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    // 时钟源配置
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    // 初始化PWM模式
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    // 主模式配置
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    // PWM通道配置
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  // 初始为0，不发声
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
        Error_Handler();
    }

    // 初始化GPIO - 与官方demo保持一致
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;  // TIM4_CH3在GPIOD_PIN_14上
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

// 错误处理函数

int main(void) {
    // 初始化HAL库
    HAL_Init();

    // 配置系统时钟
    SystemClock_Config();

    // 初始化TIM4
    MX_TIM4_Init();

    // 启动定时器和PWM - 与官方demo保持一致
    HAL_TIM_Base_Start(&htim4);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    // 创建Buzzer对象
    Buzzer buzzer;

    // 测试1: 播放C大调音阶
    printf("测试1: 播放C大调音阶\n");
    buzzer.playNote(Buzzer::NOTE_C4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_D4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_E4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_F4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_G4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_A4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_B4, 1000, 80);
    delay_ms(400);

    buzzer.playNote(Buzzer::NOTE_C5, 1000, 80);
    delay_ms(1000);

    // // 测试2: 测试不同音量
    // printf("测试2: 测试不同音量\n");
    // buzzer.setVolume(20);  // 低音量
    // buzzer.playTone(4000, 20);
    // delay_ms(1000);

    // buzzer.setVolume(50);  // 中等音量
    // buzzer.playTone(4000, 50);
    // delay_ms(1000);

    // buzzer.setVolume(80);  // 高音量
    // buzzer.playTone(4000, 80);
    // delay_ms(1000);

    // 停止蜂鸣
    buzzer.stop();

    // 测试3: 播放提示音
    printf("测试3: 播放提示音\n");
    delay_ms(500);
    buzzer.playShortBeep();
    delay_ms(500);

    buzzer.playLongBeep();
    delay_ms(1000);

    buzzer.playErrorBeep();
    delay_ms(1000);

    buzzer.playSuccessBeep();
    delay_ms(1000);

    // 测试4: 播放简单旋律
    printf("测试4: 播放简单旋律\n");
    delay_ms(500);

    // 小星星旋律片段
    buzzer.playNote(Buzzer::NOTE_C4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_C4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_G4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_G4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_A4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_A4, 200, 70);
    delay_ms(100);
    buzzer.playNote(Buzzer::NOTE_G4, 400, 70);
    delay_ms(200);

    // 循环播放测试 - 测试所有功能
    while (1) {
        // 短暂休息
        delay_ms(3000);

        printf("循环测试: 测试所有功能\n");

        // // 测试beep函数
        // printf("测试beep函数\n");
        // buzzer.beep(70);
        // delay_ms(800);
        // buzzer.stop();
        // delay_ms(500);

        // 测试playTone函数
        // printf("测试playTone函数 - 不同频率\n");
        // buzzer.playTone(1000, 60);
        // delay_ms(300);
        // buzzer.playTone(2000, 60);
        // delay_ms(300);
        // buzzer.playTone(3000, 60);
        // delay_ms(300);
        // buzzer.stop();
        // delay_ms(500);

        // 测试音量控制
        // printf("测试音量控制\n");
        // buzzer.playTone(2000, 30);  // 低音量
        // delay_ms(500);
        // buzzer.setVolume(80);       // 立即切换到高音量
        // delay_ms(500);
        // buzzer.stop();
        // delay_ms(500);

        // 测试提示音函数
        // printf("测试提示音函数\n");
        // buzzer.playShortBeep();
        // delay_ms(500);
        // buzzer.playLongBeep();
        // delay_ms(1000);
        // buzzer.playErrorBeep();
        // delay_ms(1000);
        // buzzer.playSuccessBeep();
        // delay_ms(1000);

        buzzer.playNote(Buzzer::NOTE_C4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_D4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_E4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_F4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_G4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_A4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_B4, 1000, 80);
        delay_ms(400);

        buzzer.playNote(Buzzer::NOTE_C5, 1000, 80);
        delay_ms(1000);
    }
}