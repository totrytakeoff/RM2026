#pragma once

#include "stdint.h"
#include "arm_math.h"
#include "balance.h"

// 串联腿机械参数 - 需要根据实际机械结构调整
#define SERIAL_THIGH_LEN 0.20f        // 大腿长度 [m] - 调参要点1
#define SERIAL_CALF_LEN 0.20f         // 小腿长度 [m] - 调参要点2  
#define SERIAL_ANKLE_LEN 0.08f        // 踝关节到足底距离 [m] - 调参要点3
#define SERIAL_HIP_HEIGHT 0.30f       // 髋关节离地高度 [m] - 调参要点4
#define SERIAL_LEG_MASS 1.5f          // 单腿质量 [kg] - 调参要点5

// 串联腿关节限位 - 必须严格设置以保护机械结构
#define HIP_MIN_ANGLE -60.0f        // 髋关节最小角度 [deg] - 调参要点6
#define HIP_MAX_ANGLE 60.0f         // 髋关节最大角度 [deg] - 调参要点7
#define KNEE_MIN_ANGLE -150.0f      // 膝关节最小角度 [deg] - 调参要点8  
#define KNEE_MAX_ANGLE 10.0f          // 膝关节最大角度 [deg] - 调参要点9
#define ANKLE_MIN_ANGLE -45.0f      // 踝关节最小角度 [deg] - 调参要点10
#define ANKLE_MAX_ANGLE 45.0f       // 踝关节最大角度 [deg] - 调参要点11

// 串联腿控制参数
#define SERIAL_MAX_LEG_LENGTH 0.45f   // 最大腿长 [m] - 调参要点12
#define SERIAL_MIN_LEG_LENGTH 0.15f   // 最小腿长 [m] - 调参要点13
#define SERIAL_NOMINAL_LEG_LENGTH 0.30f // 标称腿长 [m] - 调参要点14

// 串联腿状态结构体 - 扩展了并联腿的状态
typedef struct {
    // 关节角度和角速度
    float hip_angle;          // 髋关节角度 [rad] - 相对于竖直方向
    float hip_velocity;       // 髋关节角速度 [rad/s]
    float knee_angle;         // 膝关节角度 [rad] - 相对于大腿
    float knee_velocity;      // 膝关节角速度 [rad/s]  
    float ankle_angle;        // 踝关节角度 [rad] - 相对于小腿
    float ankle_velocity;     // 踝关节角速度 [rad/s]
    
    // 末端执行器状态
    float foot_x;             // 足部X坐标 [m] - 相对于髋关节
    float foot_y;             // 足部Y坐标 [m] - 相对于髋关节
    float foot_velocity_x;    // 足部X速度 [m/s]
    float foot_velocity_y;    // 足部Y速度 [m/s]
    
    // 等效单摆参数 - 用于简化控制
    float equivalent_length;  // 等效腿长 [m] - 从髋关节到足底
    float equivalent_angle;   // 等效腿角度 [rad] - 相对于竖直方向
    float equivalent_vel;     // 等效腿角速度 [rad/s]
    
    // 控制目标
    float target_hip_angle;   // 目标髋关节角度 [rad] - 调参要点15
    float target_knee_angle;  // 目标膝关节角度 [rad] - 调参要点16
    float target_ankle_angle; // 目标踝关节角度 [rad] - 调参要点17
    float target_foot_x;      // 目标足部X坐标 [m] - 调参要点18
    float target_foot_y;      // 目标足部Y坐标 [m] - 调参要点19
    
    // 控制输出
    float hip_torque;         // 髋关节力矩 [Nm] - 控制输出
    float knee_torque;        // 膝关节力矩 [Nm] - 控制输出  
    float ankle_torque;       // 踝关节力矩 [Nm] - 控制输出
    
    // 支撑相状态
    uint8_t stance_phase;     // 支撑相标志 [0/1] - 用于步态控制
    float ground_reaction_force; // 地面反作用力 [N] - 调参要点20
    
} SerialLegParam;

// 串联腿控制模式枚举
typedef enum {
    SERIAL_LEG_BALANCE,       // 平衡模式 - 静止或缓慢移动
    SERIAL_LEG_WALKING,       // 行走模式 - 周期性步态
    SERIAL_LEG_TROT,          // 小跑模式 - 对角步态
    SERIAL_LEG_BOUND,         // 跳跃模式 - 双足同时离地
    SERIAL_LEG_RESET          // 复位模式 - 回到初始位置
} SerialLegControlMode_e;

// 串联腿步态参数结构体
typedef struct {
    float swing_time;         // 摆动相持续时间 [s] - 调参要点21
    float stance_time;        // 支撑相持续时间 [s] - 调参要点22
    float step_height;        // 步高 [m] - 调参要点23
    float step_length;        // 步长 [m] - 调参要点24
    float touchdown_velocity; // 触地速度 [m/s] - 调参要点25
    float lift_off_velocity;  // 离地速度 [m/s] - 调参要点26
} SerialGaitParam_s;

// 串联腿LQR增益结构体 - 用于增益调度
typedef struct {
    float k_theta;            // 角度反馈增益 - 调参要点27
    float k_theta_dot;        // 角速度反馈增益 - 调参要点28
    float k_length;           // 腿长反馈增益 - 调参要点29
    float k_length_dot;       // 腿长变化率反馈增益 - 调参要点30
    float k_pitch;            // 机体俯仰角反馈增益 - 调参要点31
    float k_pitch_dot;        // 机体俯仰角速度反馈增益 - 调参要点32
} SerialLQRGains_s;

/**
 * @brief 串联腿正运动学计算 - 从关节角度到末端位置
 * @param leg 串联腿参数结构体
 * @note 使用DH参数法或几何法计算正向运动学
 */
void SerialLegForwardKinematics(SerialLegParam *leg);

/**
 * @brief 串联腿逆运动学计算 - 从末端位置到关节角度
 * @param leg 串联腿参数结构体  
 * @param target_x 目标X坐标 [m]
 * @param target_y 目标Y坐标 [m]
 * @return 逆解是否成功 [0/1]
 * @note 可能存在多解，需要选择最优解
 */
uint8_t SerialLegInverseKinematics(SerialLegParam *leg, float target_x, float target_y);

/**
 * @brief 串联腿雅可比矩阵计算 - 速度映射
 * @param leg 串联腿参数结构体
 * @param J 输出的3x3雅可比矩阵
 * @note J = ∂(x,y,θ)/∂(q1,q2,q3) - 关节空间到任务空间的映射
 */
void SerialLegJacobian(SerialLegParam *leg, float J[3][3]);

/**
 * @brief 串联腿动力学计算 - 使用拉格朗日方程
 * @param leg 串联腿参数结构体
 * @param chassis 底盘参数结构体
 * @param gravity_comp 重力补偿力矩数组 [3]
 * @param coriolis_comp 科氏力补偿力矩数组 [3]
 * @note M(q)q̈ + C(q,q̇)q̇ + G(q) = τ
 */
void SerialLegDynamics(SerialLegParam *leg, ChassisParam *chassis, 
                      float gravity_comp[3], float coriolis_comp[3]);

/**
 * @brief 串联腿VMC控制映射 - 抽象力到关节力矩
 * @param leg 串联腿参数结构体
 * @param F_virtual 虚拟力 [N] - 等效单摆的切向力
 * @param T_virtual 虚拟力矩 [Nm] - 等效单摆的力矩
 * @note 使用雅可比转置映射：τ = J^T * [F_x, F_y, T_z]^T
 */
void SerialLegVMCProject(SerialLegParam *leg, float F_virtual, float T_virtual);

/**
 * @brief 串联腿LQR控制律计算 - 增益调度
 * @param leg 串联腿参数结构体
 * @param chassis 底盘参数结构体
 * @param gains LQR增益结构体
 * @note 根据等效腿长和机体状态调度增益
 */
void SerialLegCalcLQR(SerialLegParam *leg, ChassisParam *chassis, SerialLQRGains_s *gains);

/**
 * @brief 串联腿步态规划 - 生成足部轨迹
 * @param leg 串联腿参数结构体
 * @param gait 步态参数结构体
 * @param phase 当前步态相位 [0, 2π]
 * @param velocity 期望速度 [m/s]
 * @note 生成平滑的摆动相和支撑相轨迹
 */
void SerialLegGaitPlanning(SerialLegParam *leg, SerialGaitParam_s *gait, 
                          float phase, float velocity);

/**
 * @brief 串联腿触地检测 - 判断支撑相
 * @param leg 串联腿参数结构体
 * @param force_threshold 触地力阈值 [N] - 调参要点33
 * @param velocity_threshold 触地速度阈值 [m/s] - 调参要点34
 * @return 触地状态 [0/1]
 * @note 结合力传感器和速度变化率判断
 */
uint8_t SerialLegTouchdownDetection(SerialLegParam *leg, float force_threshold, 
                                   float velocity_threshold);

/**
 * @brief 串联腿状态初始化 - 设置初始参数
 * @param leg 串联腿参数结构体
 * @note 设置默认的关节角度和控制参数
 */
void SerialLegInit(SerialLegParam *leg);

/**
 * @brief 串联腿安全检查 - 限位和力矩保护
 * @param leg 串联腿参数结构体
 * @return 安全状态 [0/1] - 0表示不安全
 * @note 检查关节角度、角速度、力矩是否超限
 */
uint8_t SerialLegSafetyCheck(SerialLegParam *leg);