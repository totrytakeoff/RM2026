#include <stdio.h>
#include "../drivers/buzzer/buzzer.hpp"
#include "pin_map.h"
#include "stm32f4xx_hal.h"

// TIM4句柄声明
TIM_HandleTypeDef htim4;

// 更精确的延时函数（使用HAL库）
void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

// 系统时钟初始化函数（保持不变）
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

// TIM4初始化函数（保持不变）
void MX_TIM4_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 20999;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
        Error_Handler();
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

// 改进的小星星播放函数
void playTwinkleTwinkleLittleStar(Buzzer& buzzer) {
    printf("开始演奏：小星星\n");

    // 设置合适的音量
    buzzer.setVolume(70);  // 稍微提高音量以便更好地听到

    // 定义节奏常量（单位：毫秒）
    const uint32_t QUARTER_NOTE = 200;    // 四分音符（稍微缩短以获得更好的节奏感）
    const uint32_t HALF_NOTE = 500;       // 二分音符
    const uint32_t REST_BETWEEN_NOTES = 40; // 音符之间的小停顿
    const uint32_t REST_BETWEEN_PHRASES = 100; // 句子之间的停顿

    // 小星星简谱：1 1 5 5 | 6 6 5 - | 4 4 3 3 | 2 2 1 - |
    // 第一句：一闪一闪亮晶晶
    buzzer.playNote(Buzzer::NOTE_C4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_C4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_A4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_A4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES);  // 句子结束停顿

    // 第二句：满天都是小星星
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_C4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES);

    // 第三句：挂在天上放光明
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES);

    // 第四句：好像许多小眼睛
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES);

    // 第五句：一闪一闪亮晶晶（重复第一句）
    buzzer.playNote(Buzzer::NOTE_C4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_C4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_A4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_A4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_G4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES);

    // 第六句：满天都是小星星（重复第二句）
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_F4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_E4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_D4, QUARTER_NOTE, 70);
    HAL_Delay(REST_BETWEEN_NOTES);
    
    buzzer.playNote(Buzzer::NOTE_C4, HALF_NOTE, 70);
    HAL_Delay(REST_BETWEEN_PHRASES * 2);  // 整首歌结束后的较长停顿

    // 结束后停止蜂鸣器
    buzzer.stop();
    printf("小星星演奏完毕\n");
}

// 完整的main函数
int main(void) {
    // 初始化HAL库
    HAL_Init();

    // 配置系统时钟
    SystemClock_Config();

    // 初始化TIM4
    MX_TIM4_Init();

    // 启动定时器和PWM
    HAL_TIM_Base_Start(&htim4);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    // 创建Buzzer对象
    Buzzer buzzer;

    printf("=== 蜂鸣器音乐播放器 ===\n");
    printf("等待3秒后开始播放小星星...\n");
    HAL_Delay(3000);

    // 循环播放小星星
    while (1) {
        playTwinkleTwinkleLittleStar(buzzer);
        printf("等待5秒后重复播放...\n");
        HAL_Delay(5000);
    }
}