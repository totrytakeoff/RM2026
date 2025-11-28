/**
 * @file    button_manager.hpp
 * @brief   按钮管理器头文件（处理中断模式）
 * @author  myself 
 * @date    2025/11/27
 */

#ifndef __BUTTON_MANAGER_HPP
#define __BUTTON_MANAGER_HPP

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

// 前向声明
class Button;

// 最大支持的按钮数量
#define MAX_BUTTON_COUNT 16

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册按钮对象（用于中断模式）
 * @param button 按钮对象指针
 */
bool register_button(Button* button);

/**
 * @brief 注销按钮对象
 * @param button 按钮对象指针
 */
void unregister_button(Button* button);

/**
 * @brief 按钮中断处理函数（C接口，供中断处理函数调用）
 * @param GPIO_Pin 触发中断的引脚
 */
void button_interrupt_handler(uint16_t GPIO_Pin);

#ifdef __cplusplus
}

/**
 * @brief 按钮管理器类（使用简单数组管理多个按钮）
 */
class ButtonManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ButtonManager& getInstance();
    
    /**
     * @brief 注册按钮
     * @param button 按钮对象指针
     * @return true:注册成功 false:注册失败（已满或已存在）
     */
    bool registerButton(Button* button);
    
    /**
     * @brief 注销按钮
     * @param button 按钮对象指针
     * @return true:注销成功 false:注销失败（未找到）
     */
    bool unregisterButton(Button* button);
    
    /**
     * @brief 处理中断
     * @param GPIO_Pin 触发中断的引脚
     */
    void handleInterrupt(uint16_t GPIO_Pin);
    
    /**
     * @brief 获取注册的按钮数量
     */
    uint8_t getButtonCount() const;
    
    /**
     * @brief 根据引脚查找按钮
     * @param port GPIO端口
     * @param pin GPIO引脚
     * @return 按钮指针，未找到返回nullptr
     */
    Button* findButton(GPIO_TypeDef* port, uint16_t pin);

private:
    ButtonManager() = default;
    ~ButtonManager() = default;
    ButtonManager(const ButtonManager&) = delete;
    ButtonManager& operator=(const ButtonManager&) = delete;
    
    Button* buttons_[MAX_BUTTON_COUNT];  // 按钮指针数组
    uint8_t button_count_;                // 当前按钮数量
};

#endif /* __cplusplus */

#endif /* __BUTTON_MANAGER_HPP */
