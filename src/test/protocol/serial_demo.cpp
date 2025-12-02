/**
 * @file serial_demo.cpp
 * @brief 串口通讯类使用示例
 * @date 2025-12-02
 *
 * @details
 * 演示SerialPort类的各种使用方法：
 * 1. 基本收发
 * 2. DMA + IDLE中断模式
 * 3. 回调函数使用
 * 4. 环形缓冲区读取
 */

#include <stdio.h>
#include "../../drivers/protocol/serial_port.hpp"
#include "gpio.h"
#include "main.h"

// 全局串口对象
SerialPort uart1(SerialType::UART6);
SerialPort uart6(SerialType::UART1);

// 接收数据缓冲区
uint8_t rxData[256];

/**
 * @brief UART1接收回调函数
 * @param data 接收到的数据
 * @param length 数据长度
 */
void uart1RxCallback(uint8_t* data, size_t length) {
    // 方式1: 直接在回调中处理数据
    // 注意：回调在中断中执行，应尽快返回

    // 示例：回显接收到的数据
    uart1.send(data, length);

    // 示例：打印接收到的数据（调试用）
    // printf("UART1 Received %d bytes\r\n", length);
}

/**
 * @brief UART6接收回调函数
 */
void uart6RxCallback(uint8_t* data, size_t length) {
    // 处理UART6接收到的数据
    uart6.send((const uint8_t*)"UART6 ACK\r\n", 11);
}

/**
 * @brief 示例1: 基本初始化和发送
 */
void example1_basic_usage() {
    // 使用默认配置初始化 (115200, 8N1, DMA+IDLE模式)
    SerialConfig config;
    uart1.init(config);

    // 发送字符串
    uart1.sendString("Hello from UART1!\r\n");

    // 发送字节数组
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uart1.send(data, sizeof(data));
}

/**
 * @brief 示例2: 自定义配置
 */
void example2_custom_config() {
    // 自定义配置
    SerialConfig config;
    config.baudrate = 9600;  // 9600波特率
    config.wordLength = UART_WORDLENGTH_8B;
    config.stopBits = UART_STOPBITS_1;
    config.parity = UART_PARITY_NONE;
    config.mode = SerialMode::DMA_IDLE;  // DMA + IDLE中断模式

    uart6.init(config);
    uart6.sendString("UART6 initialized with 9600 baud\r\n");
}

/**
 * @brief 示例3: 使用回调函数接收数据
 */
void example3_callback_receive() {
    // 初始化
    uart1.init();

    // 设置接收回调函数
    uart1.setRxCallback(uart1RxCallback);

    // 发送提示信息
    uart1.sendString("Send me something, I will echo it back!\r\n");

    // 数据接收在中断中自动处理，无需手动调用
}

/**
 * @brief 示例4: 从环形缓冲区读取数据
 */
void example4_ring_buffer_read() {
    uart1.init();

    while (1) {
        // 检查是否有可用数据
        if (uart1.available() > 0) {
            // 读取数据
            size_t len = uart1.read(rxData, sizeof(rxData));

            // 处理数据
            if (len > 0) {
                // 示例：回显数据
                uart1.send(rxData, len);

                // 示例：解析命令
                if (rxData[0] == 'A') {
                    uart1.sendString("Command A received\r\n");
                } else if (rxData[0] == 'B') {
                    uart1.sendString("Command B received\r\n");
                }
            }
        }

        HAL_Delay(10);  // 延时10ms
    }
}

/**
 * @brief 示例5: 阻塞模式接收（不推荐，仅用于简单场景）
 */
void example5_blocking_receive() {
    SerialConfig config;
    config.mode = SerialMode::POLLING;  // 阻塞轮询模式
    uart1.init(config);

    uart1.sendString("Waiting for 10 bytes...\r\n");

    // 阻塞接收10个字节，超时1000ms
    SerialStatus status = uart1.receive(rxData, 10, 1000);

    if (status == SerialStatus::OK) {
        uart1.sendString("Received successfully!\r\n");
        uart1.send(rxData, 10);
    } else if (status == SerialStatus::TIMEOUT) {
        uart1.sendString("Receive timeout!\r\n");
    }
}

/**
 * @brief 示例6: 多串口同时使用
 */
void example6_multiple_serial() {
    // 初始化UART1
    SerialConfig config1;
    config1.baudrate = 115200;
    uart1.init(config1);
    uart1.setRxCallback(uart1RxCallback);

    // 初始化UART6
    SerialConfig config6;
    config6.baudrate = 9600;
    uart6.init(config6);
    uart6.setRxCallback(uart6RxCallback);

    // 两个串口独立工作
    uart1.sendString("UART1 ready\r\n");
    uart6.sendString("UART6 ready\r\n");
}

/**
 * @brief 示例7: 发送格式化字符串
 */
void example7_printf_style() {
    uart1.init();

    // 使用sprintf格式化字符串
    char buffer[128];
    int value = 12345;
    float temperature = 25.6f;

    sprintf(buffer, "Value: %d, Temp: %.1f°C\r\n", value, temperature);
    uart1.sendString(buffer);

    // 或者使用snprintf更安全
    snprintf(buffer, sizeof(buffer), "System time: %lu ms\r\n", HAL_GetTick());
    uart1.sendString(buffer);
}

/**
 * @brief 示例8: 错误处理
 */
void example8_error_handling() {
    SerialStatus status;

    // 初始化
    status = uart1.init();
    if (status != SerialStatus::OK) {
        // 初始化失败处理
        return;
    }

    // 发送数据
    status = uart1.send((const uint8_t*)"Test", 4);
    if (status == SerialStatus::BUSY) {
        // 串口忙，等待或重试
        HAL_Delay(10);
    } else if (status == SerialStatus::ERROR) {
        // 发送错误
    }

    // 检查串口状态
    if (uart1.isBusy()) {
        // 串口正在传输数据
    }
}

extern "C" void SystemClock_Config(void);
/**
 * @brief 主函数 - 选择要运行的示例
 */
int main(void) {
    // 系统初始化
    HAL_Init();
    MX_GPIO_Init();
    SystemClock_Config();

    // 运行示例（取消注释要运行的示例）



    // example1_basic_usage();
    // example2_custom_config();
    example3_callback_receive();  // 推荐使用
    // example4_ring_buffer_read();
    // example5_blocking_receive();
    // example6_multiple_serial();
    // example7_printf_style();
    // example8_error_handling();

    // 主循环
    while (1) {
        // 在DMA+IDLE模式下，接收在中断中自动处理
        // 主循环可以处理其他任务
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        HAL_Delay(500);
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
        HAL_Delay(500);
    }
}

/**
 * @brief 系统时钟配置（示例）
 */
extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;  // 测试HSE=8MHz: 8/4=2MHz
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}
