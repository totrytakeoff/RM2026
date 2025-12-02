/**
 * @file uart6_simple_test.cpp
 * @brief UART6 最简单的测试程序
 * @date 2025-12-03
 * 
 * 专门用于测试UART6的基本收发功能
 */

#include "main.h"
#include "gpio.h"
#include "../../drivers/protocol/serial_port.hpp"
#include <stdio.h>
#include <stdint.h>

// 使用UART6（3针接口，外壳丝印UART1）
SerialPort uart6(SerialType::UART6);

// 接收计数器
volatile uint32_t rxCount = 0;

/**
 * @brief UART6接收回调
 */
void onUart6Receive(uint8_t* data, size_t length) {
    rxCount++;
    
    // 翻转LED指示收到数据
    HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
    
    // 回显数据
    uart6.send(data, length);
}

/**
 * @brief 系统时钟配置
 */
extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 使能PWR时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    // 配置HSE和PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;  // 改为4，假设HSE=8MHz: 8/4=2MHz
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
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  // APB1 = 42MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  // APB2 = 84MHz (UART6在这里)
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    // 1. HAL库初始化
    HAL_Init();
    
    // 2. 配置系统时钟
    SystemClock_Config();
    
    // 3. 初始化GPIO（LED）
    MX_GPIO_Init();
    
    // 4. 初始化UART6
    SerialConfig config;
    config.baudrate = 115200;  // 确保是115200
    config.mode = SerialMode::DMA_IDLE;
    
    SerialStatus status = uart6.init(config);
    
    if (status != SerialStatus::OK) {
        // 初始化失败，LED快闪
        while(1) {
            HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
            HAL_Delay(100);
        }
    }
    
    // 5. 设置接收回调
    uart6.setRxCallback(onUart6Receive);
    
    // 6. 发送启动消息
    uart6.sendString("\r\n");
    uart6.sendString("========================================\r\n");
    uart6.sendString("  UART6 Test Program\r\n");
    uart6.sendString("========================================\r\n");
    uart6.sendString("Baudrate: 115200 bps\r\n");
    uart6.sendString("Mode: DMA + IDLE\r\n");
    uart6.sendString("Hardware: UART6 (PG14/PG9)\r\n");
    uart6.sendString("Send me something!\r\n");
    uart6.sendString("========================================\r\n\r\n");
    
    // 7. 主循环
    uint32_t lastHeartbeat = 0;
    uint32_t heartbeatCount = 0;
    
    while (1) {
        uint32_t currentTime = HAL_GetTick();
        
        // 每2秒发送一次心跳
        if (currentTime - lastHeartbeat >= 2000) {
            lastHeartbeat = currentTime;
            heartbeatCount++;
            
            char msg[128];
            snprintf(msg, sizeof(msg), 
                     "[%lu ms] Heartbeat #%lu | RX Count: %lu\r\n",
                     currentTime, heartbeatCount, rxCount);
            
            uart6.sendString(msg);
        }
        
        // LED慢闪表示正常运行
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
        HAL_Delay(500);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
        HAL_Delay(500);
    }
}

/**
 * @brief 错误处理
 */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        // 红灯快闪表示错误
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
