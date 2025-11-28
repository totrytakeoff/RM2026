/******************************************************************************
 *
 * @file       led.hpp
 * @brief      通用led封装类
 *
 * @author     myself
 * @date       2025/11/27
 *
 *****************************************************************************/

#ifndef LED_HPP
#define LED_HPP

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

struct RGB_LED {
    GPIO_TypeDef* port;
    uint16_t pin_r;
    uint16_t pin_g;
    uint16_t pin_b;
};

struct RGB_Struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct ARGB_Struct {
    uint8_t alpha; /**< 透明度分量 (0-255) */
    uint8_t red;   /**< 红色分量 (0-255) */
    uint8_t green; /**< 绿色分量 (0-255) */
    uint8_t blue;  /**< 蓝色分量 (0-255) */
};

struct HSV_Struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
};

#define LED_COLOR_RED ((uint32_t)0xFFFF0000)     /**< 红色 */
#define LED_COLOR_GREEN ((uint32_t)0xFF00FF00)   /**< 绿色 */
#define LED_COLOR_BLUE ((uint32_t)0xFF0000FF)    /**< 蓝色 */
#define LED_COLOR_YELLOW ((uint32_t)0xFFFFFF00)  /**< 黄色 */
#define LED_COLOR_CYAN ((uint32_t)0xFF00FFFF)    /**< 青色 */
#define LED_COLOR_MAGENTA ((uint32_t)0xFFFF00FF) /**< 洋红色 */
#define LED_COLOR_WHITE ((uint32_t)0xFFFFFFFF)   /**< 白色 */
#define LED_COLOR_BLACK ((uint32_t)0xFF000000)   /**< 黑色(关闭) */
#define LED_COLOR_ORANGE ((uint32_t)0xFFFFA500)  /**< 橙色 */
#define LED_COLOR_PURPLE ((uint32_t)0xFF800080)  /**< 紫色 */

enum Color_Types {
    RED = LED_COLOR_RED,
    ORANGE = LED_COLOR_ORANGE,
    YELLOW = LED_COLOR_YELLOW,
    GREEN = LED_COLOR_GREEN,
    CYAN = LED_COLOR_CYAN,
    BLUE = LED_COLOR_BLUE,
    PURPLE = LED_COLOR_PURPLE,
    WHITE = LED_COLOR_WHITE,
};

class LED {
public:
    LED(GPIO_TypeDef* port, uint16_t pin);
    LED(GPIO_TypeDef* port, uint16_t pin, TIM_HandleTypeDef* tim, uint32_t channel);

    void on();
    void off();
    void toggle(uint8_t times = 1, uint16_t delay_ms = 200);  // 闪烁 times 次，每次间隔 delay_ms ms

    bool setBrightness(uint8_t brightness);  // 控制led 亮度 0-255  仅对pwm模式有效

    bool setPWM(uint16_t value);  // pwm 直接设置占空比控制led

    bool BreathingLight(uint8_t period,
                        uint8_t interval);  // period: 呼吸周期(s), interval: 呼吸间隔(ms)

    void setPortPin(GPIO_TypeDef* port, uint16_t pin);
    void setTimChannel(TIM_HandleTypeDef* tim, uint32_t channel);

    GPIO_TypeDef* getPort() { return port_; }
    uint16_t getPin() { return pin_; }

    TIM_HandleTypeDef* getTim() { return tim_; }
    uint32_t getChannel() { return channel_; }

private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
    TIM_HandleTypeDef* tim_;
    uint32_t channel_;
};

class RGBLED {
public:
    // RGBLED(RGB_LED rgb_led);
    RGBLED(LED& red_led, LED& green_led, LED& blue_led);

    void setColor(Color_Types color_type);
    void setColorRGB(uint8_t r, uint8_t g, uint8_t b);
    void setColorARGB(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);
    void setColorHSV(uint16_t h, uint8_t s, uint8_t v);

    bool BreathingLight(uint8_t period,
                        uint8_t interval);  // period: 呼吸周期(s), interval: 呼吸间隔(ms)
    void setBrightness(uint8_t brightness);                   // 0-255

    void on();
    void off();
    void toggle(uint8_t times = 1, uint16_t delay_ms = 200);  // 闪烁 times 次，每次间隔 delay_ms ms

    LED& getRedLED() { return red_led_; }
    LED& getGreenLED() { return green_led_; }
    LED& getBlueLED() { return blue_led_; }

    void setRed(LED& red_led) { red_led_ = red_led; }
    void setGreen(LED& green_led) { green_led_ = green_led; }
    void setBlue(LED& blue_led) { blue_led_ = blue_led; }

private:
    LED& red_led_;
    LED& green_led_;
    LED& blue_led_;
};

#endif
