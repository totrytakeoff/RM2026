/**
 * @file    button.hpp
 * @brief   按钮类（带防抖）
 * @author  myself 
 * @date    2025/11/27
 * 
 * 功能特性：
 * - ✅ 通用GPIO按钮支持
 * - ✅ 硬件防抖（50ms）
 * - ✅ 边沿检测（按下/释放）
 * - ✅ 长按检测（可选）
 * - ✅ 上拉/下拉可配置
 * 
 * 使用示例：
 * @code
 * // 创建按钮对象（PD2，按下为低电平，带上拉）
 * Button calibButton(GPIOD, GPIO_PIN_2, ButtonMode::PULL_UP);
 * 
 * // 在主循环中检测
 * if (calibButton.isPressed()) {
 *     // 按钮被按下
 * }
 * @endcode
 */

#ifndef __BUTTON_HPP
#define __BUTTON_HPP

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

/**
 * @brief 按钮模式
 */
enum class ButtonMode {
    PULL_UP,    ///< 上拉模式（按下为低电平）
    PULL_DOWN   ///< 下拉模式（按下为高电平）
};

/**
 * @brief 按钮工作模式
 */
enum class ButtonWorkMode {
    POLLING,    ///< 轮询模式（主循环中检测）
    INTERRUPT   ///< 中断模式（GPIO中断触发）
};

/**
 * @class Button
 * @brief 通用按钮类（带防抖）
 */
class Button {
public:
    /**
     * @brief 构造函数（轮询模式）
     * @param port GPIO端口（GPIOA, GPIOB, GPIOC等）
     * @param pin GPIO引脚（GPIO_PIN_0 ~ GPIO_PIN_15）
     * @param mode 按钮模式（上拉/下拉）
     * @param debounce_ms 防抖时间（毫秒），默认50ms
     */
    Button(GPIO_TypeDef* port, uint16_t pin, 
           ButtonMode mode = ButtonMode::PULL_UP,
           uint32_t debounce_ms = 50);
    
    /**
     * @brief 构造函数（支持中断和轮询模式）
     * @param port GPIO端口（GPIOA, GPIOB, GPIOC等）
     * @param pin GPIO引脚（GPIO_PIN_0 ~ GPIO_PIN_15）
     * @param mode 按钮模式（上拉/下拉）
     * @param work_mode 工作模式（轮询/中断）
     * @param debounce_ms 防抖时间（毫秒），默认50ms
     */
    Button(GPIO_TypeDef* port, uint16_t pin, 
           ButtonMode mode, ButtonWorkMode work_mode,
           uint32_t debounce_ms = 50);
    
    /**
     * @brief 初始化按钮GPIO
     * @note 会自动使能对应GPIO端口的时钟
     */
    void init();
    
    /**
     * @brief 读取按钮当前状态（原始电平）
     * @return GPIO_PinState 引脚电平状态
     */
    GPIO_PinState readRaw() const;
    
    /**
     * @brief 读取按钮逻辑状态（已处理上拉/下拉）
     * @return true 按钮按下
     * @return false 按钮未按下
     */
    bool read() const;
    
    /**
     * @brief 检测按钮按下事件（带防抖，边沿触发）
     * @return true 检测到按下事件（仅触发一次）
     * @return false 未检测到
     * 
     * @note 按下后返回true一次，释放后可再次触发
     */
    bool isPressed();
    
    /**
     * @brief 检测按钮释放事件（带防抖，边沿触发）
     * @return true 检测到释放事件
     * @return false 未检测到
     */
    bool isReleased();
    
    /**
     * @brief 检测长按事件
     * @param long_press_ms 长按时间阈值（毫秒），默认2000ms
     * @return true 检测到长按
     * @return false 未检测到
     * 
     * @note 长按期间会持续返回true
     */
    bool isLongPressed(uint32_t long_press_ms = 2000);
    
    /**
     * @brief 获取按钮按下持续时间
     * @return uint32_t 按下持续时间（毫秒），未按下返回0
     */
    uint32_t getPressedDuration() const;
    
    /**
     * @brief 重置按钮状态
     * @note 用于清除触发标志，通常不需要手动调用
     */
    void reset();
    
    /**
     * @brief 设置防抖时间
     * @param debounce_ms 防抖时间（毫秒）
     */
    void setDebounceTime(uint32_t debounce_ms);
    
    /**
     * @brief 中断回调函数（由中断处理函数调用）
     * @param pin_state 触发中断时的引脚状态
     */
    void interruptCallback(GPIO_PinState pin_state);
    
    /**
     * @brief 获取工作模式
     * @return ButtonWorkMode 当前工作模式
     */
    ButtonWorkMode getWorkMode() const { return work_mode_; }
    
    /**
     * @brief 获取GPIO端口
     * @return GPIO_TypeDef* GPIO端口指针
     */
    GPIO_TypeDef* getPort() const { return port_; }
    
    /**
     * @brief 获取GPIO引脚
     * @return uint16_t GPIO引脚号
     */
    uint16_t getPin() const { return pin_; }
    
    /**
     * @brief 启用中断模式（注册到中断管理器）
     * @return true:注册成功 false:注册失败
     * @note 只有在中断模式下才需要调用此方法
     */
    bool enableInterrupt();

private:
    /* ========== 硬件参数 ========== */
    GPIO_TypeDef* port_;        ///< GPIO端口
    uint16_t pin_;              ///< GPIO引脚
    ButtonMode mode_;           ///< 按钮模式
    ButtonWorkMode work_mode_;  ///< 工作模式（轮询/中断）
    uint32_t debounce_time_;    ///< 防抖时间（毫秒）
    
    /* ========== 状态变量 ========== */
    bool last_state_;           ///< 上次的按钮状态
    bool current_state_;        ///< 当前的按钮状态
    uint32_t last_change_time_; ///< 上次状态改变时间
    uint32_t press_start_time_; ///< 按下开始时间
    bool press_triggered_;      ///< 按下事件已触发标志
    bool release_triggered_;    ///< 释放事件已触发标志
    bool initialized_;          ///< 初始化标志
    
    /* ========== 边沿检测状态 ========== */
    bool prev_state_for_release_;  ///< 用于isReleased边沿检测
    bool prev_state_for_longpress_; ///< 用于isLongPressed边沿检测
    
    /**
     * @brief 使能GPIO端口时钟
     */
    void enablePortClock();
    
    /**
     * @brief 更新按钮状态（内部使用）
     */
    void update();
};

#endif /* __BUTTON_HPP */
