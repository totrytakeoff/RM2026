#ifndef GM6020_HPP
#define GM6020_HPP

//
//  文件: gm6020.hpp
//  说明: RoboMaster GM6020 电机封装（仅基础 C++，不使用 STL/动态分配）
//
//  功能:
//  - 基于 CAN 的电流控制/电压控制 (组播帧 0x1FF/0x2FF)
//  - 基于 PWM 的速度/位置控制 (50Hz, 1000~2000us)
//  - 反馈解析（角度/速度/电流/温度）可选
//
//  使用说明:
//  - 构造函数传入 CanBus* 与电机 ID (1..7)。
//  - 单电机控制: setCurrent()/setVoltage() 会只填写自己的通道并发送组播帧。
//    若同时控制多台，建议使用 sendCurrentGroup()/sendVoltageGroup() 以避免覆盖。
//  - PWM 控制: 需先调用 attachPwm() 绑定 TIM 句柄与通道，定时器需预先配置为 PWM 模式且频率为 50Hz。
//
extern "C" {
#include "stm32f4xx_hal.h"
}

#include "can_comm.hpp"

class GM6020 {
public:
    // 电机反馈结构
    struct Measure {
        uint16_t ecd;          // 编码器
        int16_t  speed_rpm;    // 速度
        int16_t  given_current;// 给定电流
        uint8_t  temperature;  // 温度
        uint16_t last_ecd;     // 上次编码器
    };

    // 构造: id=1..7
    GM6020(CanBus* can, uint8_t motor_id)
        : can_(can), id_(motor_id), pwm_tim_(0), pwm_channel_(0), period_ticks_(0)
    {
        if (id_ >= 1 && id_ <= 4) { group_id_ = 0x1FF; slot_ = id_ - 1; }
        else if (id_ >= 5 && id_ <= 7) { group_id_ = 0x2FF; slot_ = id_ - 5; }
        else { group_id_ = 0x1FF; slot_ = 0; }
        fbk_id_ = 0x204 + id_;
        meas_ = {0};
    }

    // 绑定 PWM 输出（要求外部已将定时器配置为 PWM 模式、50Hz）
    // 参数: tim = HAL TIM 句柄, channel = TIM_CHANNEL_x, period_ticks = AutoReload寄存器值(Period)
    void attachPwm(TIM_HandleTypeDef* tim, uint32_t channel, uint32_t period_ticks)
    {
        pwm_tim_ = tim; pwm_channel_ = channel; period_ticks_ = period_ticks;
    }

    // 发送单电机电流, 其它通道置 0（并发控制多电机时不推荐） 电流给定值范围：-16384~0~16384, 对应最大转矩电流范围 -3A~0~3A。
    bool setCurrent(int16_t current)
    {
        if (!can_) return false;
        uint8_t p[8] = {0};
        put16(p, slot_*2, current);
        return can_->sendStd(group_id_, p, 8) == HAL_OK;
    }

    // 发送单电机电压（-25000..+25000），其它通道置 0
    bool setVoltage(int16_t voltage)
    {
        if (!can_) return false;
        uint8_t p[8] = {0};
        put16(p, slot_*2, voltage);
        return can_->sendStd(group_id_, p, 8) == HAL_OK;
    }

    // 批量发送4通道电流（推荐用于同时控制多电机）电流给定值范围：-16384~0~16384, 对应最大转矩电流范围 -3A~0~3A。
    static bool sendCurrentGroup(CanBus* can, uint16_t group_id,
                                 int16_t i1, int16_t i2, int16_t i3, int16_t i4)
    {
        if (!can) return false;
        uint8_t p[8]; pack4(p, i1,i2,i3,i4);
        return can->sendStd(group_id, p, 8) == HAL_OK;
    }

    // 批量发送4通道电压 电压给定值范围：-25000~0~25000。
    static bool sendVoltageGroup(CanBus* can, uint16_t group_id,
                                 int16_t v1, int16_t v2, int16_t v3, int16_t v4)
    {
        if (!can) return false;
        uint8_t p[8]; pack4(p, v1,v2,v3,v4);
        return can->sendStd(group_id, p, 8) == HAL_OK;
    }

    // PWM: 直接设置脉宽(us), 一般范围 1000~2000us
    bool setPwmUs(uint16_t us)
    {
        if (!pwm_tim_ || period_ticks_ == 0) return false;
        uint32_t period_us = 20000; // 50Hz -> 20ms
        if (us < 500) us = 500; if (us > 2500) us = 2500; // 安全夹取
        uint32_t ccr = (uint32_t)((uint64_t)(period_ticks_ + 1) * us / period_us);
        __HAL_TIM_SET_COMPARE(pwm_tim_, pwm_channel_, ccr);
        return true;
    }

    // PWM: 速度模式，-100..+100 rpm 映射 (示例: 1080us -> -100, 1520 -> 0, 1920 -> +100)
    bool setPwmSpeed(float rpm)
    {
        if (rpm < -100.f) rpm = -100.f; if (rpm > 100.f) rpm = 100.f;
        float us = 1520.f + (rpm/100.f) * 400.f; // +-400us 范围
        return setPwmUs((uint16_t)us);
    }

    // PWM: 位置模式，角度映射到 [min_us, max_us]，默认中点 1500us
    bool setPwmPosition(float degrees, float center_us=1500.f, float span_us=500.f)
    {
        // 用户应在外部进行角度范围裁剪；此处仅作线性映射示例
        float us = center_us + (degrees/90.f) * span_us; // +-90deg -> +-span
        if (us < 1000.f) us = 1000.f; if (us > 2000.f) us = 2000.f;
        return setPwmUs((uint16_t)us);
    }

    // 解析反馈帧（放在接收回调里调用）
    void parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d)
    {
        if (!h || !d) return;
        if (h->IDE != CAN_ID_STD) return;
        if (h->StdId != fbk_id_) return;
        meas_.last_ecd = meas_.ecd;
        meas_.ecd = (uint16_t)((d[0] << 8) | d[1]);
        meas_.speed_rpm = (int16_t)((d[2] << 8) | d[3]);
        meas_.given_current = (int16_t)((d[4] << 8) | d[5]);
        meas_.temperature = d[6];
    }

    const Measure& measure() const { return meas_; }
    uint8_t id() const { return id_; }
    uint16_t groupId() const { return group_id_; }
    uint16_t feedbackId() const { return fbk_id_; }
    uint8_t slot() const { return slot_; }

private:
    static inline void put16(uint8_t* p, uint8_t off, int16_t v)
    {
        p[off] = (uint8_t)(v >> 8); p[off+1] = (uint8_t)(v);
    }
    static inline void pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d)
    {
        put16(p,0,a); put16(p,2,b); put16(p,4,c); put16(p,6,d);
    }

private:
    CanBus* can_;
    uint8_t id_;
    uint16_t group_id_;
    uint16_t fbk_id_;
    uint8_t slot_;

    // PWM 绑定
    TIM_HandleTypeDef* pwm_tim_;
    uint32_t pwm_channel_;
    uint32_t period_ticks_;

    // 反馈
    Measure meas_;
};

#endif // GM6020_HPP
