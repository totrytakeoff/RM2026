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

/**
 * @brief M3508/C610/C620 底盘电机封装（仅接口声明）
 *
 * 提供基于 CAN 的电流控制与反馈解析接口。实现移至对应的 .cpp 文件。
 */
class M3508 {
public:
    /** 组播帧常量：底盘 4 电机电流控制 */
    static constexpr uint16_t kGroupCurrent = 0x200;

    /** 电机反馈结构 */
    struct Measure {
        uint16_t ecd;          /**< 编码器 */
        int16_t  speed_rpm;    /**< 速度 rpm */
        int16_t  given_current;/**< 实际转矩电流 */
        uint8_t  temperature;  /**< 温度 */
        uint16_t last_ecd;     /**< 上次编码器 */
    };

    /**
     * @brief 构造函数
     * @param can 底层 CanBus 指针
     * @param motor_id 电机 id (1..4)
     */
    explicit M3508(CanBus* can, uint8_t motor_id);

    /**
     * @brief 单电机电流给定（仅填写本槽位）
     * @param current 给定值，范围 -16384..16384
     * @return true 表示已入队发送
     */
    bool setCurrent(int16_t current);

    /**
     * @brief 批量发送 4 通道电流（推荐同时控制多电机时使用）
     */
    static bool sendCurrentGroup(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4);

    /**
     * @brief 解析来自电机的反馈帧（在接收回调或轮询时调用）
     */
    void parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d);

    const Measure& measure() const; /**< 获取最新测量值 */
    uint8_t id() const;            /**< 获取电机 id */
    uint16_t feedbackId() const;   /**< 获取反馈报文 ID */
    uint8_t slot() const;          /**< 获取槽位索引（0..3） */

private:
    static void put16(uint8_t* p, uint8_t off, int16_t v);
    static void pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d);

private:
    CanBus* can_;
    uint8_t id_;
    uint8_t slot_;
    uint16_t fbk_id_;
    Measure meas_;
};

#endif // M3508_HPP
