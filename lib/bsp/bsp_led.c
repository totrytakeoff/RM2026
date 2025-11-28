/**
 * @file bsp_led.c
 * @brief RM C型板RGB LED BSP封装实现文件
 * @author RM2026 Team
 * @version 1.0
 * @date 2025-11-26
 * 
 * 本文件提供RGB LED的BSP封装实现，
 * 支持基本颜色显示、呼吸灯、流水灯、渐变等功能。
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_led.h"
#include "bsp_config.h"
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* PWM配置 - 参考官方demo */
#define BSP_LED_PWM_PERIOD        65535    /* PWM周期，与官方demo一致 */
#define BSP_LED_TIM_INSTANCE     5        /* TIM5 */

/* TIM通道定义 */
#define TIM_CHANNEL_1            1
#define TIM_CHANNEL_2            2
#define TIM_CHANNEL_3            3


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static BSP_LED_ConfigTypeDef led_config = {0};
static uint8_t led_brightness = 255;
static uint32_t flow_colors[8] = {0};
static uint8_t flow_color_count = 0;
static uint32_t gradient_start_color = 0;
static uint32_t gradient_end_color = 0;
static uint32_t blink_color = 0;
static uint8_t blink_duty_cycle = 50;

/* 外部变量声明 - 这些函数在bsp_tim.c中实现 */
extern void BSP_TIM_Start(uint8_t tim_instance);
extern void BSP_TIM_Stop(uint8_t tim_instance);
extern void BSP_TIM_PWM_Start(uint8_t tim_instance, uint8_t channel);
extern void BSP_TIM_PWM_Stop(uint8_t tim_instance, uint8_t channel);
extern void BSP_TIM_SetCompare(uint8_t tim_instance, uint8_t channel, uint32_t compare);
extern uint32_t BSP_GetTick(void);

/* Private function prototypes -----------------------------------------------*/
static void BSP_LED_InitPWM(void);
static void BSP_LED_SetPWMValue(uint16_t red, uint16_t green, uint16_t blue);
static uint32_t BSP_LED_GetCurrentTime(void);
static void BSP_LED_UpdateBreathing(void);
static void BSP_LED_UpdateFlow(void);
static void BSP_LED_UpdateGradient(void);
static void BSP_LED_UpdateBlink(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief BSP LED初始化函数
 * @retval BSP_StatusTypeDef 初始化状态
 */
BSP_StatusTypeDef BSP_LED_Init(void)
{
    /* 初始化配置结构体 */
    led_config.color.red = 0;
    led_config.color.green = 0;
    led_config.color.blue = 0;
    led_config.effect = BSP_LED_EFFECT_NONE;
    led_config.period = 1000;
    led_config.enabled = 1;
    led_config.last_update_time = BSP_LED_GetCurrentTime();
    led_config.current_step = 0;
    
    /* 默认亮度和颜色 */
    led_brightness = 255;
    flow_color_count = 0;
    
    /* 初始化PWM */
    BSP_LED_InitPWM();
    
    /* 关闭LED */
    BSP_LED_Off();
    
    /* 初始化完成，做一个简单的测试闪烁 */
    BSP_LED_SetPresetColor(BSP_LED_COLOR_RED);
    BSP_Delay(100);
    BSP_LED_Off();
    BSP_Delay(100);
    BSP_LED_SetPresetColor(BSP_LED_COLOR_GREEN);
    BSP_Delay(100);
    BSP_LED_Off();
    
    return BSP_OK;
}

/**
 * @brief BSP LED去初始化函数
 * @retval BSP_StatusTypeDef 去初始化状态
 */
BSP_StatusTypeDef BSP_LED_DeInit(void)
{
    /* 停止PWM */
    BSP_TIM_PWM_Stop(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_RED);
    BSP_TIM_PWM_Stop(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_GREEN);
    BSP_TIM_PWM_Stop(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_BLUE);
    
    /* 停止定时器 */
    BSP_TIM_Stop(BSP_LED_TIM_INSTANCE);
    
    return BSP_OK;
}

/**
 * @brief 设置RGB颜色（aRGB格式）
 * @param aRGB: 0xaaRRGGBB, 'aa'是透明度, 'RR'是红色, 'GG'是绿色, 'BB'是蓝色
 * @retval None
 */
void BSP_LED_SetColor(uint32_t aRGB)
{
    uint8_t alpha, red, green, blue;
    
    /* 停止当前效果 */
    BSP_LED_StopEffect();
    
    /* 提取颜色分量 */
    alpha = (aRGB >> 24) & 0xFF;
    red = (aRGB >> 16) & 0xFF;
    green = (aRGB >> 8) & 0xFF;
    blue = aRGB & 0xFF;
    
    /* 如果没有指定透明度，默认为255（完全不透明） */
    if (alpha == 0 && (red != 0 || green != 0 || blue != 0)) {
        alpha = 255;
    }
    
    /* 应用透明度和亮度 */
    red = (red * alpha * led_brightness) / (255 * 255);
    green = (green * alpha * led_brightness) / (255 * 255);
    blue = (blue * alpha * led_brightness) / (255 * 255);
    
    /* 保存当前颜色（保存原始颜色，不受亮度影响） */
    led_config.color.red = red;
    led_config.color.green = green;
    led_config.color.blue = blue;
    
    /* 设置PWM值 */
    BSP_LED_SetPWMValue(red, green, blue);
}

/**
 * @brief 设置RGB颜色（分量格式）
 * @param red: 红色分量 (0-255)
 * @param green: 绿色分量 (0-255)
 * @param blue: 蓝色分量 (0-255)
 * @retval None
 */
void BSP_LED_SetRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    BSP_LED_SetaRGB(255, red, green, blue);
}

/**
 * @brief 设置aRGB颜色（分量格式）
 * @param alpha: 透明度分量 (0-255)
 * @param red: 红色分量 (0-255)
 * @param green: 绿色分量 (0-255)
 * @param blue: 蓝色分量 (0-255)
 * @retval None
 */
void BSP_LED_SetaRGB(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t aRGB = ((uint32_t)alpha << 24) | ((uint32_t)red << 16) | 
                   ((uint32_t)green << 8) | ((uint32_t)blue);
    BSP_LED_SetColor(aRGB);
}

/**
 * @brief 设置预定义颜色
 * @param color: 预定义颜色值
 * @retval None
 */
void BSP_LED_SetPresetColor(uint32_t color)
{
    BSP_LED_SetColor(color);
}

/**
 * @brief 关闭LED
 * @retval None
 */
void BSP_LED_Off(void)
{
    BSP_LED_StopEffect();
    BSP_LED_SetPWMValue(0, 0, 0);
}

/**
 * @brief 开启LED（恢复上次颜色）
 * @retval None
 */
void BSP_LED_On(void)
{
    BSP_LED_SetPWMValue(led_config.color.red, led_config.color.green, led_config.color.blue);
}

/**
 * @brief 设置呼吸灯效果
 * @param color: 呼吸灯颜色
 * @param period: 呼吸周期(ms)
 * @retval None
 */
void BSP_LED_SetBreathing(uint32_t color, uint16_t period)
{
    led_config.effect = BSP_LED_EFFECT_BREATHING;
    led_config.period = period;
    led_config.last_update_time = BSP_LED_GetCurrentTime();
    led_config.current_step = 0;
    
    /* 保存呼吸灯颜色 */
    uint8_t red = ((color & 0x00FF0000) >> 16);
    uint8_t green = ((color & 0x0000FF00) >> 8);
    uint8_t blue = (color & 0x000000FF);
    
    led_config.color.red = red;
    led_config.color.green = green;
    led_config.color.blue = blue;
}

/**
 * @brief 设置流水灯效果
 * @param colors: 颜色数组
 * @param count: 颜色数量
 * @param period: 每个颜色的显示时间(ms)
 * @retval None
 */
void BSP_LED_SetFlow(const uint32_t *colors, uint8_t count, uint16_t period)
{
    if (colors == NULL || count == 0 || count > 8) {
        return;
    }
    
    led_config.effect = BSP_LED_EFFECT_FLOW;
    led_config.period = period;
    led_config.last_update_time = BSP_LED_GetCurrentTime();
    led_config.current_step = 0;
    
    /* 保存颜色数组 */
    for (uint8_t i = 0; i < count; i++) {
        flow_colors[i] = colors[i];
    }
    flow_color_count = count;
}

/**
 * @brief 设置渐变效果
 * @param start_color: 起始颜色
 * @param end_color: 结束颜色
 * @param period: 渐变时间(ms)
 * @retval None
 */
void BSP_LED_SetGradient(uint32_t start_color, uint32_t end_color, uint16_t period)
{
    led_config.effect = BSP_LED_EFFECT_GRADIENT;
    led_config.period = period;
    led_config.last_update_time = BSP_LED_GetCurrentTime();
    led_config.current_step = 0;
    
    gradient_start_color = start_color;
    gradient_end_color = end_color;
}

/**
 * @brief 设置闪烁效果
 * @param color: 闪烁颜色
 * @param period: 闪烁周期(ms)
 * @param duty_cycle: 占空比 (0-100)
 * @retval None
 */
void BSP_LED_SetBlink(uint32_t color, uint16_t period, uint8_t duty_cycle)
{
    led_config.effect = BSP_LED_EFFECT_BLINK;
    led_config.period = period;
    led_config.last_update_time = BSP_LED_GetCurrentTime();
    led_config.current_step = 0;
    
    blink_color = color;
    blink_duty_cycle = (duty_cycle > 100) ? 100 : duty_cycle;
}

/**
 * @brief 停止所有效果
 * @retval None
 */
void BSP_LED_StopEffect(void)
{
    led_config.effect = BSP_LED_EFFECT_NONE;
    led_config.current_step = 0;
}

/**
 * @brief LED效果更新函数（需要在主循环中调用）
 * @retval None
 */
void BSP_LED_Update(void)
{
    if (!led_config.enabled) {
        return;
    }
    
    switch (led_config.effect) {
        case BSP_LED_EFFECT_BREATHING:
            BSP_LED_UpdateBreathing();
            break;
            
        case BSP_LED_EFFECT_FLOW:
            BSP_LED_UpdateFlow();
            break;
            
        case BSP_LED_EFFECT_GRADIENT:
            BSP_LED_UpdateGradient();
            break;
            
        case BSP_LED_EFFECT_BLINK:
            BSP_LED_UpdateBlink();
            break;
            
        default:
            break;
    }
}

/**
 * @brief 获取LED调试信息
 * @param info: 调试信息结构体指针
 * @retval None
 */
void BSP_LED_GetDebugInfo(char *info, uint16_t max_len)
{
    if (info == NULL || max_len == 0) {
        return;
    }
    
    snprintf(info, max_len, 
             "LED: eff=%d, col=(%d,%d,%d), bright=%d, period=%d, step=%d, en=%d",
             led_config.effect,
             led_config.color.red, led_config.color.green, led_config.color.blue,
             led_brightness,
             led_config.period,
             led_config.current_step,
             led_config.enabled);
}

/**
 * @brief 获取当前LED配置
 * @retval BSP_LED_ConfigTypeDef* 当前配置指针
 */
BSP_LED_ConfigTypeDef* BSP_LED_GetConfig(void)
{
    return &led_config;
}

/**
 * @brief 设置LED亮度
 * @param brightness: 亮度值 (0-255)
 * @retval None
 */
void BSP_LED_SetBrightness(uint8_t brightness)
{
    led_brightness = brightness;
    
    /* 如果没有效果在运行，重新应用当前颜色 */
    if (led_config.effect == BSP_LED_EFFECT_NONE) {
        BSP_LED_SetRGB(led_config.color.red, led_config.color.green, led_config.color.blue);
    }
}

/**
 * @brief 获取LED亮度
 * @retval uint8_t 当前亮度值
 */
uint8_t BSP_LED_GetBrightness(void)
{
    return led_brightness;
}

/* 高级功能函数 -------------------------------------------------------------*/

/**
 * @brief 颜色混合
 * @param color1: 颜色1
 * @param color2: 颜色2
 * @param ratio: 混合比例 (0-255, 0表示全color1, 255表示全color2)
 * @retval uint32_t 混合后的颜色
 */
uint32_t BSP_LED_MixColor(uint32_t color1, uint32_t color2, uint8_t ratio)
{
    uint8_t a1, r1, g1, b1;
    uint8_t a2, r2, g2, b2;
    uint8_t a, r, g, b;
    
    /* 提取颜色分量 */
    a1 = (color1 & 0xFF000000) >> 24;
    r1 = ((color1 & 0x00FF0000) >> 16);
    g1 = ((color1 & 0x0000FF00) >> 8);
    b1 = (color1 & 0x000000FF);
    
    a2 = (color2 & 0xFF000000) >> 24;
    r2 = ((color2 & 0x00FF0000) >> 16);
    g2 = ((color2 & 0x0000FF00) >> 8);
    b2 = (color2 & 0x000000FF);
    
    /* 混合颜色 */
    a = a1 + ((a2 - a1) * ratio) / 255;
    r = r1 + ((r2 - r1) * ratio) / 255;
    g = g1 + ((g2 - g1) * ratio) / 255;
    b = b1 + ((b2 - b1) * ratio) / 255;
    
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
}

/**
 * @brief 颜色淡入
 * @param color: 目标颜色
 * @param duration: 淡入时间(ms)
 * @retval None
 */
void BSP_LED_FadeIn(uint32_t color, uint16_t duration)
{
    BSP_LED_SetGradient(BSP_LED_COLOR_BLACK, color, duration);
}

/**
 * @brief 颜色淡出
 * @param duration: 淡出时间(ms)
 * @retval None
 */
void BSP_LED_FadeOut(uint16_t duration)
{
    uint32_t current_color = ((uint32_t)255 << 24) | 
                             ((uint32_t)led_config.color.red << 16) | 
                             ((uint32_t)led_config.color.green << 8) | 
                             ((uint32_t)led_config.color.blue);
    BSP_LED_SetGradient(current_color, BSP_LED_COLOR_BLACK, duration);
}

/**
 * @brief 彩虹循环效果
 * @param period: 完整循环时间(ms)
 * @retval None
 */
void BSP_LED_Rainbow(uint16_t period)
{
    uint32_t rainbow_colors[] = {
        BSP_LED_COLOR_RED,
        BSP_LED_COLOR_YELLOW,
        BSP_LED_COLOR_GREEN,
        BSP_LED_COLOR_CYAN,
        BSP_LED_COLOR_BLUE,
        BSP_LED_COLOR_MAGENTA
    };
    
    BSP_LED_SetFlow(rainbow_colors, 6, period / 6);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 初始化PWM
 * @retval None
 */
static void BSP_LED_InitPWM(void)
{
    /* 启动定时器 */
    BSP_TIM_Start(BSP_LED_TIM_INSTANCE);
    
    /* 启动PWM通道 */
    BSP_TIM_PWM_Start(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_RED);
    BSP_TIM_PWM_Start(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_GREEN);
    BSP_TIM_PWM_Start(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_BLUE);
}

/**
 * @brief 设置PWM值
 * @param red: 红色PWM值 (0-255)
 * @param green: 绿色PWM值 (0-255)
 * @param blue: 蓝色PWM值 (0-255)
 * @retval None
 */
static void BSP_LED_SetPWMValue(uint16_t red, uint16_t green, uint16_t blue)
{
    /* 将8位颜色值映射到16位PWM周期 (65535) */
    /* 共阳极LED，占空比越高LED越亮 */
    uint16_t pwm_red = (red * 65535) / 255;
    uint16_t pwm_green = (green * 65535) / 255;
    uint16_t pwm_blue = (blue * 65535) / 255;
    
    /* 根据硬件连接映射：CH1->BLUE, CH2->GREEN, CH3->RED */
    BSP_TIM_SetCompare(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_BLUE, pwm_blue);
    BSP_TIM_SetCompare(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_GREEN, pwm_green);
    BSP_TIM_SetCompare(BSP_LED_TIM_INSTANCE, BSP_LED_TIM_CHANNEL_RED, pwm_red);
}

/**
 * @brief 获取当前时间
 * @retval uint32_t 当前时间(ms)
 */
static uint32_t BSP_LED_GetCurrentTime(void)
{
    return BSP_GetTick();
}

/**
 * @brief 更新呼吸灯效果
 * @retval None
 */
static void BSP_LED_UpdateBreathing(void)
{
    uint32_t current_time = BSP_LED_GetCurrentTime();
    uint32_t elapsed = current_time - led_config.last_update_time;
    
    if (elapsed >= 10) {  /* 每10ms更新一次 */
        led_config.last_update_time = current_time;
        
        /* 计算呼吸灯相位 (0-2π) */
        float phase = (2.0f * 3.14159f * led_config.current_step) / (led_config.period / 10);
        
        /* 计算亮度 (0-1) */
        float brightness = (sinf(phase) + 1.0f) / 2.0f;
        
        /* 获取呼吸灯原始颜色（不受亮度影响的） */
        uint8_t orig_red = (led_config.color.red * 255) / led_brightness;
        uint8_t orig_green = (led_config.color.green * 255) / led_brightness;
        uint8_t orig_blue = (led_config.color.blue * 255) / led_brightness;
        
        /* 应用呼吸亮度效果 */
        uint8_t red = (uint8_t)(orig_red * brightness * led_brightness) / 255;
        uint8_t green = (uint8_t)(orig_green * brightness * led_brightness) / 255;
        uint8_t blue = (uint8_t)(orig_blue * brightness * led_brightness) / 255;
        
        /* 设置PWM值 */
        BSP_LED_SetPWMValue(red, green, blue);
        
        /* 更新步骤 */
        led_config.current_step++;
        if (led_config.current_step >= led_config.period / 10) {
            led_config.current_step = 0;
        }
    }
}

/**
 * @brief 更新流水灯效果
 * @retval None
 */
static void BSP_LED_UpdateFlow(void)
{
    uint32_t current_time = BSP_LED_GetCurrentTime();
    
    if (current_time - led_config.last_update_time >= led_config.period) {
        led_config.last_update_time = current_time;
        
        /* 设置当前颜色 */
        BSP_LED_SetColor(flow_colors[led_config.current_step]);
        
        /* 更新步骤 */
        led_config.current_step++;
        if (led_config.current_step >= flow_color_count) {
            led_config.current_step = 0;
        }
    }
}

/**
 * @brief 更新渐变效果
 * @retval None
 */
static void BSP_LED_UpdateGradient(void)
{
    uint32_t current_time = BSP_LED_GetCurrentTime();
    uint32_t elapsed = current_time - led_config.last_update_time;
    
    if (elapsed >= 10) {  /* 每10ms更新一次 */
        led_config.last_update_time = current_time;
        
        /* 计算渐变进度 (0-1) */
        float progress = (float)led_config.current_step / (led_config.period / 10);
        
        /* 混合颜色 */
        uint32_t mixed_color = BSP_LED_MixColor(gradient_start_color, gradient_end_color, 
                                               (uint8_t)(progress * 255));
        
        /* 设置颜色 */
        BSP_LED_SetColor(mixed_color);
        
        /* 更新步骤 */
        led_config.current_step++;
        if (led_config.current_step >= led_config.period / 10) {
            led_config.current_step = 0;
        }
    }
}

/**
 * @brief 更新闪烁效果
 * @retval None
 */
static void BSP_LED_UpdateBlink(void)
{
    uint32_t current_time = BSP_LED_GetCurrentTime();
    uint32_t elapsed = current_time - led_config.last_update_time;
    
    if (elapsed >= 10) {  /* 每10ms更新一次 */
        led_config.last_update_time = current_time;
        
        /* 计算闪烁周期进度 */
        uint16_t cycle_progress = led_config.current_step % (led_config.period / 10);
        uint16_t half_period = (led_config.period / 10) / 2;
        
        /* 计算占空比 */
        uint16_t on_time = (half_period * blink_duty_cycle) / 50;
        
        if (cycle_progress < on_time) {
            /* LED开启 */
            BSP_LED_SetColor(blink_color);
        } else {
            /* LED关闭 */
            BSP_LED_SetPWMValue(0, 0, 0);
        }
        
        /* 更新步骤 */
        led_config.current_step++;
    }
}