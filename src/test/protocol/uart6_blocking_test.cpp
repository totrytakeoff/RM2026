/**
 * @file uart6_blocking_test.cpp
 * @brief UART6 阻塞模式测试（不使用DMA，排查波特率问题）
 * @date 2025-12-03
 */

#include "main.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"

// 直接使用HAL库，不用SerialPort类
UART_HandleTypeDef huart6;

/**
 * @brief 系统时钟配置
 */
extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    // HSE = 12MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;   // 12MHz / 6 = 2MHz
    RCC_OscInitStruct.PLL.PLLN = 168; // 2MHz * 168 = 336MHz
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 336MHz / 2 = 168MHz
    RCC_OscInitStruct.PLL.PLLQ = 4;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // SYSCLK = 168MHz
    // AHB = 168MHz
    // APB1 = 42MHz
    // APB2 = 84MHz (UART6在这里)
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 168MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;    // 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;    // 84MHz
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief UART6初始化（阻塞模式）
 */
void MX_USART6_UART_Init(void) {
    // 1. 使能时钟
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();
    
    // 2. 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // TX: PG14
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    
    // RX: PG9
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    
    // 3. 配置UART
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief 发送字符串
 */
void UART6_SendString(const char* str) {
    HAL_UART_Transmit(&huart6, (uint8_t*)str, strlen(str), 1000);
}

/**
 * @brief 主函数
 */
int main(void) {
    // 1. HAL初始化
    HAL_Init();
    
    // 2. 配置系统时钟
    SystemClock_Config();
    
    // 3. 初始化GPIO（LED）
    MX_GPIO_Init();
    
    // 4. 初始化UART6
    MX_USART6_UART_Init();
    
    // 5. 打印时钟信息（用于调试）
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    
    // 6. 发送测试消息
    UART6_SendString("\r\n\r\n");
    UART6_SendString("========================================\r\n");
    UART6_SendString("  UART6 Blocking Mode Test\r\n");
    UART6_SendString("========================================\r\n");
    
    // 发送时钟信息（需要sprintf）
    char buffer[128];
    sprintf(buffer, "SYSCLK: %lu Hz\r\n", sysclk);
    UART6_SendString(buffer);
    sprintf(buffer, "HCLK:   %lu Hz\r\n", hclk);
    UART6_SendString(buffer);
    sprintf(buffer, "PCLK1:  %lu Hz\r\n", pclk1);
    UART6_SendString(buffer);
    sprintf(buffer, "PCLK2:  %lu Hz (UART6)\r\n", pclk2);
    UART6_SendString(buffer);
    
    UART6_SendString("========================================\r\n");
    UART6_SendString("Baudrate: 115200 bps\r\n");
    UART6_SendString("Mode: Blocking (Polling)\r\n");
    UART6_SendString("Hardware: UART6 (PG14/PG9)\r\n");
    UART6_SendString("========================================\r\n\r\n");
    
    // 7. 主循环
    uint32_t counter = 0;
    uint8_t rxByte;
    
    while (1) {
        // 发送心跳
        counter++;
        sprintf(buffer, "[%lu] Heartbeat %lu\r\n", HAL_GetTick(), counter);
        UART6_SendString(buffer);
        
        // 尝试接收1个字节（非阻塞，超时10ms）
        if (HAL_UART_Receive(&huart6, &rxByte, 1, 10) == HAL_OK) {
            // 回显
            HAL_UART_Transmit(&huart6, &rxByte, 1, 100);
            
            // LED闪烁表示收到数据
            HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
        }
        
        // LED慢闪表示运行
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
        HAL_Delay(1000);
    }
}

/**
 * @brief 错误处理
 */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_Delay(100);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    while (1) {
    }
}
#endif
