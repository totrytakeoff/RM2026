/**
 * @file uart6_hsi_test.cpp
 * @brief UART6 "Safe Mode" Test using HSI clock
 * @date 2025-12-03
 * 
 * @details
 * This program bypasses the HSE/PLL clock configuration entirely and runs the MCU
 * directly off the internal 16MHz HSI oscillator. This is the most reliable way
 * to test UART functionality, as it eliminates all clock-related variables.
 * If this test works, the problem is confirmed to be in the SystemClock_Config.
 * If it fails, the problem is in the GPIO, UART peripheral, or hardware.
 */

#include "main.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

// We will use the HAL library directly to keep it simple
UART_HandleTypeDef huart6;

/**
 * @brief Configures the system clock to run directly from HSI at 16MHz.
 *        NO PLL, NO HSE.
 */
void SystemClock_Config_HSI(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 1. Configure HSI as the clock source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // Turn OFF the PLL
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // 2. Set HSI as the SYSCLK source, and set all prescalers to 1
    // This makes SYSCLK = HCLK = PCLK1 = PCLK2 = 16MHz
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
    
    // The HAL_Init() function will update the SystemCoreClock variable with this new value (16MHz)
}

/**
 * @brief Initializes UART6 using HAL functions.
 */
void MX_USART6_UART_Init(void)
{
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief This function is called by HAL_UART_Init()
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(huart->Instance==USART6)
    {
        __HAL_RCC_USART6_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        
        // PG14 (TX) and PG9 (RX)
        GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    }
}

/**
 * @brief Main function
 */
int main(void)
{
    // 1. Initialize HAL Libs
    HAL_Init();

    // 2. Configure system clock to 16MHz HSI
    SystemClock_Config_HSI();

    // 3. Initialize GPIOs (for LEDs)
    MX_GPIO_Init();

    // 4. Initialize UART6
    MX_USART6_UART_Init();

    char msg[128];
    HAL_UART_Transmit(&huart6, (uint8_t*)"\r\n--- UART HSI Safe Mode Test ---\r\n", 37, 1000);
    
    sprintf(msg, "SYSCLK is now: %lu Hz\r\n", HAL_RCC_GetSysClockFreq());
    HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 1000);
    
    sprintf(msg, "PCLK2 (UART6 Clock) is now: %lu Hz\r\n", HAL_RCC_GetPCLK2Freq());
    HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 1000);
    
    HAL_UART_Transmit(&huart6, (uint8_t*)"Baudrate should be 115200. Please check.\r\n", 46, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t*)"--- Starting echo test ---\r\n", 30, 1000);

    uint8_t rxByte;
    while (1)
    {
        // Echo any received byte
        if (HAL_UART_Receive(&huart6, &rxByte, 1, 10) == HAL_OK)
        {
            HAL_UART_Transmit(&huart6, &rxByte, 1, 100);
            HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin); // Green LED for RX activity
        }

        // Blue LED for heartbeat
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
        HAL_Delay(500);
    }
}

/**
 * @brief Error Handler
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        // Red LED blinking indicates an error
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_Delay(100);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
