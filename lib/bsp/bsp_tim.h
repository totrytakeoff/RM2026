/**
 * @file bsp_tim.h
 * @brief RM C型板定时器BSP封装头文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件提供定时器的BSP封装接口声明，
 * 为其他BSP模块提供定时器服务。
 */

#ifndef __BSP_TIM_H
#define __BSP_TIM_H

/* Includes ------------------------------------------------------------------*/
#include "bsp_config.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 启动指定定时器
 * @param tim_instance: 定时器实例号 (5)
 * @retval None
 */
void BSP_TIM_Start(uint8_t tim_instance);

/**
 * @brief 停止指定定时器
 * @param tim_instance: 定时器实例号 (5)
 * @retval None
 */
void BSP_TIM_Stop(uint8_t tim_instance);

/**
 * @brief 启动指定定时器的PWM通道
 * @param tim_instance: 定时器实例号 (5)
 * @param channel: PWM通道号 (1-4)
 * @retval None
 */
void BSP_TIM_PWM_Start(uint8_t tim_instance, uint8_t channel);

/**
 * @brief 停止指定定时器的PWM通道
 * @param tim_instance: 定时器实例号 (5)
 * @param channel: PWM通道号 (1-4)
 * @retval None
 */
void BSP_TIM_PWM_Stop(uint8_t tim_instance, uint8_t channel);

/**
 * @brief 设置指定定时器PWM通道的比较值
 * @param tim_instance: 定时器实例号 (5)
 * @param channel: PWM通道号 (1-4)
 * @param compare: 比较值
 * @retval None
 */
void BSP_TIM_SetCompare(uint8_t tim_instance, uint8_t channel, uint32_t compare);

#endif /* __BSP_TIM_H */