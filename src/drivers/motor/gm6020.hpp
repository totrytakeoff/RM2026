#ifndef GM6020_HPP
#define GM6020_HPP

//
//  文件: gm6020.hpp
//  说明: RoboMaster GM6020 电机封装
//
//  功能:
//  - 基于 CAN 的电流控制/电压控制 (组播帧 0x1FF/0x2FF) 注意文档中写的控制电流是 0x1FE/0x2FE 但是实测无效,只有0x1FF/0x2FF能驱动电机
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

/**
 * @brief GM6020 电机封装（接口声明）
 *
 * 提供 CAN 电流/电压和 PWM 控制接口；实现移至 gm6020.cpp。
 */
class GM6020 {
public:
    /** 电机反馈数据结构 */
    struct Measure {
        uint16_t ecd;          /**< 编码器 */
        int16_t  speed_rpm;    /**< 速度 */
        int16_t  given_current;/**< 给定电流 */
        uint8_t  temperature;  /**< 温度 */
        uint16_t last_ecd;     /**< 上次编码器 */
    };

    /**
     * @brief 构造函数
     * @param can 底层 CanBus 指针
     * @param motor_id 电机 id (1..7)
     */
    GM6020(CanBus* can, uint8_t motor_id);

    /**
     * @brief 绑定 PWM 输出（要求外部定时器已配置为 50Hz PWM）
     * @param tim TIM HAL 句柄
     * @param channel TIM 通道 (TIM_CHANNEL_x)
     * @param period_ticks 自动重载寄存器周期值（Period）
     */
    void attachPwm(TIM_HandleTypeDef* tim, uint32_t channel, uint32_t period_ticks);

    bool setCurrent(int16_t current); /**< 发送单电机电流，其他通道置 0 */
    bool setVoltage(int16_t voltage); /**< 发送单电机电压，其他通道置 0 */

    static bool sendCurrentGroup(CanBus* can, uint16_t group_id,
                                 int16_t i1, int16_t i2, int16_t i3, int16_t i4);

    static bool sendVoltageGroup(CanBus* can, uint16_t group_id,
                                 int16_t v1, int16_t v2, int16_t v3, int16_t v4);

    bool setPwmUs(uint16_t us);                         /**< 直接设置脉宽（us） */
    bool setPwmSpeed(float rpm);                        /**< PWM 速度映射示例 */
    bool setPwmPosition(float degrees, float center_us=1500.f, float span_us=500.f);

    void parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d);

    const Measure& measure() const;
    uint8_t id() const;
    uint16_t groupId() const;
    uint16_t feedbackId() const;
    uint8_t slot() const;

private:
    static void put16(uint8_t* p, uint8_t off, int16_t v);
    static void pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d);

private:
    CanBus* can_;
    uint8_t id_;
    uint16_t group_id_;
    uint16_t fbk_id_;
    uint8_t slot_;

    /* PWM 绑定 */
    TIM_HandleTypeDef* pwm_tim_;
    uint32_t pwm_channel_;
    uint32_t period_ticks_;

    /* 反馈 */
    Measure meas_;
};

#endif // GM6020_HPP
