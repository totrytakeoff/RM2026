/**
 * @file usb_simple_demo.cpp
 * @brief USB通讯最简单示例 - 回显功能
 * @version 1.0
 * @date 2024-12-04
 * 
 * @details
 * 这是一个最简单的USB通讯示例，实现以下功能：
 * - USB虚拟串口初始化
 * - 接收PC发送的数据
 * - 将接收到的数据回显给PC
 * - LED指示连接状态
 * 
 * 使用方法：
 * 1. 编译并烧录到开发板
 * 2. 用USB线连接开发板到PC
 * 3. 打开串口助手（如PuTTY、Tera Term等）
 * 4. 选择对应的COM口（波特率任意，USB CDC不需要配置）
 * 5. 发送任意数据，开发板会回显
 */

#include "main.h"
#include "gpio.h"
#include "../../drivers/protocol/usb_port.hpp"

// 全局USB实例
USBPort usb;

// 系统时钟配置
void SystemClock_Config(void);

/**
 * @brief USB接收回调 - 实现回显
 */
void onReceive(uint8_t* data, size_t length)
{
    // 行缓冲：只有当收到换行或回车时才回显整行，避免每个字符立即回显
    static char linebuf[256];
    static size_t lineidx = 0;

    for (size_t i = 0; i < length; ++i) {
        char c = (char)data[i];
        // 忽略回车符中的回车（但当遇到换行或回车都视为一行结束）
        if (c == '\r' || c == '\n') {
            if (lineidx == 0) {
                // 空行：直接发送一个换行回显
                usb.sendString("\r\n");
            } else {
                // 终止并处理整行
                linebuf[lineidx] = '\0';
                // 特殊处理：如果行首是 'H'，发送自定义消息
                if (linebuf[0] == '/') {
                    // 合并命令响应为一次发送，减少发送队列压力
                    char resp[128];
                    int rn = snprintf(resp, sizeof(resp), "\r\n*** cmd--- ***\r\n");
                    if (strcmp(linebuf, "/help") == 0) {
                        rn += snprintf(resp + rn, sizeof(resp) - rn, "*** help --- ***\r\n");
                    } else if (strcmp(linebuf, "/status") == 0) {
                        rn += snprintf(resp + rn, sizeof(resp) - rn, "*** status --- ***\r\n");
                    }
                    // 立即发送合并后的响应（如果队列满会返回 BUSY，但减少次数）
                    usb.send((const uint8_t*)resp, (size_t)rn);
                    if(strcmp(linebuf, "/R") == 0) {
                        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
                    }
                    if(strcmp(linebuf, "/G") == 0) {
                        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
                    }
                    if(strcmp(linebuf, "/B") == 0) {
                        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
                    }
                    if(strcmp(linebuf, "/r") == 0) {
                        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
                    }
                    if(strcmp(linebuf, "/g") == 0) {
                        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
                    }
                    if(strcmp(linebuf, "/b") == 0) {
                        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
                    }


                } else {
                    // 回显整行一次，避免分多次发送
                    char out[260];
                    int n = snprintf(out, sizeof(out), "%s\r\n", linebuf);
                    usb.send((const uint8_t*)out, (size_t)n);
                }
                lineidx = 0;
            }
        } else {
            // 普通字符，追加到行缓冲
            if (lineidx < sizeof(linebuf) - 1) {
                linebuf[lineidx++] = c;
            } else {
                // 缓冲区溢出：丢弃并重置
                lineidx = 0;
            }
        }
    }
}

/**
 * @brief USB连接状态回调
 * @note 重要：不要在连接回调中发送数据！
 *       此时USB可能还在初始化流程中，发送会导致阻塞或失败
 *       应该在主循环中检测连接状态后再发送
 */
void onConnect(USBConnectionState state)
{
    if (state == USBConnectionState::CONNECTED) {
        // 连接成功，点亮绿色LED
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        // ❌ 不要在这里发送！会导致阻塞
        // usb.sendString("\r\n*** USB Connected ***\r\n");
    } else {
        // 断开连接，熄灭绿色LED
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 主程序
 */
int main(void)
{
    // 初始化HAL库和硬件
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // 初始化USB（3行代码完成USB通讯）
    usb.init();
    usb.setRxCallback(onReceive);
    usb.setConnectCallback(onConnect);
    
    // 等待USB连接并发送欢迎信息（只发送一次）
    bool welcomeSent = false;
    
    // 主循环
    while (1) {
        // 主循环：频繁调用 poll()，并用非阻塞定时发送心跳，避免长时间阻塞
        usb.poll();

        // 连接后发送欢迎信息（只发送一次）
        if (usb.isConnected() && !welcomeSent) {
            // 等待USB完全稳定，但不要长时间阻塞主循环
            uint32_t start = HAL_GetTick();
            while ((HAL_GetTick() - start) < 100) {
                usb.poll();
            }
            usb.sendString("\r\n*** USB Connected ***\r\n");
            usb.sendString("Echo mode: Type anything...\r\n");
            welcomeSent = true;
        }

        if (!usb.isConnected() && welcomeSent) {
            welcomeSent = false;
        }

        // 非阻塞心跳：每500ms发送一次，但主循环始终保持短延时以处理 poll
        static uint32_t lastBeat = 0;
        uint32_t now = HAL_GetTick();
        if ((now - lastBeat) >= 500) {
            lastBeat = now;
            usb.sendString("*-*\r\n");
            HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        }

        // 小延时提高CPU利用但保持响应性
        HAL_Delay(5);
    }
}

/**
 * @brief 系统时钟配置 - 168MHz系统时钟，48MHz USB时钟
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;   // 12MHz/6 = 2MHz
    RCC_OscInitStruct.PLL.PLLN = 168; // 2MHz*168 = 336MHz
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 336MHz/2 = 168MHz (系统)
    RCC_OscInitStruct.PLL.PLLQ = 7;   // 336MHz/7 = 48MHz (USB)
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

void Error_Handler(void)
{
    while (1) {
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_Delay(100);
    }
}
