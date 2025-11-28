/******************************************************************************
 *
 * @file       buzzer.cpp
 * @brief      蜂鸣器封装实现
 *
 * @author     myself
 * @date       2025/11/28
 * 
 *****************************************************************************/

#include "buzzer.hpp"
#include "pin_map.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

// 声明htim4外部变量，与官方demo保持一致
extern TIM_HandleTypeDef htim4;

/**
 * @brief TIM4时钟频率 (Hz)
 * @note TIM4挂载在APB1上，APB1时钟频率为42MHz，TIM4时钟为42MHz * 2 = 84MHz
 */
#define TIM4_CLOCK_FREQ 84000000

/**
 * @brief 最大预分频器值
 */
#define MAX_PSC 1000

/**
 * @brief PWM范围定义
 */
#define MAX_BUZZER_PWM 20000  // 最大PWM值
#define MIN_BUZZER_PWM 10000  // 最小PWM值

/**
 * @brief 构造函数
 */
Buzzer::Buzzer() : currentVolume(50) {
    // 构造函数不进行硬件初始化
}

/**
 * @brief 析构函数
 */
Buzzer::~Buzzer() {
    stop();
}

/**
 * @brief 计算PWM周期值
 * @param frequency 频率值(Hz)
 * @retval uint32_t PWM周期值
 */
uint16_t Buzzer::calculatePrescaler(uint32_t frequency) {
    if (frequency == 0) {
        return MAX_PSC;  // 返回最大值，使频率最小
    }
    
    // 根据目标频率计算预分频器值
    // 公式：PSC = (TIM4时钟频率 / (ARR * 目标频率)) - 1
    // 其中ARR在官方demo中固定为20999
    uint32_t psc = (TIM4_CLOCK_FREQ / (21000 * frequency)) - 1;
    
    // 限制预分频器范围
    if (psc > MAX_PSC) {
        psc = MAX_PSC;
    } else if (psc < 0) {
        psc = 0;
    }
    
    return static_cast<uint16_t>(psc);
}

/**
 * @brief 计算PWM比较值
 * @param period PWM周期值
 * @param volume 音量(0-100)
 * @retval uint32_t PWM比较值
 */
uint16_t Buzzer::calculateCompareValue(uint8_t volume) {
    // 限制音量范围
    if (volume > 100) {
        volume = 100;
    }
    if (volume == 0) {
        return 0;
    }
    
    // 根据音量计算PWM值，范围在MIN_BUZZER_PWM到MAX_BUZZER_PWM之间
    // 官方demo使用这个范围来控制音量
    uint32_t pwmValue = MIN_BUZZER_PWM + ((MAX_BUZZER_PWM - MIN_BUZZER_PWM) * volume) / 100;
    
    return static_cast<uint16_t>(pwmValue);
}

/**
 * @brief 设置TIM4的PWM参数
 * @param period PWM周期值
 * @param compare PWM比较值
 */
void Buzzer::setTIM4PWM(uint16_t psc, uint16_t compare) {
    // 使用HAL库函数设置预分频器和比较值
    // 这与官方demo的实现方式一致
    __HAL_TIM_PRESCALER(&htim4, psc);
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, compare);
}

/**
 * @brief 播放指定频率的音调
 * @param frequency 频率值(Hz)
 * @param volume 音量(0-100)
 */
void Buzzer::playTone(uint32_t frequency, uint8_t volume) {
    // 更新当前音量
    currentVolume = volume;
    
    if (frequency == 0 || volume == 0) {
        stop();
        return;
    }
    
    // 计算PWM参数
    uint16_t psc = calculatePrescaler(frequency);
    uint16_t compareValue = calculateCompareValue(volume);
    
    // 设置PWM参数
    setTIM4PWM(psc, compareValue);
}

/**
 * @brief 播放音乐音符
 * @param note 音符枚举值
 * @param duration 持续时间(ms)
 * @param volume 音量(0-100)
 */
void Buzzer::playNote(Note note, uint32_t duration, uint8_t volume) {
    // 播放指定音符
    playTone(static_cast<uint32_t>(note), volume);
    
    // 使用HAL_Delay实现精确的延时，确保音符持续指定的时间
    HAL_Delay(duration);
    
    // 播放完成后停止音符，这样下一个音符开始前会有明确的停顿
    stop();
}

/**
 * @brief 持续蜂鸣
 * @param volume 音量(0-100)
 */
void Buzzer::beep(uint8_t volume) {
    // 更新当前音量并播放默认频率
    currentVolume = volume;
    playTone(BUZZER_DEFAULT_FREQ, volume);
}

/**
 * @brief 停止蜂鸣
 */
void Buzzer::stop() {
    // 将比较值设置为0来停止蜂鸣器
    // 这与官方demo的实现方式一致
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 0);
}

/**
 * @brief 设置音量
 * @param volume 音量(0-100)
 */
void Buzzer::setVolume(uint8_t volume) {
    currentVolume = volume;
    
    // 直接应用音量变化，即使当前正在播放
    // 这样可以立即听到音量变化效果
    uint16_t newCompare = calculateCompareValue(volume);
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, newCompare);
}

/**
 * @brief 播放短提示音
 */
void Buzzer::playShortBeep() {
    playNote(NOTE_B4, 100, currentVolume);
    stop();
}

/**
 * @brief 播放长提示音
 */
void Buzzer::playLongBeep() {
    playNote(NOTE_B4, 500, currentVolume);
    stop();
}

/**
 * @brief 播放错误提示音
 */
void Buzzer::playErrorBeep() {
    // 播放两个下降音调表示错误
    playNote(NOTE_B4, 150, currentVolume);
    for (uint32_t i = 0; i < 500000; i++) {__NOP();} // 短暂停顿，增加乘数
    playNote(NOTE_A4, 150, currentVolume);
    stop();
}

/**
 * @brief 播放成功提示音
 */
void Buzzer::playSuccessBeep() {
    // 播放两个上升音调表示成功
    playNote(NOTE_A4, 150, currentVolume);
    for (uint32_t i = 0; i < 500000; i++) {__NOP();} // 短暂停顿，增加乘数
    playNote(NOTE_B4, 150, currentVolume);
    stop();
}