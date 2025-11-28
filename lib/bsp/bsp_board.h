/**
 * @file bsp_board.h
 * @brief RM C型板BSP板级支持包头文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件提供RM C型开发板的板级支持包(BSP)接口，
 * 用于替代HAL_Init, SystemClock_Config, Error_Handler等函数，
 * 提供统一的硬件初始化和配置接口。
 */

#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bsp_config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief BSP初始化状态枚举
 */
typedef enum {
    BSP_OK = 0,                /**< 初始化成功 */
    BSP_ERROR = 1,             /**< 一般错误 */
    BSP_BUSY = 2,              /**< 忙碌状态 */
    BSP_TIMEOUT = 3,           /**< 超时错误 */
    BSP_NOT_READY = 4          /**< 未就绪 */
} BSP_StatusTypeDef;

/**
 * @brief BSP初始化配置结构体
 */
typedef struct {
    uint8_t enable_uart;       /**< 使能UART初始化 */
    uint8_t enable_can;        /**< 使能CAN初始化 */
    uint8_t enable_spi;        /**< 使能SPI初始化 */
    uint8_t enable_i2c;        /**< 使能I2C初始化 */
    uint8_t enable_tim;        /**< 使能定时器初始化 */
    uint8_t enable_adc;        /**< 使能ADC初始化 */
    uint8_t enable_dma;        /**< 使能DMA初始化 */
    uint8_t enable_usb;        /**< 使能USB初始化 */
    uint8_t enable_gpio;       /**< 使能GPIO初始化 */
} BSP_InitTypeDef;

/**
 * @brief BSP时钟配置结构体
 */
typedef struct {
    uint32_t sysclk_freq;      /**< 系统时钟频率 */
    uint32_t hclk_freq;        /**< AHB时钟频率 */
    uint32_t pclk1_freq;       /**< APB1时钟频率 */
    uint32_t pclk2_freq;       /**< APB2时钟频率 */
    uint32_t latency;          /**< Flash延迟 */
} BSP_ClockConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
/* 系统时钟频率定义 */
#define BSP_SYSCLK_FREQ_168MHZ    168000000U
#define BSP_HCLK_FREQ_168MHZ      168000000U
#define BSP_PCLK1_FREQ_42MHZ      42000000U
#define BSP_PCLK2_FREQ_84MHZ      84000000U
#define BSP_FLASH_LATENCY_5       5

/* 默认初始化配置宏 */
#define BSP_INIT_DEFAULT          {1, 1, 1, 1, 1, 1, 1, 1, 1}
#define BSP_INIT_MINIMAL          {0, 0, 0, 0, 0, 0, 0, 0, 1}

/* Exported macro ------------------------------------------------------------*/
/* 错误处理宏 */
#define BSP_ERROR_HANDLER()       BSP_Error_Handler(__FILE__, __LINE__)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief BSP完整初始化函数
 * @param config 初始化配置结构体指针
 * @retval BSP_StatusTypeDef 初始化状态
 * @note 此函数替代HAL_Init()和SystemClock_Config()
 */
BSP_StatusTypeDef BSP_Init(const BSP_InitTypeDef *config);

/**
 * @brief BSP最小初始化函数
 * @retval BSP_StatusTypeDef 初始化状态
 * @note 仅初始化核心功能（时钟、GPIO等）
 */
BSP_StatusTypeDef BSP_MinimalInit(void);

/**
 * @brief BSP去初始化函数
 * @retval BSP_StatusTypeDef 去初始化状态
 */
BSP_StatusTypeDef BSP_DeInit(void);

/**
 * @brief BSP系统时钟配置
 * @param clock_config 时钟配置结构体指针
 * @retval BSP_StatusTypeDef 配置状态
 * @note 替代SystemClock_Config()
 */
BSP_StatusTypeDef BSP_ClockConfig(const BSP_ClockConfigTypeDef *clock_config);

/**
 * @brief BSP外设初始化
 * @param config 初始化配置结构体指针
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_PeripheralsInit(const BSP_InitTypeDef *config);

/**
 * @brief BSP中断配置
 * @retval BSP_StatusTypeDef 配置状态
 */
BSP_StatusTypeDef BSP_InterruptConfig(void);

/**
 * @brief BSP错误处理函数
 * @param file 文件名
 * @param line 行号
 * @note 替代Error_Handler()
 */
void BSP_Error_Handler(const char *file, uint32_t line);

/**
 * @brief BSP断言失败处理函数
 * @param file 文件名
 * @param line 行号
 */
void BSP_Assert_Failed(uint8_t *file, uint32_t line);

/**
 * @brief BSP系统复位函数
 */
void BSP_SystemReset(void);

/**
 * @brief BSP获取系统时钟频率
 * @return uint32_t 系统时钟频率(Hz)
 */
uint32_t BSP_GetSystemClockFreq(void);

/**
 * @brief BSP获取AHB时钟频率
 * @return uint32_t AHB时钟频率(Hz)
 */
uint32_t BSP_GetHCLKFreq(void);

/**
 * @brief BSP获取APB1时钟频率
 * @return uint32_t APB1时钟频率(Hz)
 */
uint32_t BSP_GetPCLK1Freq(void);

/**
 * @brief BSP获取APB2时钟频率
 * @return uint32_t APB2时钟频率(Hz)
 */
uint32_t BSP_GetPCLK2Freq(void);

/**
 * @brief BSP延时函数
 * @param delay 延时时间(ms)
 */
void BSP_Delay(uint32_t delay);

/**
 * @brief BSP微秒延时函数
 * @param delay 延时时间(us)
 */
void BSP_DelayUs(uint32_t delay);

/**
 * @brief BSP获取系统滴答计数
 * @return uint32_t 滴答计数
 */
uint32_t BSP_GetTick(void);

/**
 * @brief BSP进入低功耗模式
 * @param mode 低功耗模式
 */
void BSP_EnterLowPowerMode(uint32_t mode);

/**
 * @brief BSP退出低功耗模式
 */
void BSP_ExitLowPowerMode(void);

/* Callback functions --------------------------------------------------------*/
/**
 * @brief BSP初始化完成回调函数
 * @note 用户可重写此函数
 */
__weak void BSP_InitCompletedCallback(void);

/**
 * @brief BSP错误处理回调函数
 * @param error_code 错误代码
 * @note 用户可重写此函数
 */
__weak void BSP_ErrorCallback(uint32_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_H */
