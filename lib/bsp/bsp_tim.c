/**
 * @file bsp_tim.c
 * @brief RM C型板定时器BSP封装实现文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件提供定时器的BSP封装实现，
 * 为其他BSP模块提供定时器服务。
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_tim.h"
#include "bsp_config.h"
#include "../../include/main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern TIM_HandleTypeDef htim5;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 启动指定定时器
 * @param tim_instance: 定时器实例号 (1-8)
 * @retval None
 */
void BSP_TIM_Start(uint8_t tim_instance)
{
    switch (tim_instance) {
        case 5:
            HAL_TIM_Base_Start(&htim5);
            break;
        default:
            break;
    }
}

/**
 * @brief 停止指定定时器
 * @param tim_instance: 定时器实例号 (1-8)
 * @retval None
 */
void BSP_TIM_Stop(uint8_t tim_instance)
{
    switch (tim_instance) {
        case 5:
            HAL_TIM_Base_Stop(&htim5);
            break;
        default:
            break;
    }
}

/**
 * @brief 启动指定定时器的PWM通道
 * @param tim_instance: 定时器实例号 (1-8)
 * @param channel: PWM通道号 (1-4)
 * @retval None
 */
void BSP_TIM_PWM_Start(uint8_t tim_instance, uint8_t channel)
{
    switch (tim_instance) {
        case 5:
            switch (channel) {
                case 1:
                    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
                    break;
                case 2:
                    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
                    break;
                case 3:
                    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
                    break;
                case 4:
                    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief 停止指定定时器的PWM通道
 * @param tim_instance: 定时器实例号 (1-8)
 * @param channel: PWM通道号 (1-4)
 * @retval None
 */
void BSP_TIM_PWM_Stop(uint8_t tim_instance, uint8_t channel)
{
    switch (tim_instance) {
        case 5:
            switch (channel) {
                case 1:
                    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_1);
                    break;
                case 2:
                    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
                    break;
                case 3:
                    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3);
                    break;
                case 4:
                    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_4);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief 设置指定定时器PWM通道的比较值
 * @param tim_instance: 定时器实例号 (1-8)
 * @param channel: PWM通道号 (1-4)
 * @param compare: 比较值
 * @retval None
 */
void BSP_TIM_SetCompare(uint8_t tim_instance, uint8_t channel, uint32_t compare)
{
    switch (tim_instance) {
        case 5:
            switch (channel) {
                case 1:
                    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_1, compare);
                    break;
                case 2:
                    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_2, compare);
                    break;
                case 3:
                    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_3, compare);
                    break;
                case 4:
                    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_4, compare);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}



/* Private functions ---------------------------------------------------------*/