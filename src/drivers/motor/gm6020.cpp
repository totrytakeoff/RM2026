/**
 * @file gm6020.cpp
 * @brief GM6020 类实现
 */

extern "C" {
#include "stm32f4xx_hal.h"
}

#include "gm6020.hpp"

GM6020::GM6020(CanBus* can, uint8_t motor_id)
    : can_(can), id_(motor_id), pwm_tim_(0), pwm_channel_(0), period_ticks_(0)
{
    if (id_ >= 1 && id_ <= 4) { group_id_ = 0x1FF; slot_ = id_ - 1; }
    else if (id_ >= 5 && id_ <= 7) { group_id_ = 0x2FF; slot_ = id_ - 5; }
    else { group_id_ = 0x1FF; slot_ = 0; }
    fbk_id_ = 0x204 + id_;
    meas_ = {0};
}

void GM6020::attachPwm(TIM_HandleTypeDef* tim, uint32_t channel, uint32_t period_ticks)
{
    pwm_tim_ = tim; pwm_channel_ = channel; period_ticks_ = period_ticks;
}

bool GM6020::setCurrent(int16_t current)
{
    if (!can_) return false;
    uint8_t p[8] = {0};
    put16(p, slot_*2, current);
    return can_->sendStd(group_id_, p, 8) == HAL_OK;
}

bool GM6020::setVoltage(int16_t voltage)
{
    if (!can_) return false;
    uint8_t p[8] = {0};
    put16(p, slot_*2, voltage);
    return can_->sendStd(group_id_, p, 8) == HAL_OK;
}

bool GM6020::sendCurrentGroup(CanBus* can, uint16_t group_id,
                               int16_t i1, int16_t i2, int16_t i3, int16_t i4)
{
    if (!can) return false;
    uint8_t p[8]; pack4(p, i1,i2,i3,i4);
    return can->sendStd(group_id, p, 8) == HAL_OK;
}

bool GM6020::sendVoltageGroup(CanBus* can, uint16_t group_id,
                               int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    if (!can) return false;
    uint8_t p[8]; pack4(p, v1,v2,v3,v4);
    return can->sendStd(group_id, p, 8) == HAL_OK;
}

bool GM6020::setPwmUs(uint16_t us)
{
    if (!pwm_tim_ || period_ticks_ == 0) return false;
    uint32_t period_us = 20000; // 50Hz -> 20ms
    if (us < 500) us = 500;
    if (us > 2500) us = 2500; // 安全夹取
    uint32_t ccr = (uint32_t)((uint64_t)(period_ticks_ + 1) * us / period_us);
    __HAL_TIM_SET_COMPARE(pwm_tim_, pwm_channel_, ccr);
    return true;
}

bool GM6020::setPwmSpeed(float rpm)
{
    if (rpm < -100.f) rpm = -100.f;
    if (rpm > 100.f) rpm = 100.f;
    float us = 1520.f + (rpm/100.f) * 400.f; // +-400us 范围
    return setPwmUs((uint16_t)us);
}

bool GM6020::setPwmPosition(float degrees, float center_us, float span_us)
{
    float us = center_us + (degrees/90.f) * span_us; // +-90deg -> +-span
    if (us < 1000.f) us = 1000.f;
    if (us > 2000.f) us = 2000.f;
    return setPwmUs((uint16_t)us);
}

void GM6020::parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d)
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

const GM6020::Measure& GM6020::measure() const { return meas_; }
uint8_t GM6020::id() const { return id_; }
uint16_t GM6020::groupId() const { return group_id_; }
uint16_t GM6020::feedbackId() const { return fbk_id_; }
uint8_t GM6020::slot() const { return slot_; }

void GM6020::put16(uint8_t* p, uint8_t off, int16_t v)
{
    p[off] = (uint8_t)(v >> 8); p[off+1] = (uint8_t)(v);
}

void GM6020::pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d)
{
    put16(p,0,a); put16(p,2,b); put16(p,4,c); put16(p,6,d);
}
