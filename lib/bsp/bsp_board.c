/**
 * @file bsp_board.c
 * @brief RM C型板BSP板级支持包实现文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 *
 * 本文件提供RM C型开发板的板级支持包(BSP)实现，
 * 用于替代HAL_Init, SystemClock_Config, Error_Handler等函数，
 * 提供统一的硬件初始化和配置接口。
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_board.h"
#include "bsp_config.h"
#include "stm32f4xx_hal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static BSP_InitTypeDef bsp_current_config = BSP_INIT_DEFAULT;
static uint8_t bsp_initialized = 0;

/* Private function prototypes -----------------------------------------------*/
static void BSP_MspInit(void);
static void BSP_MspDeInit(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief BSP完整初始化函数
 * @param config 初始化配置结构体指针
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_Init(const BSP_InitTypeDef* config) {
    BSP_StatusTypeDef bsp_status;

    /* 检查参数 */
    if (config == NULL) {
        return BSP_ERROR;
    }

    /* 保存配置 */
    bsp_current_config = *config;

    /* 基础硬件初始化 */
    HAL_Init();

    /* 系统时钟配置 */
    bsp_status = BSP_ClockConfig(NULL); /* 使用默认配置 */
    if (bsp_status != BSP_OK) {
        return bsp_status;
    }

    /* MSP初始化 */
    BSP_MspInit();

    // /* 外设初始化 */
    // bsp_status = BSP_PeripheralsInit(config);
    // if (bsp_status != BSP_OK) {
    //     return bsp_status;
    // }

    /* 中断配置 */
    bsp_status = BSP_InterruptConfig();
    if (bsp_status != BSP_OK) {
        return bsp_status;
    }

    /* 标记已初始化 */
    bsp_initialized = 1;

    /* 调用初始化完成回调 */
    BSP_InitCompletedCallback();

    return BSP_OK;
}

/**
 * @brief BSP最小初始化函数
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_MinimalInit(void) {
    BSP_InitTypeDef minimal_config = BSP_INIT_MINIMAL;
    return BSP_Init(&minimal_config);
}

/**
 * @brief BSP去初始化函数
 * @retval BSP_StatusTypeDef 去初始化状态
 */
BSP_StatusTypeDef BSP_DeInit(void) {
    if (!bsp_initialized) {
        return BSP_OK;
    }

    /* MSP去初始化 */
    BSP_MspDeInit();

    HAL_DeInit();

    /* 清除初始化标志 */
    bsp_initialized = 0;

    return BSP_OK;
}

/**
 * @brief BSP系统时钟配置
 * @param clock_config 时钟配置结构体指针
 * @retval BSP_StatusTypeDef 配置状态
 */
BSP_StatusTypeDef BSP_ClockConfig(const BSP_ClockConfigTypeDef* clock_config) {
    /* 暂时返回OK，实际时钟配置将在HAL库集成后实现 */
    /* 这里需要配置：
     * - HSE: 8MHz外部晶振
     * - PLL: 6 * 168 / 2 = 504MHz -> 168MHz系统时钟
     * - AHB: 168MHz
     * - APB1: 42MHz (168/4)
     * - APB2: 84MHz (168/2)
     * - Flash Latency: 5
     */

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        // Error_Handler();
    }

    return BSP_OK;
}

/**
 * @brief BSP外设初始化
 * @param config 初始化配置结构体指针
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_PeripheralsInit(const BSP_InitTypeDef* config) {
    BSP_StatusTypeDef status = BSP_OK;

    /* GPIO初始化（总是需要） */
    if (config->enable_gpio) {
        /* TODO: 调用BSP_GPIO_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* DMA初始化 */
    if (config->enable_dma) {
        /* TODO: 调用BSP_DMA_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* UART初始化 */
    if (config->enable_uart) {
        /* TODO: 调用BSP_UART_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* CAN初始化 */
    if (config->enable_can) {
        /* TODO: 调用BSP_CAN_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* SPI初始化 */
    if (config->enable_spi) {
        /* TODO: 调用BSP_SPI_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* I2C初始化 */
    if (config->enable_i2c) {
        /* TODO: 调用BSP_I2C_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* 定时器初始化 */
    if (config->enable_tim) {
        /* TODO: 调用BSP_TIM_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* ADC初始化 */
    if (config->enable_adc) {
        /* TODO: 调用BSP_ADC_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    /* USB初始化 */
    if (config->enable_usb) {
        /* TODO: 调用BSP_USB_Init() */
        status = BSP_OK; /* 暂时返回OK */
        if (status != BSP_OK) {
            return status;
        }
    }

    return status;
}

/**
 * @brief BSP中断配置
 * @retval BSP_StatusTypeDef 配置状态
 */
BSP_StatusTypeDef BSP_InterruptConfig(void) {
    /* 配置系统中断优先级 */
    /* TODO: 调用HAL_NVIC_SetPriority() */

    /* 配置外设中断优先级 */
    /* TODO: 根据需要配置具体外设的中断优先级 */

    return BSP_OK;
}

/**
 * @brief BSP错误处理函数
 * @param file 文件名
 * @param line 行号
 */
void BSP_Error_Handler(const char* file, uint32_t line) {
    /* 用户可添加自己的实现来报告HAL错误返回状态 */

    /* 禁用所有中断 */
    __disable_irq();

    /* 调用错误回调 */
    BSP_ErrorCallback(BSP_ERROR);

/* 如果定义了调试模式，可以在这里添加调试信息输出 */
#ifdef DEBUG
/* printf("Error occurred in file %s at line %lu\r\n", file, line); */
#endif

    /* 系统复位 */
    NVIC_SystemReset();

    /* 无限循环，防止程序继续执行 */
    while (1) {
        /* 等待复位 */
    }
}

/**
 * @brief BSP断言失败处理函数
 * @param file 文件名
 * @param line 行号
 */
void BSP_Assert_Failed(uint8_t* file, uint32_t line) {
    /* 用户可以添加自己的实现来报告文件名和行号 */
    BSP_Error_Handler((const char*)file, line);
}

/**
 * @brief BSP系统复位函数
 */
void BSP_SystemReset(void) { NVIC_SystemReset(); }

/**
 * @brief BSP获取系统时钟频率
 * @return uint32_t 系统时钟频率(Hz)
 */
uint32_t BSP_GetSystemClockFreq(void) { return HAL_RCC_GetSysClockFreq(); }

/**
 * @brief BSP获取AHB时钟频率
 * @return uint32_t AHB时钟频率(Hz)
 */
uint32_t BSP_GetHCLKFreq(void) { return HAL_RCC_GetHCLKFreq(); }

/**
 * @brief BSP获取APB1时钟频率
 * @return uint32_t APB1时钟频率(Hz)
 */
uint32_t BSP_GetPCLK1Freq(void) { return HAL_RCC_GetPCLK1Freq(); }

/**
 * @brief BSP获取APB2时钟频率
 * @return uint32_t APB2时钟频率(Hz)
 */
uint32_t BSP_GetPCLK2Freq(void) { return HAL_RCC_GetPCLK2Freq(); }

/**
 * @brief BSP延时函数
 * @param delay 延时时间(ms)
 */
void BSP_Delay(uint32_t delay) { HAL_Delay(delay); }

/**
 * @brief BSP微秒延时函数
 * @param delay 延时时间(us)
 */
void BSP_DelayUs(uint32_t delay) {
    /* 使用简单的循环延时 */
    volatile uint32_t count = delay * (SystemCoreClock / 1000000) / 4;
    while (count--) {
        __NOP();
    }
}

/**
 * @brief BSP获取系统滴答计数
 * @return uint32_t 滴答计数
 */
uint32_t BSP_GetTick(void) { return HAL_GetTick(); }

/**
 * @brief BSP进入低功耗模式
 * @param mode 低功耗模式
 */
void BSP_EnterLowPowerMode(uint32_t mode) {
    /* TODO: 实现低功耗模式 */
    switch (mode) {
        case 0:
            /* Sleep Mode */
            break;
        case 1:
            /* Stop Mode */
            break;
        case 2:
            /* Standby Mode */
            break;
        default:
            break;
    }
}

/**
 * @brief BSP退出低功耗模式
 */
void BSP_ExitLowPowerMode(void) {
    /* 重新配置系统时钟 */
    BSP_ClockConfig(NULL);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief MSP初始化函数
 */
static void BSP_MspInit(void) {
    /* MSP初始化代码 */
    /* 这里可以添加MSP级别的初始化代码 */
}

/**
 * @brief MSP去初始化函数
 */
static void BSP_MspDeInit(void) {
    /* MSP去初始化代码 */
    /* 这里可以添加MSP级别的去初始化代码 */
}

/* Weak functions -----------------------------------------------------------*/

/**
 * @brief BSP初始化完成回调函数
 */
__weak void BSP_InitCompletedCallback(void) { /* 用户可重写此函数 */ }

/**
 * @brief BSP错误处理回调函数
 * @param error_code 错误代码
 */
__weak void BSP_ErrorCallback(uint32_t error_code) {
    /* 用户可重写此函数 */
    /* 防止未使用参数警告 */
    (void)error_code;
}