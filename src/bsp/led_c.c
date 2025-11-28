/**
 * @file    led_c_clean.c
 * @brief   C风格LED封装实现 - 清洁版本
 * @author  myself
 * @date    2025/11/27
 */

#include "led_c.h"
#include "main.h"
#include "hal/tim.h"
#include <math.h>

// 外部定时器句柄
extern TIM_HandleTypeDef htim5;

// 静态变量用于状态管理
static uint8_t current_r = 0;
static uint8_t current_g = 0;
static uint8_t current_b = 0;
static uint8_t current_alpha = 255;
static uint8_t global_brightness = 255;

// 呼吸灯相关变量
static uint8_t breathing_enable = 0;
static uint8_t breathing_period = 2;
static uint8_t breathing_interval = 20;
static uint32_t breathing_last_update = 0;
static uint16_t breathing_step = 0;

// HSV到RGB转换
void LED_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float h_f = h / 360.0f;
    float s_f = s / 255.0f;
    float v_f = v / 255.0f;

    float c = v_f * s_f;
    float x = c * (1.0f - fabsf(fmodf(h_f * 6.0f, 2.0f) - 1.0f));
    float m = v_f - c;

    float r_f, g_f, b_f;

    if (h_f < 1.0f/6.0f) {
        r_f = c; g_f = x; b_f = 0;
    } else if (h_f < 2.0f/6.0f) {
        r_f = x; g_f = c; b_f = 0;
    } else if (h_f < 3.0f/6.0f) {
        r_f = 0; g_f = c; b_f = x;
    } else if (h_f < 4.0f/6.0f) {
        r_f = 0; g_f = x; b_f = c;
    } else if (h_f < 5.0f/6.0f) {
        r_f = x; g_f = 0; b_f = c;
    } else {
        r_f = c; g_f = 0; b_f = x;
    }

    *r = (uint8_t)((r_f + m) * 255);
    *g = (uint8_t)((g_f + m) * 255);
    *b = (uint8_t)((b_f + m) * 255);
}

// 初始化LED系统
void LED_Init(void) {
    // 启动PWM
    HAL_TIM_Base_Start(&htim5);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    // 初始状态：关闭所有LED
    LED_Off();

    // 初始化呼吸灯变量
    breathing_enable = 0;
    breathing_last_update = HAL_GetTick();
    breathing_step = 0;
}

// 关闭所有LED
void LED_Off(void) {
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 0);  // 蓝色
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 0);  // 绿色
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);  // 红色

    current_r = 0;
    current_g = 0;
    current_b = 0;
}

// 直接设置RGB值
void LED_SetRGB(uint8_t r, uint8_t g, uint8_t b) {
    current_r = r;
    current_g = g;
    current_b = b;

    LED_SetARGB(current_alpha, r, g, b);
}

// 设置ARGB值
void LED_SetARGB(uint8_t alpha, uint8_t r, uint8_t g, uint8_t b) {
    current_alpha = alpha;

    // 按照官方demo的方式计算PWM值
    uint16_t red_pwm = r * alpha;      // 最大值：255 * 255 = 65025
    uint16_t green_pwm = g * alpha;    // 虽然不是65535，但这是官方demo的方式
    uint16_t blue_pwm = b * alpha;

    // 应用全局亮度
    if (global_brightness < 255) {
        red_pwm = (red_pwm * global_brightness) / 255;
        green_pwm = (green_pwm * global_brightness) / 255;
        blue_pwm = (blue_pwm * global_brightness) / 255;
    }

    // 设置PWM值
    LED_SetPWM(red_pwm, green_pwm, blue_pwm);
}

// 设置预定义颜色
void LED_SetColor(Color_TypeDef color) {
    LED_SetARGB32((uint32_t)color);
}

// 设置32位ARGB值
void LED_SetARGB32(uint32_t argb) {
    uint8_t alpha = (argb & 0xFF000000) >> 24;
    uint8_t red = (argb & 0x00FF0000) >> 16;
    uint8_t green = (argb & 0x0000FF00) >> 8;
    uint8_t blue = (argb & 0x000000FF);

    LED_SetARGB(alpha, red, green, blue);
}

// 设置HSV颜色
void LED_SetHSV(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    LED_HSVtoRGB(h, s, v, &r, &g, &b);
    LED_SetRGB(r, g, b);
}

// 设置全局亮度
void LED_SetBrightness(uint8_t brightness) {
    global_brightness = brightness;
    // 重新应用当前颜色
    LED_SetARGB(current_alpha, current_r, current_g, current_b);
}

// 直接设置PWM值（底层函数）
void LED_SetPWM(uint16_t red_pwm, uint16_t green_pwm, uint16_t blue_pwm) {
    // 限制PWM值范围
    if (red_pwm > 65535) red_pwm = 65535;
    if (green_pwm > 65535) green_pwm = 65535;
    if (blue_pwm > 65535) blue_pwm = 65535;

    // 直接设置PWM值 - 这是已知能工作的方式
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, blue_pwm);  // 蓝色 - CH1
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, green_pwm); // 绿色 - CH2
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, red_pwm);   // 红色 - CH3
}

// 呼吸灯效果
void LED_BreathingLight(uint8_t period, uint8_t interval, uint8_t enable) {
    breathing_enable = enable;
    breathing_period = (period > 0) ? period : 1;
    breathing_interval = (interval > 0) ? interval : 20;

    if (!enable) {
        // 停用呼吸灯，恢复正常颜色
        LED_SetARGB(current_alpha, current_r, current_g, current_b);
    }
}

// LED闪烁
void LED_Blink(uint8_t times, uint16_t on_ms, uint16_t off_ms) {
    for (uint8_t i = 0; i < times; i++) {
        LED_SetARGB(current_alpha, current_r, current_g, current_b);  // 亮起
        HAL_Delay(on_ms);
        LED_Off();  // 熄灭
        if (i < times - 1) {
            HAL_Delay(off_ms);
        }
    }
}

// 获取当前RGB值
void LED_GetRGB(uint8_t *r, uint8_t *g, uint8_t *b) {
    if (r) *r = current_r;
    if (g) *g = current_g;
    if (b) *b = current_b;
}

// 呼吸灯处理函数 - 应该在主循环中定期调用
void LED_BreathingHandler(void) {
    if (!breathing_enable) {
        return;
    }

    uint32_t current_time = HAL_GetTick();

    if (current_time - breathing_last_update >= breathing_interval) {
        breathing_last_update = current_time;

        // 计算呼吸灯相位 (0-2π)
        float phase = (2.0f * 3.1415926f * breathing_step) / (breathing_period * 1000 / breathing_interval);

        // 计算亮度 (0-1)
        float brightness = (sin(phase) + 1.0f) / 2.0f;

        // 应用呼吸灯效果到当前颜色
        uint8_t breath_r = (uint8_t)(current_r * brightness);
        uint8_t breath_g = (uint8_t)(current_g * brightness);
        uint8_t breath_b = (uint8_t)(current_b * brightness);

        LED_SetARGB(current_alpha, breath_r, breath_g, breath_b);

        // 更新步骤
        breathing_step++;
        uint16_t steps_per_period = (breathing_period * 1000) / breathing_interval;
        if (breathing_step >= steps_per_period) {
            breathing_step = 0;
        }
    }
}