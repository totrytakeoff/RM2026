/**
 * @file    pid_controller.hpp
 * @brief   通用PID控制器类
 * @author  AI Assistant
 * @date    2025
 * 
 * @description
 * 提供完整的PID控制算法实现，支持：
 * - 标准PID控制（比例、积分、微分）
 * - 输出限制
 * - 积分抗饱和（Anti-Windup）
 * - 微分滤波
 * - 采样时间配置
 * - 自动/手动模式切换
 * 
 * 使用示例：
 * @code
 * PIDController pid(1.0f, 0.1f, 0.05f);  // Kp, Ki, Kd
 * pid.setOutputLimits(-100.0f, 100.0f);
 * pid.setSampleTime(0.02f);  // 20ms
 * 
 * float output = pid.compute(setpoint, measured_value);
 * @endcode
 */

#ifndef PID_CONTROLLER_HPP
#define PID_CONTROLLER_HPP

#include <stdint.h>

/**
 * @brief 通用PID控制器类
 */
class PIDController {
public:
    /**
     * @brief PID控制模式
     */
    enum class Mode {
        MANUAL = 0,    // 手动模式（不计算PID）
        AUTOMATIC = 1  // 自动模式（正常PID计算）
    };

    /**
     * @brief PID控制方向
     */
    enum class Direction {
        DIRECT = 0,    // 正向控制（误差为正时输出为正）
        REVERSE = 1    // 反向控制（误差为正时输出为负）
    };

public:
    /**
     * @brief 构造函数
     * @param kp 比例系数
     * @param ki 积分系数
     * @param kd 微分系数
     */
    PIDController(float kp = 0.0f, float ki = 0.0f, float kd = 0.0f);

    /**
     * @brief 计算PID输出
     * @param setpoint 目标值
     * @param input 当前测量值
     * @return PID控制输出
     */
    float compute(float setpoint, float input);

    /**
     * @brief 计算PID输出（使用自定义时间间隔）
     * @param setpoint 目标值
     * @param input 当前测量值
     * @param dt 时间间隔（秒）
     * @return PID控制输出
     */
    float compute(float setpoint, float input, float dt);

    /**
     * @brief 设置PID参数
     * @param kp 比例系数
     * @param ki 积分系数
     * @param kd 微分系数
     */
    void setTunings(float kp, float ki, float kd);

    /**
     * @brief 设置输出限制
     * @param min 最小输出值
     * @param max 最大输出值
     */
    void setOutputLimits(float min, float max);

    /**
     * @brief 设置采样时间
     * @param sample_time_sec 采样时间（秒）
     */
    void setSampleTime(float sample_time_sec);

    /**
     * @brief 设置控制模式
     * @param mode 控制模式（MANUAL/AUTOMATIC）
     */
    void setMode(Mode mode);

    /**
     * @brief 设置控制方向
     * @param direction 控制方向（DIRECT/REVERSE）
     */
    void setDirection(Direction direction);

    /**
     * @brief 启用/禁用积分抗饱和
     * @param enable true=启用，false=禁用
     */
    void setAntiWindup(bool enable);

    /**
     * @brief 设置微分滤波系数
     * @param alpha 滤波系数 (0.0-1.0)，0=无滤波，越大滤波越强
     * @note alpha = dt / (dt + tau)，其中tau是滤波时间常数
     */
    void setDerivativeFilter(float alpha);

    /**
     * @brief 重置PID控制器
     * @note 清空积分项、上次误差等内部状态
     */
    void reset();

    /**
     * @brief 获取当前误差
     * @return 当前误差值
     */
    float getError() const { return error_; }

    /**
     * @brief 获取比例项输出
     * @return 比例项输出值
     */
    float getProportional() const { return p_term_; }

    /**
     * @brief 获取积分项输出
     * @return 积分项输出值
     */
    float getIntegral() const { return i_term_; }

    /**
     * @brief 获取微分项输出
     * @return 微分项输出值
     */
    float getDerivative() const { return d_term_; }

    /**
     * @brief 获取最后的输出值
     * @return 最后的输出值
     */
    float getOutput() const { return output_; }

    /**
     * @brief 获取当前Kp值
     * @return Kp系数
     */
    float getKp() const { return kp_; }

    /**
     * @brief 获取当前Ki值
     * @return Ki系数
     */
    float getKi() const { return ki_; }

    /**
     * @brief 获取当前Kd值
     * @return Kd系数
     */
    float getKd() const { return kd_; }

    /**
     * @brief 检查是否处于自动模式
     * @return true=自动模式，false=手动模式
     */
    bool isAutomatic() const { return mode_ == Mode::AUTOMATIC; }

private:
    // PID参数
    float kp_;              // 比例系数
    float ki_;              // 积分系数
    float kd_;              // 微分系数
    
    // 内部状态
    float error_;           // 当前误差
    float last_error_;      // 上次误差
    float integral_;        // 积分累积值
    float derivative_;      // 微分值
    float last_input_;      // 上次输入值（用于微分on measurement）
    
    // 输出项
    float p_term_;          // 比例项输出
    float i_term_;          // 积分项输出
    float d_term_;          // 微分项输出
    float output_;          // 最终输出
    
    // 限制参数
    float out_min_;         // 输出最小值
    float out_max_;         // 输出最大值
    
    // 时间参数
    float sample_time_;     // 采样时间（秒）
    uint32_t last_time_;    // 上次计算时间（ms）
    
    // 控制参数
    Mode mode_;             // 控制模式
    Direction direction_;   // 控制方向
    bool anti_windup_;      // 积分抗饱和使能
    
    // 微分滤波
    float d_filter_alpha_;  // 微分滤波系数
    float filtered_derivative_; // 滤波后的微分值
    
    // 标志位
    bool first_run_;        // 首次运行标志

    /**
     * @brief 限幅函数
     * @param value 输入值
     * @param min 最小值
     * @param max 最大值
     * @return 限幅后的值
     */
    float constrain(float value, float min, float max);
};

#endif // PID_CONTROLLER_HPP
