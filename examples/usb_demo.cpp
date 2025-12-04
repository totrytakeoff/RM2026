/**
 * @file usb_demo.cpp
 * @brief USB通讯类使用示例
 * @version 1.0
 * @date 2024-12-04
 * 
 * @details
 * 本文件演示USBPort类的各种使用方式：
 * 1. 基础发送/接收
 * 2. 回调机制
 * 3. 格式化输出
 * 4. 连接状态管理
 * 5. 数据回显
 */

#include "main.h"
#include "gpio.h"
#include "usb_port.hpp"

// 全局USB实例
USBPort usb;

// 系统时钟配置（与usb_led_main.c相同）
void SystemClock_Config(void);

// 示例计数器
static uint32_t counter = 0;

/**
 * @brief USB接收回调函数 - 实现数据回显
 * @param data 接收到的数据
 * @param length 数据长度
 */
void onUSBReceive(uint8_t* data, size_t length)
{
    // 方式1: 直接回显接收到的数据
    usb.send(data, length);
    
    // 方式2: 添加前缀回显
    // usb.sendString("Echo: ");
    // usb.send(data, length);
    // usb.sendString("\r\n");
}

/**
 * @brief USB连接状态回调函数
 * @param state 连接状态
 */
void onUSBConnect(USBConnectionState state)
{
    if (state == USBConnectionState::CONNECTED) {
        // USB连接成功，点亮绿色LED
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        
        // 发送欢迎消息
        usb.sendString("\r\n=== USB Connected ===\r\n");
        usb.sendString("Type 'help' for available commands\r\n");
        usb.sendString("=====================\r\n");
    } 
    else if (state == USBConnectionState::DISCONNECTED) {
        // USB断开，熄灭绿色LED
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 示例1: 最简单的使用（3行代码）
 */
void example1_basic()
{
    usb.init();
    usb.sendString("Hello USB!\r\n");
}

/**
 * @brief 示例2: 使用回调接收数据
 */
void example2_callback()
{
    usb.init();
    
    // 设置接收回调
    usb.setRxCallback(onUSBReceive);
    
    // 设置连接状态回调
    usb.setConnectCallback(onUSBConnect);
    
    // 等待USB连接
    if (usb.waitForConnection(5000)) {
        usb.sendString("Connected!\r\n");
    } else {
        usb.sendString("Connection timeout\r\n");
    }
}

/**
 * @brief 示例3: 格式化输出
 */
void example3_printf()
{
    usb.init();
    
    // 等待连接
    usb.waitForConnection();
    
    // 格式化输出各种数据类型
    usb.printf("Integer: %d\r\n", 12345);
    usb.printf("Float: %.2f\r\n", 3.14159f);
    usb.printf("Hex: 0x%08X\r\n", 0xDEADBEEF);
    usb.printf("String: %s\r\n", "Hello World");
}

/**
 * @brief 示例4: 非阻塞读取
 */
void example4_nonblocking_read()
{
    usb.init();
    usb.waitForConnection();
    
    uint8_t buffer[128];
    
    while (1) {
        // 检查是否有数据可读
        if (usb.available() > 0) {
            size_t len = usb.read(buffer, sizeof(buffer));
            
            // 处理接收到的数据
            usb.printf("Received %d bytes: ", len);
            usb.send(buffer, len);
            usb.sendString("\r\n");
        }
        
        HAL_Delay(10);
    }
}

/**
 * @brief 示例5: 周期性发送数据
 */
void example5_periodic_send()
{
    usb.init();
    usb.waitForConnection();
    
    uint32_t lastSendTime = 0;
    
    while (1) {
        uint32_t currentTime = HAL_GetTick();
        
        // 每1秒发送一次状态信息
        if (currentTime - lastSendTime >= 1000) {
            lastSendTime = currentTime;
            
            usb.printf("Status: Counter=%lu, Time=%lu ms\r\n", 
                       counter++, currentTime);
        }
        
        HAL_Delay(10);
    }
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // MCU初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // 初始化USB
    usb.init();
    
    // 设置接收回调（实现回显功能）
    usb.setRxCallback(onUSBReceive);
    
    // 设置连接状态回调
    usb.setConnectCallback(onUSBConnect);
    
    // 等待USB连接（最多等待5秒）
    usb.sendString("Waiting for USB connection...\r\n");
    if (usb.waitForConnection(5000)) {
        usb.sendString("USB Connected!\r\n");
    }
    
    // 主循环
    uint32_t lastStatusTime = 0;
    uint32_t lastBlinkTime = 0;
    bool ledState = false;
    
    while (1) {
        uint32_t currentTime = HAL_GetTick();
        
        // 每2秒发送一次状态信息
        if (currentTime - lastStatusTime >= 2000) {
            lastStatusTime = currentTime;
            
            if (usb.isConnected()) {
                usb.printf("[%lu] Status: OK, Counter=%lu\r\n", 
                           currentTime, counter++);
            }
        }
        
        // LED闪烁指示系统运行
        if (currentTime - lastBlinkTime >= 500) {
            lastBlinkTime = currentTime;
            ledState = !ledState;
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, 
                            ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        
        // 短暂延时
        HAL_Delay(1);
    }
}

/**
 * @brief 系统时钟配置
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    // 配置主内部稳压器输出电压
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 初始化CPU、AHB和APB总线时钟
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    
    // PLL配置 (针对 12MHz HSE):
    // PLLM = 6:    12MHz / 6 = 2MHz (VCO输入)
    // PLLN = 168:  2MHz * 168 = 336MHz (VCO输出)
    // PLLP = 2:    336MHz / 2 = 168MHz (系统时钟)
    // PLLQ = 7:    336MHz / 7 = 48MHz (USB时钟) ✓
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // 初始化CPU、AHB和APB总线时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }

    // 配置RTC时钟
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV30;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief 错误处理函数
 */
void Error_Handler(void)
{
    // 错误时快速闪烁红色LED
    while (1) {
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_Delay(100);
    }
}
