/******************************************************************************
 *
 * @file       led.cpp
 * @brief      通用led封装类实现
 *
 * @author     myself
 * @date       2025/11/27
 * !!! 注意TIM_CHANNEL_1 的定义通常为 0x00000000U , channel_ != 0 会导致channel_1(蓝) 无法工作
 *****************************************************************************/

#include <math.h>
#include "led.hpp"
#include "main.h"

// 外部定时器句柄声明
extern TIM_HandleTypeDef htim5;

// 静态变量用于呼吸灯效果
static uint32_t system_tick = 0;

// 获取系统时间的简单实现（实际项目中应该使用HAL_GetTick()）
static uint32_t get_system_tick() { return HAL_GetTick(); }

// HSV转RGB的辅助函数
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float h_f = h / 360.0f;
    float s_f = s / 255.0f;
    float v_f = v / 255.0f;

    float c = v_f * s_f;
    float x = c * (1.0f - fabsf(fmodf(h_f * 6.0f, 2.0f) - 1.0f));
    float m = v_f - c;

    float r_f, g_f, b_f;

    if (h_f < 1.0f / 6.0f) {
        r_f = c;
        g_f = x;
        b_f = 0;
    } else if (h_f < 2.0f / 6.0f) {
        r_f = x;
        g_f = c;
        b_f = 0;
    } else if (h_f < 3.0f / 6.0f) {
        r_f = 0;
        g_f = c;
        b_f = x;
    } else if (h_f < 4.0f / 6.0f) {
        r_f = 0;
        g_f = x;
        b_f = c;
    } else if (h_f < 5.0f / 6.0f) {
        r_f = x;
        g_f = 0;
        b_f = c;
    } else {
        r_f = c;
        g_f = 0;
        b_f = x;
    }

    r = static_cast<uint8_t>((r_f + m) * 255);
    g = static_cast<uint8_t>((g_f + m) * 255);
    b = static_cast<uint8_t>((b_f + m) * 255);
}

// LED类实现
LED::LED(GPIO_TypeDef* port, uint16_t pin) : port_(port), pin_(pin), tim_(nullptr), channel_(0) {}

LED::LED(GPIO_TypeDef* port, uint16_t pin, TIM_HandleTypeDef* tim, uint32_t channel)
        : port_(port), pin_(pin), tim_(tim), channel_(channel) {
    // 不在构造函数中设置PWM值，因为定时器可能还没初始化
}

void LED::on() {
    if (tim_ != nullptr) {
        // PWM模式，设置最大占空比
        __HAL_TIM_SET_COMPARE(tim_, channel_, 65535);
        // 确保PWM已启动
        HAL_TIM_PWM_Start(tim_, channel_);
    } else {
        // GPIO模式，设置高电平
        HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
    }
}

void LED::off() {
    if (tim_ != nullptr) {
        // PWM模式，设置最小占空比
        __HAL_TIM_SET_COMPARE(tim_, channel_, 0);
    } else {
        // GPIO模式，设置低电平
        HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
    }
}

void LED::toggle(uint8_t times, uint16_t delay_ms) {
    for (uint8_t i = 0; i < times; i++) {
        on();
        HAL_Delay(delay_ms);
        off();
        if (i < times - 1) {
            HAL_Delay(delay_ms);
        }
    }
}

bool LED::setBrightness(uint8_t brightness) {
    if (tim_ != nullptr) {
        // PWM模式，根据亮度设置占空比
        uint16_t pwm_value = (brightness * 65535) / 255;
        __HAL_TIM_SET_COMPARE(tim_, channel_, pwm_value);
        return true;
    }
    return false;  // GPIO模式不支持亮度调节
}

bool LED::setPWM(uint16_t value) {
    if (tim_ != nullptr) {
        // 限制PWM值范围
        if (value > 65535) value = 65535;

        // 直接使用传入的定时器句柄
        __HAL_TIM_SET_COMPARE(tim_, channel_, value);
        return true;
    }
    return false;  // GPIO模式不支持PWM
}

bool LED::BreathingLight(uint8_t period, uint8_t interval) {
    if (tim_ != nullptr) {
        static uint32_t last_update = 0;
        static uint16_t current_step = 0;
        static bool is_breathing = false;
        static uint8_t breathing_period = 0;
        static uint8_t breathing_interval = 0;

        uint32_t current_time = get_system_tick();

        // 如果参数改变，重新初始化
        if (!is_breathing || breathing_period != period || breathing_interval != interval) {
            is_breathing = true;
            breathing_period = period;
            breathing_interval = interval;
            current_step = 0;
            last_update = current_time;
        }

        // 检查是否需要更新
        if (current_time - last_update >= interval) {
            last_update = current_time;

            // 计算呼吸灯相位 (0-2π)
            float phase = (2.0f * 3.1415926f * current_step) / (period * 1000 / interval);

            // 计算亮度 (0-1)
            float brightness = (sin(phase) + 1.0f) / 2.0f;

            // 设置PWM值
            uint16_t pwm_value = static_cast<uint16_t>(brightness * 65535);
            __HAL_TIM_SET_COMPARE(tim_, channel_, pwm_value);

            // 确保PWM已启动 - 简化逻辑，直接启动PWM
            HAL_TIM_PWM_Start(tim_, channel_);

            // 更新步骤
            current_step++;
            if (current_step >= (period * 1000 / interval)) {
                current_step = 0;
            }
        }

        return true;
    }
    return false;  // GPIO模式不支持呼吸灯
}

void LED::setPortPin(GPIO_TypeDef* port, uint16_t pin) {
    port_ = port;
    pin_ = pin;
}

void LED::setTimChannel(TIM_HandleTypeDef* tim, uint32_t channel) {
    tim_ = tim;
    channel_ = channel;
}

// RGBLED类实现
RGBLED::RGBLED(LED& red_led, LED& green_led, LED& blue_led)
        : red_led_(red_led), green_led_(green_led), blue_led_(blue_led) {}

void RGBLED::setColor(Color_Types color_type) {
    setColorARGB(255, (color_type >> 16) & 0xFF, (color_type >> 8) & 0xFF, color_type & 0xFF);
}

void RGBLED::setColorRGB(uint8_t r, uint8_t g, uint8_t b) { setColorARGB(255, r, g, b); }

void RGBLED::setColorARGB(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
    // 完全按照官方demo的方式：直接乘法
    uint16_t red_pwm = red * alpha;      // 255 * 255 = 65025
    uint16_t green_pwm = green * alpha;  // 虽然不是65535，但这是官方demo的方式
    uint16_t blue_pwm = blue * alpha;

    // 直接设置PWM值，避免setBrightness的二次转换
    red_led_.setPWM(red_pwm);
    green_led_.setPWM(green_pwm);
    blue_led_.setPWM(blue_pwm);
}

void RGBLED::setColorHSV(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, r, g, b);
    setColorRGB(r, g, b);
}

bool RGBLED::BreathingLight(uint8_t period, uint8_t interval) {
    // 对三个LED同时进行呼吸灯效果
    bool result = true;
    result &= red_led_.BreathingLight(period, interval);
    result &= green_led_.BreathingLight(period, interval);
    result &= blue_led_.BreathingLight(period, interval);
    return result;
}

void RGBLED::setBrightness(uint8_t brightness) {
    red_led_.setBrightness(brightness);
    green_led_.setBrightness(brightness);
    blue_led_.setBrightness(brightness);
}

void RGBLED::on() {
    red_led_.on();
    green_led_.on();
    blue_led_.on();
}

void RGBLED::off() {
    red_led_.off();
    green_led_.off();
    blue_led_.off();
}

void RGBLED::toggle(uint8_t times, uint16_t delay_ms) {
    for (uint8_t i = 0; i < times; i++) {
        on();
        HAL_Delay(delay_ms);
        off();
        if (i < times - 1) {
            HAL_Delay(delay_ms);
        }
    }
}
