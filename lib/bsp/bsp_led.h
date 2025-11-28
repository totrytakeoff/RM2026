/**
 * @file bsp_led.h
 * @brief RM C型板RGB LED BSP封装头文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件提供RGB LED的BSP封装接口，
 * 支持基本颜色显示、呼吸灯、流水灯、渐变等功能。
 */

#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bsp_board.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief RGB颜色结构体
 */
typedef struct {
    uint8_t red;    /**< 红色分量 (0-255) */
    uint8_t green;  /**< 绿色分量 (0-255) */
    uint8_t blue;   /**< 蓝色分量 (0-255) */
} BSP_RGB_ColorTypeDef;

/**
 * @brief aRGB颜色结构体（带透明度）
 */
typedef struct {
    uint8_t alpha;  /**< 透明度分量 (0-255) */
    uint8_t red;    /**< 红色分量 (0-255) */
    uint8_t green;  /**< 绿色分量 (0-255) */
    uint8_t blue;   /**< 蓝色分量 (0-255) */
} BSP_aRGB_ColorTypeDef;

/**
 * @brief LED效果枚举
 */
typedef enum {
    BSP_LED_EFFECT_NONE = 0,      /**< 无效果 */
    BSP_LED_EFFECT_BREATHING,     /**< 呼吸灯效果 */
    BSP_LED_EFFECT_FLOW,          /**< 流水灯效果 */
    BSP_LED_EFFECT_GRADIENT,      /**< 渐变效果 */
    BSP_LED_EFFECT_BLINK          /**< 闪烁效果 */
} BSP_LED_EffectTypeDef;

/**
 * @brief LED配置结构体
 */
typedef struct {
    BSP_RGB_ColorTypeDef color;           /**< 当前颜色 */
    BSP_LED_EffectTypeDef effect;         /**< 当前效果 */
    uint16_t period;                      /**< 效果周期(ms) */
    uint8_t enabled;                      /**< 使能状态 */
    uint32_t last_update_time;            /**< 上次更新时间 */
    uint16_t current_step;                /**< 当前步骤 */
} BSP_LED_ConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
/* 预定义颜色 */
#define BSP_LED_COLOR_RED         ((uint32_t)0xFFFF0000)  /**< 红色 */
#define BSP_LED_COLOR_GREEN       ((uint32_t)0xFF00FF00)  /**< 绿色 */
#define BSP_LED_COLOR_BLUE        ((uint32_t)0xFF0000FF)  /**< 蓝色 */
#define BSP_LED_COLOR_YELLOW      ((uint32_t)0xFFFFFF00)  /**< 黄色 */
#define BSP_LED_COLOR_CYAN        ((uint32_t)0xFF00FFFF)  /**< 青色 */
#define BSP_LED_COLOR_MAGENTA     ((uint32_t)0xFFFF00FF)  /**< 洋红色 */
#define BSP_LED_COLOR_WHITE       ((uint32_t)0xFFFFFFFF)  /**< 白色 */
#define BSP_LED_COLOR_BLACK       ((uint32_t)0xFF000000)  /**< 黑色(关闭) */
#define BSP_LED_COLOR_ORANGE      ((uint32_t)0xFFFFA500)  /**< 橙色 */
#define BSP_LED_COLOR_PURPLE      ((uint32_t)0xFF800080)  /**< 紫色 */

/* LED引脚定义 */
#define BSP_LED_RED_PIN     GPIO_PIN_12    /**< 红色LED引脚 */
#define BSP_LED_GREEN_PIN   GPIO_PIN_11    /**< 绿色LED引脚 */
#define BSP_LED_BLUE_PIN    GPIO_PIN_10    /**< 蓝色LED引脚 */
#define BSP_LED_GPIO_PORT   GPIOH          /**< LED GPIO端口 */

/* PWM配置 - 与硬件连接匹配 */
#define BSP_LED_TIM_HANDLE  htim5          /**< LED定时器句柄 */
#define BSP_LED_TIM_CHANNEL_RED   TIM_CHANNEL_3  /**< 红色PWM通道 PH12 */
#define BSP_LED_TIM_CHANNEL_GREEN TIM_CHANNEL_2  /**< 绿色PWM通道 PH11 */
#define BSP_LED_TIM_CHANNEL_BLUE  TIM_CHANNEL_1  /**< 蓝色PWM通道 PH10 */
#define BSP_LED_PWM_PERIOD    65535          /**< PWM周期 */
#define BSP_LED_PWM_PRESCALER 0              /**< PWM预分频器 */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief BSP LED初始化函数
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_LED_Init(void);

/**
 * @brief BSP LED去初始化函数
 * @retval BSP_StatusTypeDef 去初始化状态
 */
BSP_StatusTypeDef BSP_LED_DeInit(void);

/**
 * @brief 设置RGB颜色（aRGB格式）
 * @param aRGB: 0xaaRRGGBB, 'aa'是透明度, 'RR'是红色, 'GG'是绿色, 'BB'是蓝色
 * @retval None
 */
void BSP_LED_SetColor(uint32_t aRGB);

/**
 * @brief 设置RGB颜色（分量格式）
 * @param red: 红色分量 (0-255)
 * @param green: 绿色分量 (0-255)
 * @param blue: 蓝色分量 (0-255)
 * @retval None
 */
void BSP_LED_SetRGB(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 设置aRGB颜色（分量格式）
 * @param alpha: 透明度分量 (0-255)
 * @param red: 红色分量 (0-255)
 * @param green: 绿色分量 (0-255)
 * @param blue: 蓝色分量 (0-255)
 * @retval None
 */
void BSP_LED_SetaRGB(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 设置预定义颜色
 * @param color: 预定义颜色值
 * @retval None
 */
void BSP_LED_SetPresetColor(uint32_t color);

/**
 * @brief 关闭LED
 * @retval None
 */
void BSP_LED_Off(void);

/**
 * @brief 开启LED（恢复上次颜色）
 * @retval None
 */
void BSP_LED_On(void);

/**
 * @brief 设置呼吸灯效果
 * @param color: 呼吸灯颜色
 * @param period: 呼吸周期(ms)
 * @retval None
 */
void BSP_LED_SetBreathing(uint32_t color, uint16_t period);

/**
 * @brief 设置流水灯效果
 * @param colors: 颜色数组
 * @param count: 颜色数量
 * @param period: 每个颜色的显示时间(ms)
 * @retval None
 */
void BSP_LED_SetFlow(const uint32_t *colors, uint8_t count, uint16_t period);

/**
 * @brief 设置渐变效果
 * @param start_color: 起始颜色
 * @param end_color: 结束颜色
 * @param period: 渐变时间(ms)
 * @retval None
 */
void BSP_LED_SetGradient(uint32_t start_color, uint32_t end_color, uint16_t period);

/**
 * @brief 设置闪烁效果
 * @param color: 闪烁颜色
 * @param period: 闪烁周期(ms)
 * @param duty_cycle: 占空比 (0-100)
 * @retval None
 */
void BSP_LED_SetBlink(uint32_t color, uint16_t period, uint8_t duty_cycle);

/**
 * @brief 停止所有效果
 * @retval None
 */
void BSP_LED_StopEffect(void);

/**
 * @brief LED效果更新函数（需要在主循环中调用）
 * @retval None
 */
void BSP_LED_Update(void);

/**
 * @brief 获取LED调试信息
 * @param info: 调试信息字符串缓冲区
 * @param max_len: 缓冲区最大长度
 * @retval None
 */
void BSP_LED_GetDebugInfo(char *info, uint16_t max_len);

/**
 * @brief 获取当前LED配置
 * @retval BSP_LED_ConfigTypeDef* 当前配置指针
 */
BSP_LED_ConfigTypeDef* BSP_LED_GetConfig(void);

/**
 * @brief 设置LED亮度
 * @param brightness: 亮度值 (0-255)
 * @retval None
 */
void BSP_LED_SetBrightness(uint8_t brightness);

/**
 * @brief 获取LED亮度
 * @retval uint8_t 当前亮度值
 */
uint8_t BSP_LED_GetBrightness(void);

/* 高级功能函数 -------------------------------------------------------------*/

/**
 * @brief 颜色混合
 * @param color1: 颜色1
 * @param color2: 颜色2
 * @param ratio: 混合比例 (0-255, 0表示全color1, 255表示全color2)
 * @retval uint32_t 混合后的颜色
 */
uint32_t BSP_LED_MixColor(uint32_t color1, uint32_t color2, uint8_t ratio);

/**
 * @brief 颜色淡入
 * @param color: 目标颜色
 * @param duration: 淡入时间(ms)
 * @retval None
 */
void BSP_LED_FadeIn(uint32_t color, uint16_t duration);

/**
 * @brief 颜色淡出
 * @param duration: 淡出时间(ms)
 * @retval None
 */
void BSP_LED_FadeOut(uint16_t duration);

/**
 * @brief 彩虹循环效果
 * @param period: 完整循环时间(ms)
 * @retval None
 */
void BSP_LED_Rainbow(uint16_t period);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */