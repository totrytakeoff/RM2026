/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
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

/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "pin_map.h"  // 添加pin_map.h以使用蜂鸣器相关宏定义

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

TIM_HandleTypeDef htim4;  // 蜂鸣器使用的TIM4定时器句柄
TIM_HandleTypeDef htim5;

/**
  * @brief TIM4初始化函数 - 用于蜂鸣器PWM控制
  * @param 无
  * @retval 无
  * @note TIM4配置为PWM模式，用于驱动无源蜂鸣器
  */
void MX_TIM4_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  // 定时器基础配置
  htim4.Instance = TIM4;
  
  /* 预分频器配置：
   * 系统时钟频率为168MHz，APB1时钟频率为42MHz
   * TIM4挂载在APB1上，由于APB1预分频器为4，TIM4时钟频率为42MHz * 2 = 84MHz
   * 设置预分频器为83，得到1MHz的计数频率（84MHz / (83+1) = 1MHz）
   */
  htim4.Init.Prescaler = 83;  // 84MHz / (83+1) = 1MHz
  
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;  // 向上计数模式
  
  /* 自动重装载值配置：
   * 根据蜂鸣器默认频率4kHz计算ARR值
   * ARR = 计数频率 / 目标频率 - 1 = 1MHz / 4kHz - 1 = 250 - 1 = 249
   */
  htim4.Init.Period = (1000000 / BUZZER_DEFAULT_FREQ) - 1;  // 根据默认频率计算ARR值
  
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  // 时钟不分频
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;  // 使能自动重装载预加载
  
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  
  // 时钟源配置
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;  // 使用内部时钟
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  // 初始化PWM模式
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  
  // 主模式配置（不使用主从模式）
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  // PWM通道配置
  sConfigOC.OCMode = TIM_OCMODE_PWM1;  // PWM模式1
  /* 脉冲宽度配置：
   * 设置为ARR值的一半，即50%占空比
   * 对于蜂鸣器，50%占空比可以提供最佳的声音输出
   */
  sConfigOC.Pulse = (1000000 / BUZZER_DEFAULT_FREQ) / 2;  // 50%占空比
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;  // 输出极性为高
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;  // 禁用快速模式
  
  // 配置TIM4通道3（蜂鸣器通道）
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  
  // 配置完通道后，初始化GPIO
  HAL_TIM_MspPostInit(&htim4);
}

/* TIM5 init function */
void MX_TIM5_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 65535;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief 定时器MSP初始化函数
  * @param tim_baseHandle: 定时器句柄
  * @retval 无
  * @note 配置定时器时钟和基础硬件
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
  if(tim_baseHandle->Instance==TIM4)
  {
  /* USER CODE BEGIN TIM4_MspInit 0 */

  /* USER CODE END TIM4_MspInit 0 */
    /* TIM4时钟使能 */
    __HAL_RCC_TIM4_CLK_ENABLE();
  /* USER CODE BEGIN TIM4_MspInit 1 */

  /* USER CODE END TIM4_MspInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspInit 0 */

  /* USER CODE END TIM5_MspInit 0 */
    /* TIM5时钟使能 */
    __HAL_RCC_TIM5_CLK_ENABLE();
  /* USER CODE BEGIN TIM5_MspInit 1 */

  /* USER CODE END TIM5_MspInit 1 */
  }
}

/**
  * @brief 定时器MSP后初始化函数
  * @param timHandle: 定时器句柄
  * @retval 无
  * @note 配置定时器相关的GPIO引脚
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if(timHandle->Instance==TIM4)
  {
  /* USER CODE BEGIN TIM4_MspPostInit 0 */

  /* USER CODE END TIM4_MspPostInit 0 */
  
    /* 使能GPIOD时钟 */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**TIM4 GPIO配置
    PD14     ------> TIM4_CH3  (蜂鸣器控制引脚)
    */
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // 复用推挽输出模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // 上拉电阻
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;  // 复用功能为TIM4
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM4_MspPostInit 1 */

  /* USER CODE END TIM4_MspPostInit 1 */
  }
  else if(timHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspPostInit 0 */

  /* USER CODE END TIM5_MspPostInit 0 */
  
    __HAL_RCC_GPIOH_CLK_ENABLE();
    /**TIM5 GPIO配置    
    PH12     ------> TIM5_CH3
    PH11     ------> TIM5_CH2
    PH10     ------> TIM5_CH1 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_11|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM5_MspPostInit 1 */

  /* USER CODE END TIM5_MspPostInit 1 */
  }

}

/**
  * @brief 定时器MSP去初始化函数
  * @param tim_baseHandle: 定时器句柄
  * @retval 无
  * @note 禁用定时器时钟
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{
  if(tim_baseHandle->Instance==TIM4)
  {
  /* USER CODE BEGIN TIM4_MspDeInit 0 */

  /* USER CODE END TIM4_MspDeInit 0 */
    /* 外设时钟禁用 */
    __HAL_RCC_TIM4_CLK_DISABLE();
  /* USER CODE BEGIN TIM4_MspDeInit 1 */

  /* USER CODE END TIM4_MspDeInit 1 */
  }
  else if(tim_baseHandle->Instance==TIM5)
  {
  /* USER CODE BEGIN TIM5_MspDeInit 0 */

  /* USER CODE END TIM5_MspDeInit 0 */
    /* 外设时钟禁用 */
    __HAL_RCC_TIM5_CLK_DISABLE();
  /* USER CODE BEGIN TIM5_MspDeInit 1 */

  /* USER CODE END TIM5_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/