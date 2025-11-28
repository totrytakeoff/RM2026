/**
 * @file    led_c.h
 * @brief   C风格LED封装头文件
 * @author  myself
 * @date    2025/11/27
 */

#ifndef LED_C_H
#define LED_C_H

#include "stdint.h"

// 颜色定义
#define LED_COLOR_RED     ((uint32_t)0xFFFF0000)     /**< 红色 */
#define LED_COLOR_GREEN   ((uint32_t)0xFF00FF00)   /**< 绿色 */
#define LED_COLOR_BLUE    ((uint32_t)0xFF0000FF)    /**< 蓝色 */
#define LED_COLOR_YELLOW  ((uint32_t)0xFFFFFF00)  /**< 黄色 */
#define LED_COLOR_CYAN    ((uint32_t)0xFF00FFFF)    /**< 青色 */
#define LED_COLOR_MAGENTA ((uint32_t)0xFFFF00FF) /**< 洋红色 */
#define LED_COLOR_WHITE   ((uint32_t)0xFFFFFFFF)   /**< 白色 */
#define LED_COLOR_BLACK   ((uint32_t)0xFF000000)   /**< 黑色(关闭) */
#define LED_COLOR_ORANGE  ((uint32_t)0xFFFFA500)  /**< 橙色 */
#define LED_COLOR_PURPLE  ((uint32_t)0xFF800080)  /**< 紫色 */

// 颜色枚举
typedef enum {
    COLOR_RED = LED_COLOR_RED,
    COLOR_ORANGE = LED_COLOR_ORANGE,
    COLOR_YELLOW = LED_COLOR_YELLOW,
    COLOR_GREEN = LED_COLOR_GREEN,
    COLOR_CYAN = LED_COLOR_CYAN,
    COLOR_BLUE = LED_COLOR_BLUE,
    COLOR_PURPLE = LED_COLOR_PURPLE,
    COLOR_WHITE = LED_COLOR_WHITE,
    COLOR_BLACK = LED_COLOR_BLACK,
} Color_TypeDef;

// RGB结构体
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_TypeDef;

// HSV结构体
typedef struct {
    uint16_t h;  // 色相 0-360
    uint8_t s;    // 饱和度 0-255
    uint8_t v;    // 明度 0-255
} HSV_TypeDef;

// ARGB结构体
typedef struct {
    uint8_t alpha;  // 透明度 0-255
    uint8_t red;    // 红色分量 0-255
    uint8_t green;  // 绿色分量 0-255
    uint8_t blue;   // 蓝色分量 0-255
} ARGB_TypeDef;

/**
 * @brief 初始化LED系统
 * @note  必须在使用LED功能前调用
 */
void LED_Init(void);

/**
 * @brief 关闭所有LED
 */
void LED_Off(void);

/**
 * @brief 直接设置RGB值
 * @param r 红色分量 0-255
 * @param g 绿色分量 0-255
 * @param b 蓝色分量 0-255
 */
void LED_SetRGB(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置ARGB值（带透明度）
 * @param alpha 透明度 0-255
 * @param r 红色分量 0-255
 * @param g 绿色分量 0-255
 * @param b 蓝色分量 0-255
 */
void LED_SetARGB(uint8_t alpha, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置预定义颜色
 * @param color 颜色枚举值
 */
void LED_SetColor(Color_TypeDef color);

/**
 * @brief 设置32位ARGB值
 * @param argb 0xAARRGGBB格式颜色值
 */
void LED_SetARGB32(uint32_t argb);

/**
 * @brief 设置HSV颜色
 * @param h 色相 0-360
 * @param s 饱和度 0-255
 * @param v 明度 0-255
 */
void LED_SetHSV(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief 设置LED亮度
 * @param brightness 亮度 0-255
 */
void LED_SetBrightness(uint8_t brightness);

/**
 * @brief 设置单个LED的PWM值（底层函数）
 * @param red_pwm 红色PWM值 0-65535
 * @param green_pwm 绿色PWM值 0-65535
 * @param blue_pwm 蓝色PWM值 0-65535
 */
void LED_SetPWM(uint16_t red_pwm, uint16_t green_pwm, uint16_t blue_pwm);

/**
 * @brief 呼吸灯效果
 * @param period 呼吸周期(秒) 1-10
 * @param interval 更新间隔(毫秒) 10-100
 * @param enable 是否启用呼吸灯效果
 */
void LED_BreathingLight(uint8_t period, uint8_t interval, uint8_t enable);

/**
 * @brief LED闪烁
 * @param times 闪烁次数
 * @param on_ms 亮起时间(毫秒)
 * @param off_ms 熄灭时间(毫秒)
 */
void LED_Blink(uint8_t times, uint16_t on_ms, uint16_t off_ms);

/**
 * @brief 获取当前RGB值
 * @param r 指向红色分量的指针
 * @param g 指向绿色分量的指针
 * @param b 指向蓝色分量的指针
 */
void LED_GetRGB(uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief 转换HSV到RGB
 * @param h 色相 0-360
 * @param s 饱和度 0-255
 * @param v 明度 0-255
 * @param r 输出红色分量 0-255
 * @param g 输出绿色分量 0-255
 * @param b 输出蓝色分量 0-255
 */
void LED_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief 呼吸灯处理函数
 * @note  应该在主循环中定期调用，用于处理呼吸灯效果
 */
void LED_BreathingHandler(void);

#endif // LED_C_H