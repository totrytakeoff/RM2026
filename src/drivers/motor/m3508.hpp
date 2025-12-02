#ifndef M3508_HPP
#define M3508_HPP

//
//  文件: m3508.hpp
//  说明: RoboMaster M3508/C610/C620 底盘电机封装（仅基础 C++，不使用 STL/动态分配）
//
//  功能:
//  - 基于 CAN 的电流控制（组播帧 0x200，单电机反馈 0x201~0x204）
//  - 反馈解析（角度/速度/给定电流/温度）
//  - 速度环控制（PID）
//  - 位置环控制（位置环 + 速度环级联）
//
//  使用说明:
//  - 构造函数传入 CanBus* 与电机 ID (1..4 或 5..8)。
//  - 单电机控制: setCurrent() 会只填写自己的通道并通过 0x200 组播帧发送。
//    若同时控制多台，建议使用 sendCurrentGroup() 以避免不同对象之间互相覆盖。
//
extern "C" {
#include "stm32f4xx_hal.h"
}

#include "../protocol/can_comm.hpp"
#include "../../common/pid/pid_controller.hpp"

/**
 * @brief M3508/C610/C620 底盘电机封装（仅接口声明）
 *
 * 提供基于 CAN 的电流控制与反馈解析接口。实现移至对应的 .cpp 文件。
 */
class M3508 {
public:
    /** 组播帧常量：底盘 4 电机电流控制 */
    static constexpr uint16_t kGroupCurrent = 0x200;
    static constexpr uint16_t kGroupCurrent2 = 0x1FF;

    /** 电机控制模式 */
    enum class ControlMode {
        OPEN_LOOP = 0,      /**< 开环控制（直接电流） */
        SPEED_LOOP = 1,     /**< 速度环控制 */
        POSITION_LOOP = 2   /**< 位置环控制 */
    };

    /** 位置模式 */
    enum class PositionMode {
        MULTI_TURN = 0,     /**< 多圈绝对角（使用 total_angle，单位度） */
        SHORTEST_PATH = 1   /**< 最短路就近角（对机械角取模 [-180,180]） */
    };

    /** 电机反馈结构 */
    struct Measure {
        uint16_t ecd;          /**< 编码器原始值 (0-8191) */
        int16_t  speed_rpm;    /**< 速度 rpm */
        int16_t  given_current;/**< 实际转矩电流 */
        uint8_t  temperature;  /**< 温度 */
        uint16_t last_ecd;     /**< 上次编码器 */
        int32_t  total_angle;  /**< 累计角度（度）*/
        int32_t  total_rounds; /**< 累计圈数 */
    };

    /**
     * @brief 构造函数
     * @param can 底层 CanBus 指针
     * @param motor_id 电机 id (1..4 或 5..8)
     */
    explicit M3508(CanBus* can, uint8_t motor_id);

    /**
     * @brief 初始化电机（注册 CAN 回调）
     * @return true=初始化成功
     */
    bool init();

    /**
     * @brief 单电机电流给定（仅填写本槽位）- 开环控制
     * @param current 给定值，范围 -16384..16384
     * @return true 表示已入队发送
     */
    bool setCurrent(int16_t current);

    /**
     * @brief 设置目标速度（速度环控制）
     * @param target_rpm 目标速度 (rpm)
     */
    void setTargetSpeed(float target_rpm);

    /**
     * @brief 设置目标位置（位置环控制）
     * @param target_angle 目标角度（度）
     */
    void setTargetPosition(float target_angle);

    /**
     * @brief 设置位置模式
     */
    void setPositionMode(PositionMode mode);

    /**
     * @brief 设置位置环输出速度与加速度限制（单位 rpm / rpm/s）
     */
    void setPositionLimits(float speed_limit_rpm, float accel_limit_rpm_s);

    /**
     * @brief 设置低速“速度地板”和位置死区，用于克服静摩擦
     * @param min_speed_floor_rpm 当仍有明显位置误差时，目标速度不会小于该值（按符号保留）
     * @param pos_deadband_deg 认为“已到位”的误差阈值（度），用于允许速度降为0
     */
    void setLowSpeedFloor(float min_speed_floor_rpm, float pos_deadband_deg);

    void setDeadbandTaper(float deadband_deg, float release_deg, float floor_taper_deg);

    /**
     * @brief 设定多圈绝对角目标（并切换到多圈模式）
     */
    void setTargetPositionMultiTurn(float target_angle);

    /**
     * @brief 设定就近角目标（单位度，0..360 任意取模，并切换到就近模式）
     */
    void setTargetPositionShortest(float target_angle);

    /**
     * @brief 更新控制器（在主循环中周期调用）
     * @param dt 时间间隔（秒），默认 0 使用 PID 内部采样时间
     * @return true=控制器更新成功
     */
    bool update(float dt = 0.0f);

    /**
     * @brief 设置控制模式
     * @param mode 控制模式
     */
    void setControlMode(ControlMode mode);

    /**
     * @brief 配置速度环 PID 参数
     * @param kp 比例系数
     * @param ki 积分系数
     * @param kd 微分系数
     */
    void setSpeedPID(float kp, float ki, float kd);

    /**
     * @brief 配置位置环 PID 参数
     * @param kp 比例系数
     * @param ki 积分系数
     * @param kd 微分系数
     */
    void setPositionPID(float kp, float ki, float kd);

    /**
     * @brief 重置控制器（清零 PID 状态和累计角度）
     */
    void reset();

    /**
     * @brief 批量发送 4 通道电流（推荐同时控制多电机时使用）
     */
    static bool sendCurrentGroup(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4);
    static bool sendCurrentGroup2(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4);
    
    /**
     * @brief 批量更新 4 个电机的速度环控制
     * @param m1-m4 电机对象指针
     * @param dt 时间间隔（秒），默认 0 使用 PID 内部采样时间
     * @return true=所有电机更新成功
     */
    static bool updateSpeedGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4, float dt = 0.0f);
    
    /**
     * @brief 批量更新 4 个电机的位置环控制
     * @param m1-m4 电机对象指针
     * @param dt 时间间隔（秒），默认 0 使用 PID 内部采样时间
     * @return true=所有电机更新成功
     */
    static bool updatePositionGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4, float dt = 0.0f);
    
    /**
     * @brief 批量设置 4 个电机的目标速度
     * @param m1-m4 电机对象指针
     * @param s1-s4 目标速度 (rpm)
     */
    static void setSpeedGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4, 
                              float s1, float s2, float s3, float s4);
    
    /**
     * @brief 批量设置 4 个电机的目标位置
     * @param m1-m4 电机对象指针
     * @param p1-p4 目标位置（度）
     */
    static void setPositionGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4,
                                 float p1, float p2, float p3, float p4);
    
    /**
     * @brief 直接批量控制 4 个电机速度（无需创建实例，直接计算并发送）
     * @param can CAN 总线指针
     * @param target1-target4 目标速度 (rpm)
     * @param current1-current4 当前速度 (rpm)
     * @param kp, ki, kd PID 参数
     * @param group 组号：1=ID 1-4 (0x200), 2=ID 5-8 (0x1FF)
     * @return true=发送成功
     * @note 这是简化版本，使用简单 PID 计算，适合快速测试
     */
    static bool controlSpeedDirect(CanBus* can, 
                                   float target1, float target2, float target3, float target4,
                                   float current1, float current2, float current3, float current4,
                                   float kp, float ki, float kd, uint8_t group = 1);
    
    /**
     * @brief 直接批量控制 4 个电机位置（无需创建实例，级联 PID）
     * @param can CAN 总线指针
     * @param target_pos1-4 目标位置（度）
     * @param current_pos1-4 当前位置（度）
     * @param current_spd1-4 当前速度 (rpm)
     * @param pos_kp, pos_ki, pos_kd 位置环 PID 参数
     * @param spd_kp, spd_ki, spd_kd 速度环 PID 参数
     * @param group 组号：1=ID 1-4 (0x200), 2=ID 5-8 (0x1FF)
     * @return true=发送成功
     */
    static bool controlPositionDirect(CanBus* can,
                                      float target_pos1, float target_pos2, float target_pos3, float target_pos4,
                                      float current_pos1, float current_pos2, float current_pos3, float current_pos4,
                                      float current_spd1, float current_spd2, float current_spd3, float current_spd4,
                                      float pos_kp, float pos_ki, float pos_kd,
                                      float spd_kp, float spd_ki, float spd_kd,
                                      uint8_t group = 1);

    /**
     * @brief 解析来自电机的反馈帧（在接收回调或轮询时调用）
     */
    void parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d);

    const Measure& measure() const;     /**< 获取最新测量值 */
    uint8_t id() const;                 /**< 获取电机 id */
    uint16_t feedbackId() const;        /**< 获取反馈报文 ID */
    uint8_t slot() const;               /**< 获取槽位索引（0..3 或 4..7） */
    ControlMode getControlMode() const; /**< 获取当前控制模式 */
    float getCurrentSpeed() const;      /**< 获取当前速度 (rpm) */
    float getCurrentPosition() const;   /**< 获取当前位置（度） */
    int16_t getLastCurrent() const;     /**< 获取最后输出的电流 */

private:
    static void put16(uint8_t* p, uint8_t off, int16_t v);
    static void pack4(uint8_t* p, int16_t a,int16_t b,int16_t c,int16_t d);
    
    /**
     * @brief CAN 接收回调函数（静态）
     */
    static void canRxCallback(const CAN_RxHeaderTypeDef* header, const uint8_t* data, void* user);
    
    /**
     * @brief 更新累计角度
     */
    void updateTotalAngle();

private:
    CanBus* can_;              /**< CAN 总线指针 */
    uint8_t id_;               /**< 电机 ID */
    uint8_t slot_;             /**< 槽位索引 */
    uint16_t fbk_id_;          /**< 反馈帧 ID */
    Measure meas_;             /**< 测量数据 */
    
    ControlMode mode_;         /**< 控制模式 */
    PositionMode position_mode_;/**< 位置模式 */
    float target_speed_;       /**< 目标速度 (rpm) */
    float target_position_;    /**< 目标位置（度） */
    int16_t output_current_;   /**< 输出电流 */
    
    PIDController speed_pid_;  /**< 速度环 PID */
    PIDController pos_pid_;    /**< 位置环PID */
    
    bool initialized_;         /**< 初始化标志 */
    bool has_feedback_;        /**< 已收到反馈 */
    bool pos_aligned_;         /**< 位置对齐完成 */
    float last_target_speed_;  /**< 上周期目标速度 (rpm) */
    float speed_limit_rpm_;    /**< 目标速度限幅 */
    float accel_limit_rpm_s_;  /**< 目标加速度限幅 */
    float min_speed_floor_rpm_;/**< 低速速度地板 (rpm) */
    float pos_deadband_deg_;   /**< 位置死区 (deg) */
    float pos_deadband_release_deg_;
    float floor_taper_deg_;
    bool pos_hold_;
};

#endif // M3508_HPP
