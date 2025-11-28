/**
 * @file    button.cpp
 * @brief   按钮类实现
 * @author  myself 
 * @date    2025/11/27
 */

#include "button.hpp"
#include "button_manager.hpp"

/**
 * @brief 构造函数（轮询模式）
 */
Button::Button(GPIO_TypeDef* port, uint16_t pin, ButtonMode mode, uint32_t debounce_ms)
        : port_(port)
        , pin_(pin)
        , mode_(mode)
        , work_mode_(ButtonWorkMode::POLLING)
        , debounce_time_(debounce_ms)
        , last_state_(false)
        , current_state_(false)
        , last_change_time_(0)
        , press_start_time_(0)
        , press_triggered_(false)
        , release_triggered_(false)
        , initialized_(false)
        , prev_state_for_release_(false)
        , prev_state_for_longpress_(false) {
    init();
}

/**
 * @brief 构造函数（支持中断和轮询模式）
 */
Button::Button(GPIO_TypeDef* port, uint16_t pin, ButtonMode mode, ButtonWorkMode work_mode, uint32_t debounce_ms)
        : port_(port)
        , pin_(pin)
        , mode_(mode)
        , work_mode_(work_mode)
        , debounce_time_(debounce_ms)
        , last_state_(false)
        , current_state_(false)
        , last_change_time_(0)
        , press_start_time_(0)
        , press_triggered_(false)
        , release_triggered_(false)
        , initialized_(false)
        , prev_state_for_release_(false)
        , prev_state_for_longpress_(false) {
    init();
}

/**
 * @brief 初始化按钮GPIO
 */
void Button::init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能GPIO时钟
    enablePortClock();

    // 配置GPIO引脚
    GPIO_InitStruct.Pin = pin_;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    // 根据工作模式配置GPIO
    if (work_mode_ == ButtonWorkMode::INTERRUPT) {
        // 中断模式：配置为上升沿和下降沿触发
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    } else {
        // 轮询模式：配置为输入模式
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    }

    // 根据模式设置上拉/下拉
    if (mode_ == ButtonMode::PULL_UP) {
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    } else {
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    }

    HAL_GPIO_Init(port_, &GPIO_InitStruct);

    // 初始化状态
    last_state_ = read();
    current_state_ = last_state_;
    last_change_time_ = HAL_GetTick();

    initialized_ = true;
}

/**
 * @brief 使能GPIO端口时钟
 */
void Button::enablePortClock() {
    if (port_ == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port_ == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port_ == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port_ == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port_ == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

/**
 * @brief 读取按钮原始电平
 */
GPIO_PinState Button::readRaw() const { return HAL_GPIO_ReadPin(port_, pin_); }

/**
 * @brief 读取按钮逻辑状态
 */
bool Button::read() const {
    GPIO_PinState pin_state = readRaw();

    // 根据模式返回逻辑状态
    if (mode_ == ButtonMode::PULL_UP) {
        // 上拉模式：按下为低电平
        return (pin_state == GPIO_PIN_RESET);
    } else {
        // 下拉模式：按下为高电平
        return (pin_state == GPIO_PIN_SET);
    }
}

/**
 * @brief 更新按钮状态（内部使用）
 */
void Button::update() {
    if (!initialized_) {
        return;
    }

    bool new_state = read();
    uint32_t current_time = HAL_GetTick();

    // 状态改变
    if (new_state != last_state_) {
        last_change_time_ = current_time;
        last_state_ = new_state;
    }

    // 防抖：状态稳定超过防抖时间
    if ((current_time - last_change_time_) >= debounce_time_) {
        current_state_ = new_state;
    }
}

/**
 * @brief 检测按钮按下事件
 */
bool Button::isPressed() {
    update();

    // 检测从未按下到按下的边沿
    if (current_state_ && !press_triggered_) {
        press_triggered_ = true;
        release_triggered_ = false;
        press_start_time_ = HAL_GetTick();
        return true;
    }

    // 按钮释放后重置触发标志
    if (!current_state_) {
        press_triggered_ = false;
    }

    return false;
}

/**
 * @brief 检测按钮释放事件
 */
bool Button::isReleased() {
    update();

    // 检测从按下到释放的边沿（下降沿）
    bool released = (prev_state_for_release_ && !current_state_);
    prev_state_for_release_ = current_state_;

    if (released) {
        release_triggered_ = true;
        press_triggered_ = false;
        return true;
    }

    return false;
}

/**
 * @brief 检测长按事件
 */
bool Button::isLongPressed(uint32_t long_press_ms) {
    update();

    uint32_t current_time = HAL_GetTick();

    // 检测按下的上升沿（使用防抖后的状态）
    if (current_state_ && !prev_state_for_longpress_) {
        press_start_time_ = current_time;
    }

    // 更新上一次状态
    prev_state_for_longpress_ = current_state_;

    // 如果按钮当前按下，检查持续时间
    if (current_state_) {
        if (press_start_time_ > 0) {
            uint32_t duration = current_time - press_start_time_;
            if (duration >= long_press_ms) {
                return true;
            }
        }
    } else {
        // 按钮释放时清零
        if (press_start_time_ > 0) {
            press_start_time_ = 0;
        }
    }

    return false;
}

/**
 * @brief 获取按钮按下持续时间
 */
uint32_t Button::getPressedDuration() const {
    if (current_state_ && press_start_time_ > 0) {
        return HAL_GetTick() - press_start_time_;
    }
    return 0;
}

/**
 * @brief 重置按钮状态
 */
void Button::reset() {
    update();  // 先更新一次状态

    press_triggered_ = false;
    release_triggered_ = false;
    press_start_time_ = 0;

    // 重置边沿检测状态，避免误触发
    prev_state_for_release_ = current_state_;
    prev_state_for_longpress_ = current_state_;

    // 如果当前按钮是按下状态，设置press_triggered_为true
    // 这样可以避免下次调用isPressed()时误认为是新的按下边沿
    if (current_state_) {
        press_triggered_ = true;
    }
}

/**
 * @brief 设置防抖时间
 */
void Button::setDebounceTime(uint32_t debounce_ms) { debounce_time_ = debounce_ms; }

/**
 * @brief 启用中断模式（注册到中断管理器）
 * @return true:注册成功 false:注册失败
 */
bool Button::enableInterrupt() {
    if (work_mode_ == ButtonWorkMode::INTERRUPT) {
        ButtonManager::getInstance().registerButton(this);
        return true;
    }
    return false;
}

/**
 * @brief 中断回调函数（由中断处理函数调用）
 * @param pin_state 触发中断时的引脚状态
 */
void Button::interruptCallback(GPIO_PinState pin_state) {
    uint32_t current_time = HAL_GetTick();
    
    // 将引脚状态转换为逻辑状态
    bool logic_state;
    if (mode_ == ButtonMode::PULL_UP) {
        logic_state = (pin_state == GPIO_PIN_RESET);
    } else {
        logic_state = (pin_state == GPIO_PIN_SET);
    }

    // 状态改变
    if (logic_state != last_state_) {
        last_change_time_ = current_time;
        last_state_ = logic_state;
    }

    // 防抖：状态稳定超过防抖时间
    if ((current_time - last_change_time_) >= debounce_time_) {
        current_state_ = logic_state;
    }
}
