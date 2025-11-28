/**
 * @file main.cpp
 * @brief RM C型板主程序模板
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 *
 * 本文件是RM C型开发板的主程序模板，
 * 使用BSP统一接口进行硬件初始化。
 */

extern "C" {
#include "bsp/bsp_board.h"
#include "stm32f4xx_hal.h"
}

/**
 * @brief 重写BSP初始化完成回调函数
 */
void BSP_InitCompletedCallback(void) {
    // BSP初始化完成，用户可在此处添加初始化完成后的代码
}

/**
 * @brief 重写BSP错误回调函数
 * @param error_code 错误代码
 */
void BSP_ErrorCallback(uint32_t error_code) {
    // 错误处理，用户可在此处添加自定义错误处理逻辑
    while (1) {
        // 错误状态，可在此处添加错误指示（如LED闪烁等）
    }
}

extern "C" {
void init_test_led();
}

/**
 * @brief 主函数
 * @return int 程序返回值
 */
int main(void) {
    BSP_StatusTypeDef status;

    // BSP完整初始化
    BSP_InitTypeDef config = BSP_INIT_DEFAULT;
    status = BSP_Init(&config);

    if (status != BSP_OK) {
        // 初始化失败，进入错误处理
        BSP_Error_Handler(__FILE__, __LINE__);
    }

    init_test_led();

    // 主循环
    while (1) {
        // 用户代码区域
        // 在此处添加应用程序逻辑

        // 示例：LED测试 - 使用BSP LED功能
        BSP_LED_SetPresetColor(BSP_LED_COLOR_RED);
        BSP_Delay(1000);
        BSP_LED_SetPresetColor(BSP_LED_COLOR_GREEN);
        BSP_Delay(1000);
        BSP_LED_SetPresetColor(BSP_LED_COLOR_BLUE);
        BSP_Delay(1000);
        BSP_LED_Off();
        BSP_Delay(1000);
    }

    return 0;
}

extern "C" {
#include "bsp/bsp_led.h"
}

extern "C" void init_test_led() {
    // 使用BSP LED初始化替代直接GPIO配置
    BSP_LED_Init();
}