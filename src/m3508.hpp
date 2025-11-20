#ifndef M3508_HPP
#define M3508_HPP

//
//  文件: m3508.hpp
//  说明: RoboMaster M3508/C610/C620 底盘电机封装（仅基础 C++，不使用 STL/动态分配）
//
//  功能:
//  - 基于 CAN 的电流控制（组播帧 0x200，单电机反馈 0x201~0x204）
//  - 反馈解析（角度/速度/给定电流/温度）可选
//
//  使用说明:
//  - 构造函数传入 CanBus* 与电机 ID (1..4)。
//  - 单电机控制: setCurrent() 会只填写自己的通道并通过 0x200 组播帧发送。
//    若同时控制多台，建议使用 sendCurrentGroup() 以避免不同对象之间互相覆盖。
//
extern "C" {
#include "stm32f4xx_hal.h"
}

#include "can_comm.hpp"

class M3508 {
public:
    // 组播帧常量
    static constexpr uint16_t kGroupCurrent = 0x200;   // 底盘 4 电机电流控制组播

    // 电机反馈结构
    struct Measure {
        uint16_t ecd;          // 编码器
        int16_t  speed_rpm;    // 速度 rpm
        int16_t  given_current;// 实际转矩电流
        uint8_t  temperature;  // 温度
        uint16_t last_ecd;     // 上次编码器
    };

    // 构造: id=1..4 -> 槽位 0..3，反馈 ID = 0x200 + id
    explicit M3508(CanBus* can, uint8_t motor_id)
        : can_(can), id_(motor_id), slot_(0), fbk_id_(0x200 + motor_id)
    {
        if (id_ >= 1 && id_ <= 4) slot_ = id_ - 1; else slot_ = 0;
        meas_ = {0};
    }

    // 单电机电流给定，范围: -16384 ~ 16384
    bool setCurrent(int16_t current)
    {
        if (!can_) return false;
        uint8_t p[8] = {0};
        put16(p, slot_*2, current);
        return can_->sendStd(kGroupCurrent, p, 8) == HAL_OK;
    }

    // 批量发送 4 通道电流（推荐用于同时控制 4 台底盘电机）
    static bool sendCurrentGroup(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4)
    {
        if (!can) return false;
        uint8_t p[8]; pack4(p, i1,i2,i3,i4);
        return can->sendStd(kGroupCurrent, p, 8) == HAL_OK;
    }

    // 解析反馈帧（在接收回调或轮询中调用）
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
    uint8_t slot_;
    uint16_t fbk_id_;
    Measure meas_;
};

#endif // M3508_HPP
