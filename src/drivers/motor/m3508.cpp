/**
 * @file m3508.cpp
 * @brief M3508 类实现
 */

extern "C" {
#include "stm32f4xx_hal.h"
}

#include "m3508.hpp"

M3508::M3508(CanBus* can, uint8_t motor_id)
    : can_(can), id_(motor_id), slot_(0), fbk_id_(0x200 + motor_id)
{
    if (id_ >= 1 && id_ <= 4) slot_ = id_ - 1; else slot_ = 0;
    meas_ = {0};
}

bool M3508::setCurrent(int16_t current)
{
    if (!can_) return false;
    uint8_t p[8] = {0};
    put16(p, slot_*2, current);
    return can_->sendStd(kGroupCurrent, p, 8) == HAL_OK;
}

bool M3508::sendCurrentGroup(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4)
{
    if (!can) return false;
    uint8_t p[8]; pack4(p, i1,i2,i3,i4);
    return can->sendStd(kGroupCurrent, p, 8) == HAL_OK;
}
bool M3508::sendCurrentGroup2(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4)
{
    if (!can) return false;
    uint8_t p[8]; pack4(p, i1,i2,i3,i4);
    return can->sendStd(kGroupCurrent2, p, 8) == HAL_OK;
}

void M3508::parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d)
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

const M3508::Measure& M3508::measure() const { return meas_; }
uint8_t M3508::id() const { return id_; }
uint16_t M3508::feedbackId() const { return fbk_id_; }
uint8_t M3508::slot() const { return slot_; }

void M3508::put16(uint8_t* p, uint8_t off, int16_t v)
{
    p[off] = (uint8_t)(v >> 8); p[off+1] = (uint8_t)(v);
}

void M3508::pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d)
{
    put16(p,0,a); put16(p,2,b); put16(p,4,c); put16(p,6,d);
}
