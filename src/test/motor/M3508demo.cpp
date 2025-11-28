//
// C++ 入口: 使用 CanBus 封装与 GM6020 示例进行最小测试
// 注意: 由于 HAL 与外设初始化为 C 文件实现，需要正确处理 extern "C"
//

extern "C" {
#include "stm32f4xx_hal.h"

//注意 硬件初始化文件一定要 extern "C"
#include "hal/can.h"
#include "hal/gpio.h"
// 来自 C 源文件的函数原型（C 链接）
void SystemClock_Config(void);
void Error_Handler(void);
void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void can_filter_init(void);
void MX_GPIO_Init(void);
}

#include "drivers/protocol/can_comm.hpp"
#include "drivers/motor/gm6020.hpp"
#include "drivers/motor/m3508.hpp"

// 选择要使用的 CAN 口: 可改成 &hcan2
static CanBus g_can(&hcan1);

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    can_filter_init();

    // 可选: 绑定接收回调（当前不处理反馈）
    g_can.attachRxCallback(nullptr, nullptr);

    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_SET);

    // 示例：M3508 底盘 4 电机测试：每 5ms 通过 0x200 组播发送 4 路电流
    while (1) {
        while (g_can.pollOnce()) {
        }
        M3508::sendCurrentGroup2(&g_can, 2000, 4000, 4000, 4000);
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
    // 可在此处添加调试行为（如点灯）
    while (1) {
    }
}
