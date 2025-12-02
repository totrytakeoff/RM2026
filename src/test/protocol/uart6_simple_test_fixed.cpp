/**
 * @file uart6_simple_test_fixed.cpp
 * @brief 使用 HSI 时钟的、修复版的 SerialPort 测试程序
 * @date 2025-12-03
 * 
 * @details
 * 1. 使用 HSI 时钟，彻底避免外部晶振问题。
 * 2. 使用您封装的 SerialPort 类，验证其在正确时钟下的工作情况。
 */

#include "main.h"
#include "gpio.h"
#include "../../drivers/protocol/serial_port.hpp"
#include <stdio.h>
#include <string.h>

// 使用您封装的类和UART6
SerialPort uart6(SerialType::UART6);

/**
 * @brief 配置系统时钟为 16MHz HSI (安全模式)
 */
void SystemClock_Config_HSI(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // 关闭PLL
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

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
}

/**
 * @brief UART6接收回调
 */
void onUart6Receive(uint8_t* data, size_t length) {
    // 绿灯闪烁，表示收到了数据
    HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
    
    // 回显数据
    uart6.send(data, length);
}

/**
 * @brief 主函数
 */
int main(void) {
    HAL_Init();
    
    // 使用安全的HSI时钟配置！
    SystemClock_Config_HSI();
    
    MX_GPIO_Init();
    
    // 初始化您的 SerialPort 类
    SerialConfig config;
    config.baudrate = 115200;
    config.mode = SerialMode::DMA_IDLE; // 使用您封装的DMA+IDLE模式
    uart6.init(config);
    
    // 设置回调函数
    uart6.setRxCallback(onUart6Receive);
    
    uart6.sendString("\r\n--- SerialPort Class with HSI Clock Test ---\r\n");
    char msg[128];
    sprintf(msg, "SYSCLK: %lu Hz\r\n", HAL_RCC_GetSysClockFreq());
    uart6.sendString(msg);
    uart6.sendString("--- Echo test started ---\r\n");
    
    while (1) {
        // 蓝灯心跳
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
        HAL_Delay(500);
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
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
