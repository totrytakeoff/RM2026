/******************************************************************************
 *
 * @file       buzzer.hpp
 * @brief      蜂鸣器封装
 *
 * @author     myself
 * @date       2025/11/28
 * 
 *****************************************************************************/

#ifndef BUZZER_HPP
#define BUZZER_HPP

#include "pin_map.h"
#include "stm32f4xx.h"

/**
 * @brief 蜂鸣器类，封装蜂鸣器的常用操作
 * @note 硬件初始化需在main中完成，此类仅提供控制接口
 */
class Buzzer {
public:
    /**
     * @brief 音符枚举，定义C大调音阶的音符频率
     */
    enum Note {
        NOTE_C4 = 262,   // 中央C
        NOTE_D4 = 294,   // D
        NOTE_E4 = 330,   // E
        NOTE_F4 = 349,   // F
        NOTE_G4 = 392,   // G
        NOTE_A4 = 440,   // A
        NOTE_B4 = 494,   // B
        NOTE_C5 = 523,   // 高八度C
        NOTE_D5 = 587,   // 高八度D
        NOTE_E5 = 659,   // 高八度E
        NOTE_F5 = 698,   // 高八度F
        NOTE_G5 = 784,   // 高八度G
        NOTE_A5 = 880,   // 高八度A
        NOTE_B5 = 988,   // 高八度B
        NOTE_REST = 0    // 休止符
    };

    /**
     * @brief 构造函数
     * @note 构造时不进行硬件初始化
     */
    Buzzer();

    /**
     * @brief 析构函数
     */
    ~Buzzer();

    /**
     * @brief 播放指定频率的音调
     * @param frequency 频率值(Hz)
     * @param volume 音量(0-100)
     * @note 需确保TIM4_CH3已正确初始化
     */
    void playTone(uint32_t frequency, uint8_t volume = 50);

    /**
     * @brief 播放音乐音符
     * @param note 音符枚举值
     * @param duration 持续时间(ms)
     * @param volume 音量(0-100)
     */
    void playNote(Note note, uint32_t duration, uint8_t volume = 50);

    /**
     * @brief 持续蜂鸣
     * @param volume 音量(0-100)
     */
    void beep(uint8_t volume = 50);

    /**
     * @brief 停止蜂鸣
     */
    void stop();

    /**
     * @brief 设置音量
     * @param volume 音量(0-100)
     */
    void setVolume(uint8_t volume);

    /**
     * @brief 播放短提示音
     */
    void playShortBeep();

    /**
     * @brief 播放长提示音
     */
    void playLongBeep();

    /**
     * @brief 播放错误提示音
     */
    void playErrorBeep();

    /**
     * @brief 播放成功提示音
     */
    void playSuccessBeep();

private:
    /**
     * @brief 计算预分频器值
     * @param frequency 频率值(Hz)
     * @retval uint16_t 预分频器值
     */
    uint16_t calculatePrescaler(uint32_t frequency);

    /**
     * @brief 计算PWM比较值
     * @param volume 音量(0-100)
     * @retval uint16_t PWM比较值
     */
    uint16_t calculateCompareValue(uint8_t volume);

    /**
     * @brief 设置TIM4的PWM参数
     * @param psc 预分频器值
     * @param compare PWM比较值
     */
    void setTIM4PWM(uint16_t psc, uint16_t compare);

    uint8_t currentVolume;   // 当前音量
};

#endif // BUZZER_HPP