 //
// C++ 入口: 使用 CanBus 封装与 GM6020 示例进行最小测试
// 注意: 由于 HAL 与外设初始化为 C 文件实现，需要正确处理 extern "C"
//

extern "C" {
#include "main.h"
#include "stm32f4xx_hal.h"
#include "hal/gpio.h"
#include "hal/can.h"
// 来自 C 源文件的函数原型（C 链接）
void SystemClock_Config(void);
void Error_Handler(void);
}

#include "../drivers/protocol/can_comm.hpp"
#include "../drivers/motor/gm6020.hpp"

// 选择要使用的 CAN 口: 可改成 &hcan2
static CanBus g_can(&hcan1);

int main(void)
{
    // 基本初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    // 初始化所有LED为关闭状态
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);

    // 调试：LED闪烁表示初始化成功
    for(int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10, GPIO_PIN_SET);   // 蓝灯亮
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10, GPIO_PIN_RESET); // 蓝灯灭
        HAL_Delay(200);
    }

    // CAN初始化
    MX_CAN1_Init();
    MX_CAN2_Init();
    can_filter_init();

    
    // CAN初始化成功，绿色LED闪烁
    for(int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_SET);   // 绿灯亮
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_RESET); // 绿灯灭
        HAL_Delay(200);
    }

    // 可选: 绑定接收回调（当前不处理反馈）
    g_can.attachRxCallback(nullptr, nullptr);

    // 初始化所有LED为关闭状态
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);

    // 调试LED闪烁表示程序运行
    uint32_t led_counter = 0;
    uint32_t can_error_count = 0;

    // 示例：每 5ms 发送 1/4 号电机电流指令，2/3 置 0
    while (1)
    {
        while (g_can.pollOnce()) {}

        // 发送CAN命令
        bool success = GM6020::sendCurrentGroup(&g_can, 0x1FF, 20000, 0, 0, 10000);

        // 调试：LED闪烁表示发送状态
        led_counter++;
        if (led_counter % 200 == 0) {  // 每1秒闪烁一次 (200 * 5ms)
            HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_10);  // 蓝色LED闪烁表示程序正在运行
        }

        // 如果CAN发送失败，点亮红色LED并计数
        if (!success) {
            can_error_count++;
            HAL_GPIO_WritePin(GPIOH, GPIO_PIN_12, GPIO_PIN_SET);  // 红色LED亮表示CAN错误
        } else {
            // 成功发送，绿色LED闪烁
            if (led_counter % 100 == 0) {  // 每500ms闪烁一次
                HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_11);
            }
        }

        HAL_Delay(5);
    }
}

extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

    RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

extern "C" void Error_Handler(void) {
    // 错误处理：快速闪烁所有LED表示错误
    while (1) {
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET); // 所有LED亮
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET); // 所有LED灭
        HAL_Delay(100);
    }
}
