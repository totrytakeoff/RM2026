/**
 * @file    button_manager.cpp
 * @brief   按钮管理器（处理中断模式）
 * @author  myself 
 * @date    2025/11/27
 */

#include "button_manager.hpp"
#include "button.hpp"

/**
 * @brief 获取单例实例
 */
ButtonManager& ButtonManager::getInstance() {
    static ButtonManager instance;
    return instance;
}

/**
 * @brief 注册按钮
 * @param button 按钮对象指针
 * @return true:注册成功 false:注册失败（已满或已存在）
 */
bool ButtonManager::registerButton(Button* button) {
    if (button == nullptr) {
        return false;
    }
    
    // 检查是否已满
    if (button_count_ >= MAX_BUTTON_COUNT) {
        return false;
    }
    
    // 检查是否已经注册
    for (uint8_t i = 0; i < button_count_; i++) {
        if (buttons_[i] == button) {
            return false;  // 已经注册
        }
    }
    
    // 添加到数组
    buttons_[button_count_] = button;
    button_count_++;
    
    return true;
}

/**
 * @brief 注销按钮
 * @param button 按钮对象指针
 * @return true:注销成功 false:注销失败（未找到）
 */
bool ButtonManager::unregisterButton(Button* button) {
    if (button == nullptr) {
        return false;
    }
    
    // 查找按钮
    for (uint8_t i = 0; i < button_count_; i++) {
        if (buttons_[i] == button) {
            // 找到，移除并移动后续元素
            for (uint8_t j = i; j < button_count_ - 1; j++) {
                buttons_[j] = buttons_[j + 1];
            }
            button_count_--;
            buttons_[button_count_] = nullptr;  // 清空最后一个位置
            return true;
        }
    }
    
    return false;  // 未找到
}

/**
 * @brief 处理中断
 * @param GPIO_Pin 触发中断的引脚
 */
void ButtonManager::handleInterrupt(uint16_t GPIO_Pin) {
    // 遍历所有注册的按钮
    for (uint8_t i = 0; i < button_count_; i++) {
        Button* button = buttons_[i];
        if (button != nullptr && button->getWorkMode() == ButtonWorkMode::INTERRUPT) {
            // 检查引脚是否匹配
            if (GPIO_Pin == button->getPin()) {
                // 读取当前引脚状态
                GPIO_PinState pin_state = HAL_GPIO_ReadPin(button->getPort(), button->getPin());
                button->interruptCallback(pin_state);
                break;  // 找到匹配的按钮后退出循环
            }
        }
    }
}

/**
 * @brief 获取注册的按钮数量
 */
uint8_t ButtonManager::getButtonCount() const {
    return button_count_;
}

/**
 * @brief 根据引脚查找按钮
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return 按钮指针，未找到返回nullptr
 */
Button* ButtonManager::findButton(GPIO_TypeDef* port, uint16_t pin) {
    for (uint8_t i = 0; i < button_count_; i++) {
        Button* button = buttons_[i];
        if (button != nullptr && 
            button->getPort() == port && 
            button->getPin() == pin) {
            return button;
        }
    }
    return nullptr;
}

// C接口函数实现
extern "C" {

/**
 * @brief 注册按钮对象（用于中断模式）
 * @param button 按钮对象指针
 */
bool register_button(Button* button) {
    return ButtonManager::getInstance().registerButton(button);
}

/**
 * @brief 注销按钮对象
 * @param button 按钮对象指针
 */
void unregister_button(Button* button) {
    ButtonManager::getInstance().unregisterButton(button);
}

/**
 * @brief 按钮中断处理函数（C接口，供中断处理函数调用）
 * @param GPIO_Pin 触发中断的引脚
 */
void button_interrupt_handler(uint16_t GPIO_Pin) {
    ButtonManager::getInstance().handleInterrupt(GPIO_Pin);
}

} // extern "C"
