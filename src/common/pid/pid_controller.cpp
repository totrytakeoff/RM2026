/**
 * @file    pid_controller.cpp
 * @brief   通用PID控制器类实现
 * @author  AI Assistant
 * @date    2025
 */

#include "pid_controller.hpp"
#include "stm32f4xx_hal.h"
#include <cmath>

/**
 * @brief 构造函数
 */
PIDController::PIDController(float kp, float ki, float kd)
    : kp_(kp)
    , ki_(ki)
    , kd_(kd)
    , error_(0.0f)
    , last_error_(0.0f)
    , integral_(0.0f)
    , derivative_(0.0f)
    , last_input_(0.0f)
    , p_term_(0.0f)
    , i_term_(0.0f)
    , d_term_(0.0f)
    , output_(0.0f)
    , out_min_(-100.0f)
    , out_max_(100.0f)
    , sample_time_(0.02f)  // 默认20ms
    , last_time_(0)
    , mode_(Mode::AUTOMATIC)
    , direction_(Direction::DIRECT)
    , anti_windup_(true)   // 默认启用抗饱和
    , d_filter_alpha_(0.0f) // 默认不滤波
    , filtered_derivative_(0.0f)
    , first_run_(true)
{
}

/**
 * @brief 计算PID输出（使用内部采样时间）
 */
float PIDController::compute(float setpoint, float input) {
    // 如果是手动模式，直接返回当前输出
    if (mode_ == Mode::MANUAL) {
        return output_;
    }
    
    // 检查是否到达采样时间
    uint32_t now = HAL_GetTick();
    float dt = (now - last_time_) / 1000.0f;  // 转换为秒
    
    // 如果是首次运行或时间间隔足够
    if (first_run_ || dt >= sample_time_) {
        return compute(setpoint, input, dt);
    }
    
    // 时间间隔不够，返回上次输出
    return output_;
}

/**
 * @brief 计算PID输出（使用自定义时间间隔）
 */
float PIDController::compute(float setpoint, float input, float dt) {
    // 如果是手动模式，直接返回当前输出
    if (mode_ == Mode::MANUAL) {
        return output_;
    }
    
    // 如果时间间隔无效，使用默认采样时间
    if (dt <= 0.0f || dt > 1.0f) {
        dt = sample_time_;
    }
    
    // 计算误差
    error_ = setpoint - input;
    
    // 如果是反向控制，反转误差
    if (direction_ == Direction::REVERSE) {
        error_ = -error_;
    }
    
    // === 比例项 ===
    p_term_ = kp_ * error_;
    
    // === 积分项 ===
    // 累加积分（使用梯形积分）
    if (first_run_) {
        integral_ += ki_ * error_ * dt;
    } else {
        integral_ += ki_ * (error_ + last_error_) * 0.5f * dt;
    }
    
    // 积分抗饱和（Back-calculation方法）
    if (anti_windup_) {
        // 先计算未限幅的输出
        float unclamped_output = p_term_ + integral_;
        
        // 限幅
        float clamped_output = constrain(unclamped_output, out_min_, out_max_);
        
        // 如果发生饱和，调整积分项
        if (unclamped_output != clamped_output) {
            integral_ = clamped_output - p_term_;
        }
    } else {
        // 简单限幅积分项
        float max_integral = out_max_ - p_term_;
        float min_integral = out_min_ - p_term_;
        integral_ = constrain(integral_, min_integral, max_integral);
    }
    
    i_term_ = integral_;
    
    // === 微分项 ===
    // 使用 derivative on measurement 避免setpoint突变导致的微分冲击
    if (first_run_) {
        derivative_ = 0.0f;
    } else {
        // 计算测量值的变化率（取负是因为我们要的是error的导数）
        derivative_ = -kd_ * (input - last_input_) / dt;
    }
    
    // 微分滤波（低通滤波）
    if (d_filter_alpha_ > 0.0f) {
        if (first_run_) {
            filtered_derivative_ = derivative_;
        } else {
            filtered_derivative_ = d_filter_alpha_ * derivative_ + 
                                  (1.0f - d_filter_alpha_) * filtered_derivative_;
        }
        d_term_ = filtered_derivative_;
    } else {
        d_term_ = derivative_;
    }
    
    // === 计算总输出 ===
    output_ = p_term_ + i_term_ + d_term_;
    
    // 输出限幅
    output_ = constrain(output_, out_min_, out_max_);
    
    // 保存状态
    last_error_ = error_;
    last_input_ = input;
    last_time_ = HAL_GetTick();
    first_run_ = false;
    
    return output_;
}

/**
 * @brief 设置PID参数
 */
void PIDController::setTunings(float kp, float ki, float kd) {
    // 确保参数非负
    if (kp < 0.0f || ki < 0.0f || kd < 0.0f) {
        return;
    }
    
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

/**
 * @brief 设置输出限制
 */
void PIDController::setOutputLimits(float min, float max) {
    if (min >= max) {
        return;
    }
    
    out_min_ = min;
    out_max_ = max;
    
    // 如果已经在运行，限制当前输出和积分项
    if (!first_run_) {
        output_ = constrain(output_, out_min_, out_max_);
        integral_ = constrain(integral_, out_min_, out_max_);
    }
}

/**
 * @brief 设置采样时间
 */
void PIDController::setSampleTime(float sample_time_sec) {
    if (sample_time_sec > 0.0f) {
        sample_time_ = sample_time_sec;
    }
}

/**
 * @brief 设置控制模式
 */
void PIDController::setMode(Mode mode) {
    // 从手动切换到自动时，进行平滑切换
    if (mode == Mode::AUTOMATIC && mode_ == Mode::MANUAL) {
        reset();
    }
    mode_ = mode;
}

/**
 * @brief 设置控制方向
 */
void PIDController::setDirection(Direction direction) {
    direction_ = direction;
}

/**
 * @brief 启用/禁用积分抗饱和
 */
void PIDController::setAntiWindup(bool enable) {
    anti_windup_ = enable;
}

/**
 * @brief 设置微分滤波系数
 */
void PIDController::setDerivativeFilter(float alpha) {
    if (alpha >= 0.0f && alpha <= 1.0f) {
        d_filter_alpha_ = alpha;
    }
}

/**
 * @brief 重置PID控制器
 */
void PIDController::reset() {
    error_ = 0.0f;
    last_error_ = 0.0f;
    integral_ = 0.0f;
    derivative_ = 0.0f;
    last_input_ = 0.0f;
    filtered_derivative_ = 0.0f;
    
    p_term_ = 0.0f;
    i_term_ = 0.0f;
    d_term_ = 0.0f;
    output_ = 0.0f;
    
    first_run_ = true;
    last_time_ = HAL_GetTick();
}

/**
 * @brief 限幅函数
 */
float PIDController::constrain(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}
