/**
 * @file simple_uart_test.cpp
 * @brief 简单的串口测试程序
 * @date 2025-12-02
 * 
 * @details
 * 这是一个最简单的串口测试程序，用于验证SerialPort类的基本功能
 * 功能：
 * 1. 初始化UART1
 * 2. 每秒发送一次心跳消息
 * 3. 回显接收到的所有数据
 */

#include "main.h"
#include "../../drivers/protocol/serial_port.hpp"
#include <stdio.h>

// 全局串口对象
SerialPort uart1(SerialType::UART1);

// 接收计数器
volatile uint32_t rxCount = 0;

/**
 * @brief UART接收回调函数
 * @param data 接收到的数据
 * @param length 数据长度
 */
void onUartReceive(uint8_t* data, size_t length) {
    rxCount++;
    
    // 回显接收到的数据
    uart1.send(data, length);
    
    // 可选：添加换行
    // uart1.sendString("\r\n");
}

/**
 * @brief 系统时钟配置
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 使能电源时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    // 配置HSE和PLL
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
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    // 1. 系统初始化
    HAL_Init();
    SystemClock_Config();
    
    // 2. 初始化串口（使用默认配置：115200, 8N1, DMA+IDLE）
    SerialStatus status = uart1.init();
    
    if (status != SerialStatus::OK) {
        // 初始化失败，进入错误处理
        Error_Handler();
    }
    
    // 3. 设置接收回调函数
    uart1.setRxCallback(onUartReceive);
    
    // 4. 发送启动消息
    uart1.sendString("\r\n");
    uart1.sendString("========================================\r\n");
    uart1.sendString("  SerialPort Test Program v1.0\r\n");
    uart1.sendString("========================================\r\n");
    uart1.sendString("UART1 initialized successfully!\r\n");
    uart1.sendString("Baudrate: 115200, 8N1, DMA+IDLE mode\r\n");
    uart1.sendString("Send me something, I will echo it back!\r\n");
    uart1.sendString("========================================\r\n\r\n");
    
    // 5. 主循环
    uint32_t lastHeartbeat = 0;
    uint32_t heartbeatCount = 0;
    
    while (1) {
        uint32_t currentTime = HAL_GetTick();
        
        // 每1000ms发送一次心跳消息
        if (currentTime - lastHeartbeat >= 1000) {
            lastHeartbeat = currentTime;
            heartbeatCount++;
            
            // 格式化心跳消息
            char heartbeat[128];
            snprintf(heartbeat, sizeof(heartbeat), 
                     "[%lu] Heartbeat #%lu, RX Count: %lu\r\n",
                     currentTime, heartbeatCount, rxCount);
            
            uart1.sendString(heartbeat);
        }
        
        // 可选：从缓冲区读取数据（如果不使用回调）
        // if (uart1.available() > 0) {
        //     uint8_t buffer[256];
        //     size_t len = uart1.read(buffer, sizeof(buffer));
        //     uart1.send(buffer, len);
        // }
        
        // 短暂延时，避免CPU空转
        HAL_Delay(10);
    }
}

/**
 * @brief 错误处理函数
 */
void Error_Handler(void) {
    // 错误指示：可以点亮LED或其他操作
    __disable_irq();
    while (1) {
        // 死循环
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief 断言失败处理
 */
void assert_failed(uint8_t *file, uint32_t line) {
    // 可以在这里添加调试信息
    while (1) {
    }
}
#endif
