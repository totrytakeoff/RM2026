/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body - 使用BSP封装版本
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "bsp/bsp.h"  // 包含所有BSP接口

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes - 不再需要包含HAL库文件，BSP已封装 */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  /* 注意：以下HAL相关初始化已移至BSP_Init()中 */

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  /* 注意：系统时钟配置已移至BSP_Init()中 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  // 使用BSP进行系统初始化 - 一行代码完成所有硬件初始化！
  if (BSP_Init() != BSP_OK) {
    // 初始化失败，LED闪烁红色
    while (1) {
      BSP_LED_RED();
      BSP_DelayMs(100);
      BSP_LED_OFF();
      BSP_DelayMs(100);
    }
  }

  // 显示初始化成功 - 绿色闪烁3次
  for (int i = 0; i < 3; i++) {
    BSP_LED_GREEN();
    BSP_DelayMs(200);
    BSP_LED_OFF();
    BSP_DelayMs(200);
  }

  // 主循环变量
  uint32_t demo_counter = 0;
  uint8_t current_demo = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    demo_counter++;


    if (demo_counter >= 10) {
      demo_counter = 0;
      current_demo = (current_demo + 1) % 5;
      BSP_LED_StopEffects(); // 停止当前效果
    }

    switch (current_demo) {
      case 0:
        // 演示1: GPIO模式颜色循环
        BSP_LED_SetMode(BSP_LED_MODE_GPIO);
        if (demo_counter % 200 == 0) { // 每2秒切换
          static uint8_t color_step = 0;
          color_step = (color_step + 1) % 8;
          switch (color_step) {
            case 0: BSP_LED_RED(); break;
            case 1: BSP_LED_GREEN(); break;
            case 2: BSP_LED_BLUE(); break;
            case 3: BSP_LED_YELLOW(); break;
            case 4: BSP_LED_CYAN(); break;
            case 5: BSP_LED_MAGENTA(); break;
            case 6: BSP_LED_WHITE(); break;
            case 7: BSP_LED_OFF(); break;
          }
        }
        break;

      case 1:
        // 演示2: PWM模式亮度调节
        BSP_LED_SetMode(BSP_LED_MODE_PWM);
        {
          static uint8_t brightness = 0;
          static int8_t brightness_dir = 1;

          if (demo_counter % 10 == 0) { // 每100ms更新
            brightness += brightness_dir * 5;
            if (brightness >= 255) {
              brightness = 255;
              brightness_dir = -1;
            } else if (brightness == 0) {
              brightness = 0;
              brightness_dir = 1;
            }
          }

          BSP_LED_SetBrightness(brightness);
          BSP_LED_BLUE();
        }
        break;

      case 2:
        // 演示3: 呼吸灯效果
        BSP_LED_SetMode(BSP_LED_MODE_PWM);
        BSP_LED_Breathing(255, 100, 50, 20); // 红绿蓝, 速度
        break;

      case 3:
        // 演示4: 彩虹循环
        BSP_LED_SetMode(BSP_LED_MODE_PWM);
        BSP_LED_Rainbow(30); // 速度
        break;

      case 4:
        // 演示5: aRGB格式显示
        BSP_LED_SetMode(BSP_LED_MODE_PWM);
        {
          static uint8_t argb_step = 0;
          if (demo_counter % 100 == 0) { // 每1秒切换
            argb_step = (argb_step + 1) % 6;
            switch (argb_step) {
              case 0: BSP_LED_aRGBShow(0xFFFF0000); break; // 红色
              case 1: BSP_LED_aRGBShow(0xFF00FF00); break; // 绿色
              case 2: BSP_LED_aRGBShow(0xFF0000FF); break; // 蓝色
              case 3: BSP_LED_aRGBShow(0xFFFFFF00); break; // 黄色
              case 4: BSP_LED_aRGBShow(0xFF00FFFF); break; // 青色
              case 5: BSP_LED_aRGBShow(0xFFFF00FF); break; // 洋红色
            }
          }
        }
        break;

      default:
        current_demo = 0;
        break;
    }

    // 检查用户按键 - 使用BSP接口
    if (BSP_KEY_PRESSED()) {
      // 按键按下时显示白色
      BSP_LED_SetMode(BSP_LED_MODE_PWM);
      BSP_LED_WHITE();
      BSP_DelayMs(500);
    }

    // 使用BSP延时函数
    BSP_DelayMs(10);

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* 注意：Error_Handler已通过BSP宏自动定义，无需手动实现
   #define Error_Handler() BSP_Standard_Error_Handler()
   错误处理逻辑已封装在BSP_Error_Handler()中 */

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/