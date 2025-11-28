/**
 * @file bsp_config.h
 * @brief RM C型板BSP配置文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件定义BSP相关的配置选项和宏定义。
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ARM CMSIS兼容性定义 */
#ifndef __weak
    #define __weak    __attribute__((weak))
#endif

#ifndef __NOP
    #define __NOP()   __asm__ volatile ("nop")
#endif

/* Exported constants --------------------------------------------------------*/
/* BSP版本信息 */
#define BSP_VERSION_MAJOR    1
#define BSP_VERSION_MINOR    0
#define BSP_VERSION_PATCH    0

/* 调试配置 */
#define BSP_DEBUG_ENABLE     1    /**< 使能调试输出 */
#define BSP_ASSERT_ENABLE    1    /**< 使能断言检查 */

/* 硬件配置 */
#define BSP_BOARD_TYPE       "RM_C_BOARD"  /**< 开发板类型 */
#define BSP_MCU_TYPE         "STM32F407IGT6" /**< MCU型号 */

/* 时钟配置 */
#define BSP_HSE_FREQUENCY     8000000U   /**< 外部晶振频率 8MHz */
#define BSP_LSE_FREQUENCY     32768U     /**< 外部低速晶振频率 32.768kHz */

/* 系统时钟配置 */
#define BSP_SYSCLK_SOURCE    RCC_SYSCLKSOURCE_PLLCLK  /**< 系统时钟源 */
#define BSP_SYSCLK_FREQ      168000000U  /**< 系统时钟频率 168MHz */
#define BSP_AHB_FREQ         168000000U  /**< AHB时钟频率 168MHz */
#define BSP_APB1_FREQ        42000000U   /**< APB1时钟频率 42MHz */
#define BSP_APB2_FREQ        84000000U   /**< APB2时钟频率 84MHz */

/* PLL配置 */
#define BSP_PLL_M            6   /**< PLL分频系数M */
#define BSP_PLL_N            168 /**< PLL倍频系数N */
#define BSP_PLL_P            RCC_PLLP_DIV2  /**< PLL分频系数P */
#define BSP_PLL_Q            7   /**< PLL分频系数Q */

/* Flash配置 */
#define BSP_FLASH_LATENCY    FLASH_LATENCY_5  /**< Flash等待周期 */

/* 电源配置 */
#define BSP_VOLTAGE_SCALE    PWR_REGULATOR_VOLTAGE_SCALE1  /**< 电压调节器配置 */

/* 外设配置宏 */
#define BSP_UART1_ENABLE     1   /**< 使能UART1 */
#define BSP_UART2_ENABLE     1   /**< 使能UART2 */
#define BSP_UART3_ENABLE     1   /**< 使能UART3 */
#define BSP_UART6_ENABLE     1   /**< 使能UART6 */

#define BSP_CAN1_ENABLE      1   /**< 使能CAN1 */
#define BSP_CAN2_ENABLE      1   /**< 使能CAN2 */

#define BSP_SPI1_ENABLE      1   /**< 使能SPI1 */
#define SPI2_ENABLE          1   /**< 使能SPI2 */

#define BSP_I2C1_ENABLE      1   /**< 使能I2C1 */
#define BSP_I2C2_ENABLE      1   /**< 使能I2C2 */
#define BSP_I2C3_ENABLE      1   /**< 使能I2C3 */

#define BSP_TIM1_ENABLE      1   /**< 使能TIM1 */
#define BSP_TIM2_ENABLE      1   /**< 使能TIM2 */
#define BSP_TIM3_ENABLE      1   /**< 使能TIM3 */
#define BSP_TIM4_ENABLE      1   /**< 使能TIM4 */
#define BSP_TIM5_ENABLE      1   /**< 使能TIM5 */
#define BSP_TIM8_ENABLE      1   /**< 使能TIM8 */
#define BSP_TIM10_ENABLE     1   /**< 使能TIM10 */

#define BSP_ADC1_ENABLE      1   /**< 使能ADC1 */
#define BSP_ADC3_ENABLE      1   /**< 使能ADC3 */

#define BSP_USB_ENABLE       1   /**< 使能USB */

#define BSP_DMA_ENABLE       1   /**< 使能DMA */

/* GPIO配置 */
#define BSP_GPIO_ENABLE       1   /**< 使能GPIO */

/* 中断优先级配置 */
#define BSP_PRIORITY_TICK     0   /**< SysTick中断优先级 */
#define BSP_PRIORITY_PENDSV   15  /**< PendSV中断优先级 */
#define BSP_PRIORITY_UART     5   /**< UART中断优先级 */
#define BSP_PRIORITY_CAN      6   /**< CAN中断优先级 */
#define BSP_PRIORITY_TIM      7   /**< 定时器中断优先级 */
#define BSP_PRIORITY_ADC      8   /**< ADC中断优先级 */
#define BSP_PRIORITY_USB      9   /**< USB中断优先级 */

/* 内存配置 */
#define BSP_HEAP_SIZE         0x400   /**< 堆大小 */
#define BSP_STACK_SIZE       0x800   /**< 栈大小 */

/* 调试宏定义 */
#if BSP_DEBUG_ENABLE
    #define BSP_DEBUG(fmt, ...)  do { \
        /* printf("[DEBUG] %s:%d: " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__); */ \
    } while(0)
#else
    #define BSP_DEBUG(fmt, ...)  do {} while(0)
#endif

#if BSP_ASSERT_ENABLE
    #define BSP_ASSERT(expr)  do { \
        if (!(expr)) { \
            BSP_DEBUG("Assertion failed: %s", #expr); \
            while(1) { __NOP(); } \
        } \
    } while(0)
#else
    #define BSP_ASSERT(expr)  do {} while(0)
#endif

/* 错误处理宏 */
#define BSP_ERROR_MSG(msg)   do { \
    BSP_DEBUG("Error: %s", msg); \
    while(1) { __NOP(); } \
} while(0)

/* 功能开关宏 */
#define BSP_LOWPOWER_ENABLE  0   /**< 使能低功耗功能 */
#define BSP_WATCHDOG_ENABLE  0   /**< 使能看门狗功能 */
#define BSP_RTC_ENABLE       1   /**< 使能RTC功能 */

/* 性能优化宏 */
#define BSP_CACHE_ENABLE     1   /**< 使能Cache */
#define BSP_PREFETCH_ENABLE  1   /**< 使能预取指 */

/* 兼容性宏 */
#ifndef NULL
    #define NULL    ((void*)0)
#endif

#ifndef true
    #define true    1
#endif

#ifndef false
    #define false   0
#endif

/* 导出配置函数声明 --------------------------------------------------------*/
/**
 * @brief 获取BSP版本字符串
 * @return const char* 版本字符串
 */
const char* BSP_GetVersion(void);

/**
 * @brief 获取开发板类型
 * @return const char* 开发板类型字符串
 */
const char* BSP_GetBoardType(void);

/**
 * @brief 获取MCU型号
 * @return const char* MCU型号字符串
 */
const char* BSP_GetMCUType(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */
