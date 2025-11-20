/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : official_style_main.c
  * @brief          : 完全基于官方demo风格的USB测试程序
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* 外部变量声明 */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Private includes ----------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* 模拟官方demo的全局变量 */
static uint32_t test_counter = 0;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* 完全按照官方demo的USB初始化方式 - 加入启动延迟确保时钟稳定 */
  HAL_Delay(100);  // 等待系统稳定
  MX_USB_DEVICE_Init();
  HAL_Delay(100);  // 等待USB设备初始化完成

  /* 模拟官方的test_task逻辑 - 不启动USB接收 */
  while (1)
  {
    /* 模拟FreeRTOS的osDelay(1) - 让出CPU时间 */
    HAL_Delay(1);

    /* 模拟官方的USB任务逻辑 - 定期发送状态信息 */
    test_counter++;
    if (test_counter >= 2000) {  // 每2秒
      test_counter = 0;

      /* 模拟官方usb_task的数据发送 */
      const char* status_msg = "USB Status: OK\r\n";
      CDC_Transmit_FS((uint8_t*)status_msg, strlen(status_msg));
    }
  }
}

/**
  * @brief System Clock Configuration - 完全复制官方配置
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB busses clocks */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  /* PLL配置 (针对 12MHz HSE):
   * PLLM = 6:    12MHz / 6 = 2MHz (VCO输入)
   * PLLN = 168:  2MHz * 168 = 336MHz (VCO输出)
   * PLLP = 2:    336MHz / 2 = 168MHz (系统时钟)
   * PLLQ = 7:    336MHz / 7 = 48MHz (USB时钟) ✓
   */
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB busses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** 完全按照官方demo配置RTC时钟 */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV30;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* 按照官方demo的错误处理 - 无限循环 */
  while (1)
  {
    /* 快速闪烁表示错误 */
    HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
    HAL_Delay(100);
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* 官方demo的断言处理 */
}
#endif /* USE_FULL_ASSERT */