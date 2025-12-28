/**
 * @file serial_leg.c
 * @brief 串联腿控制算法实现 - 基于并联腿控制策略改造
 * @details 主要改进点：
 * 1. 从五连杆闭链改为开链串联结构
 * 2. 增加步态规划和相位控制  
 * 3. 增强触地检测和状态切换
 * 4. 优化关节空间和任务空间的映射
 */

#include "serial_leg.h"
#include "user_lib.h"
#include "arm_math.h"
#include <math.h>

/**
 * @brief 串联腿正运动学计算 - 3连杆串联机构
 * @param leg 串联腿参数结构体
 * @note 使用几何法计算正向运动学
 *       坐标系：髋关节为原点，X轴向前，Y轴向上
 *       关节角度定义：相对于前一连杆的角度
 */
void SerialLegForwardKinematics(SerialLegParam *leg)
{
    float hip = leg->hip_angle;
    float knee = leg->knee_angle;  
    float ankle = leg->ankle_angle;
    
    // 计算各关节点坐标
    float x_knee = SERIAL_THIGH_LEN * sinf(hip);
    float y_knee = SERIAL_THIGH_LEN * cosf(hip);
    
    float x_ankle = x_knee + SERIAL_CALF_LEN * sinf(hip + knee);
    float y_ankle = y_knee + SERIAL_CALF_LEN * cosf(hip + knee);
    
    // 足部末端坐标（考虑踝关节）
    leg->foot_x = x_ankle + SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    leg->foot_y = y_ankle + SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    
    // 等效单摆参数 - 用于简化控制
    leg->equivalent_length = sqrtf(leg->foot_x * leg->foot_x + leg->foot_y * leg->foot_y);
    leg->equivalent_angle = atan2f(leg->foot_x, leg->foot_y);  // 相对于竖直方向
    
    // 计算末端速度（雅可比映射）
    float J11 = SERIAL_THIGH_LEN * cosf(hip) + SERIAL_CALF_LEN * cosf(hip + knee) + SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    float J12 = SERIAL_CALF_LEN * cosf(hip + knee) + SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    float J13 = SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    
    float J21 = -SERIAL_THIGH_LEN * sinf(hip) - SERIAL_CALF_LEN * sinf(hip + knee) - SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    float J22 = -SERIAL_CALF_LEN * sinf(hip + knee) - SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    float J23 = -SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    
    leg->foot_velocity_x = J11 * leg->hip_velocity + J12 * leg->knee_velocity + J13 * leg->ankle_velocity;
    leg->foot_velocity_y = J21 * leg->hip_velocity + J22 * leg->knee_velocity + J23 * leg->ankle_velocity;
    
    // 等效角速度
    float eq_J1 = leg->foot_x / leg->equivalent_length;
    float eq_J2 = leg->foot_y / leg->equivalent_length;
    leg->equivalent_vel = (eq_J1 * leg->foot_velocity_x + eq_J2 * leg->foot_velocity_y) / leg->equivalent_length;
}

/**
 * @brief 串联腿逆运动学计算 - 解析法求解
 * @param leg 串联腿参数结构体
 * @param target_x 目标X坐标 [m]
 * @param target_y 目标Y坐标 [m]
 * @return 逆解是否成功 [0/1]
 * @note 3连杆逆解可能存在多解，选择能量最优解
 */
uint8_t SerialLegInverseKinematics(SerialLegParam *leg, float target_x, float target_y)
{
    // 简化：假设踝关节保持垂直（ankle = -hip - knee）
    // 这样可以将3连杆简化为2连杆问题
    
    float L_total = sqrtf(target_x * target_x + target_y * target_y);
    
    // 检查可达性
    if (L_total > (SERIAL_THIGH_LEN + SERIAL_CALF_LEN + SERIAL_ANKLE_LEN) || 
        L_total < fabsf(SERIAL_THIGH_LEN - SERIAL_CALF_LEN - SERIAL_ANKLE_LEN)) {
        return 0;  // 不可达
    }
    
    // 使用几何法求解2连杆逆运动学
    float L_eff = L_total - SERIAL_ANKLE_LEN;  // 有效长度（不考虑踝关节）
    
    // 求解膝关节角度（使用余弦定理）
    float cos_knee = (L_eff * L_eff - SERIAL_THIGH_LEN * SERIAL_THIGH_LEN - SERIAL_CALF_LEN * SERIAL_CALF_LEN) / 
                     (2.0f * SERIAL_THIGH_LEN * SERIAL_CALF_LEN);
    
    if (cos_knee > 1.0f || cos_knee < -1.0f) {
        return 0;  // 无解
    }
    
    float knee_angle = acosf(cos_knee);
    
    // 求解髋关节角度
    float alpha = atan2f(target_y, target_x);  // 目标角度
    float beta = acosf((L_eff * L_eff + SERIAL_THIGH_LEN * SERIAL_THIGH_LEN - SERIAL_CALF_LEN * SERIAL_CALF_LEN) / 
                       (2.0f * L_eff * SERIAL_THIGH_LEN));
    
    float hip_angle = alpha - beta;
    
    // 踝关节保持垂直
    float ankle_angle = -hip_angle - knee_angle;
    
    // 检查关节限位
    if (hip_angle < HIP_MIN_ANGLE * DEGREE_2_RAD || hip_angle > HIP_MAX_ANGLE * DEGREE_2_RAD ||
        knee_angle < KNEE_MIN_ANGLE * DEGREE_2_RAD || knee_angle > KNEE_MAX_ANGLE * DEGREE_2_RAD ||
        ankle_angle < ANKLE_MIN_ANGLE * DEGREE_2_RAD || ankle_angle > ANKLE_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 超出限位
    }
    
    // 应用解
    leg->target_hip_angle = hip_angle;
    leg->target_knee_angle = knee_angle;
    leg->target_ankle_angle = ankle_angle;
    
    return 1;  // 求解成功
}

/**
 * @brief 串联腿雅可比矩阵计算
 * @param leg 串联腿参数结构体
 * @param J 输出的3x3雅可比矩阵 - 关节空间到任务空间的映射
 * @note J = ∂(x,y,θ)/∂(q1,q2,q3)
 */
void SerialLegJacobian(SerialLegParam *leg, float J[3][3])
{
    float hip = leg->hip_angle;
    float knee = leg->knee_angle;
    float ankle = leg->ankle_angle;
    
    // 位置雅可比 - 足部坐标对关节角度的偏导
    J[0][0] = SERIAL_THIGH_LEN * cosf(hip) + SERIAL_CALF_LEN * cosf(hip + knee) + SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    J[0][1] = SERIAL_CALF_LEN * cosf(hip + knee) + SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    J[0][2] = SERIAL_ANKLE_LEN * cosf(hip + knee + ankle);
    
    J[1][0] = -SERIAL_THIGH_LEN * sinf(hip) - SERIAL_CALF_LEN * sinf(hip + knee) - SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    J[1][1] = -SERIAL_CALF_LEN * sinf(hip + knee) - SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    J[1][2] = -SERIAL_ANKLE_LEN * sinf(hip + knee + ankle);
    
    // 姿态雅可比 - 足部角度对关节角度的偏导
    J[2][0] = 1.0f;
    J[2][1] = 1.0f;
    J[2][2] = 1.0f;
}

/**
 * @brief 串联腿动力学计算 - 使用简化模型
 * @param leg 串联腿参数结构体
 * @param chassis 底盘参数结构体
 * @param gravity_comp 重力补偿力矩数组 [3] - 输出
 * @param coriolis_comp 科氏力补偿力矩数组 [3] - 输出
 * @note M(q)q̈ + C(q,q̇)q̇ + G(q) = τ - 使用简化动力学模型
 */
void SerialLegDynamics(SerialLegParam *leg, ChassisParam *chassis, 
                      float gravity_comp[3], float coriolis_comp[3])
{
    float hip = leg->hip_angle;
    float knee = leg->knee_angle;
    float ankle = leg->ankle_angle;
    
    float hip_vel = leg->hip_velocity;
    float knee_vel = leg->knee_velocity;
    float ankle_vel = leg->ankle_velocity;
    
    // 简化重力补偿 - 基于连杆质心位置
    float m_thigh = SERIAL_LEG_MASS * 0.5f;      // 大腿质量
    float m_calf = SERIAL_LEG_MASS * 0.3f;       // 小腿质量  
    float m_ankle = SERIAL_LEG_MASS * 0.2f;      // 踝关节质量
    
    // 重力补偿力矩
    gravity_comp[0] = m_thigh * 9.81f * SERIAL_THIGH_LEN * 0.5f * sinf(hip) +
                      m_calf * 9.81f * (SERIAL_THIGH_LEN * sinf(hip) + SERIAL_CALF_LEN * 0.5f * sinf(hip + knee)) +
                      m_ankle * 9.81f * (SERIAL_THIGH_LEN * sinf(hip) + SERIAL_CALF_LEN * sinf(hip + knee));
    
    gravity_comp[1] = m_calf * 9.81f * SERIAL_CALF_LEN * 0.5f * sinf(hip + knee) +
                      m_ankle * 9.81f * SERIAL_CALF_LEN * sinf(hip + knee);
    
    gravity_comp[2] = 0.0f;  // 踝关节重力补偿较小，可忽略
    
    // 简化科氏力补偿 - 基于关节速度耦合
    coriolis_comp[0] = 0.1f * m_calf * hip_vel * knee_vel * sinf(knee);  // 耦合项
    coriolis_comp[1] = -0.1f * m_calf * hip_vel * hip_vel * sinf(knee);   // 离心项
    coriolis_comp[2] = 0.0f;
}

/**
 * @brief 串联腿VMC控制映射 - 抽象力到关节力矩
 * @param leg 串联腿参数结构体
 * @param F_virtual 虚拟力 [N] - 等效单摆的切向力
 * @param T_virtual 虚拟力矩 [Nm] - 等效单摆的力矩
 * @note 使用雅可比转置映射：τ = J^T * [F_x, F_y, T_z]^T
 */
void SerialLegVMCProject(SerialLegParam *leg, float F_virtual, float T_virtual)
{
    // 将抽象控制力转换为任务空间力向量
    float F_x = -F_virtual * sinf(leg->equivalent_angle);  // 切向力
    float F_y = F_virtual * cosf(leg->equivalent_angle);   // 法向力
    float M_z = T_virtual;                                  // 力矩
    
    // 计算雅可比矩阵
    float J[3][3];
    SerialLegJacobian(leg, J);
    
    // 使用雅可比转置映射：τ = J^T * F
    leg->hip_torque = J[0][0] * F_x + J[1][0] * F_y + J[2][0] * M_z;
    leg->knee_torque = J[0][1] * F_x + J[1][1] * F_y + J[2][1] * M_z;
    leg->ankle_torque = J[0][2] * F_x + J[1][2] * F_y + J[2][2] * M_z;
    
    // 添加重力补偿 - 提高控制精度
    float gravity_comp[3], coriolis_comp[3];
    ChassisParam dummy_chassis = {0};  // 临时使用
    SerialLegDynamics(leg, &dummy_chassis, gravity_comp, coriolis_comp);
    
    leg->hip_torque += gravity_comp[0] + coriolis_comp[0];
    leg->knee_torque += gravity_comp[1] + coriolis_comp[1];
    leg->ankle_torque += gravity_comp[2] + coriolis_comp[2];
}

/**
 * @brief 串联腿LQR控制律计算 - 增益调度
 * @param leg 串联腿参数结构体
 * @param chassis 底盘参数结构体
 * @param gains LQR增益结构体
 * @note 根据等效腿长和机体状态调度增益 - 核心控制算法
 */
void SerialLegCalcLQR(SerialLegParam *leg, ChassisParam *chassis, SerialLQRGains_s *gains)
{
    // 基于等效腿长的增益调度 - 非线性控制策略
    float leg_length = leg->equivalent_length;
    float length_error = SERIAL_NOMINAL_LEG_LENGTH - leg_length;
    
    // 根据腿长调整增益 - 适应不同工作点
    float length_factor = leg_length / SERIAL_NOMINAL_LEG_LENGTH;
    float length_factor_sqr = length_factor * length_factor;
    
    // LQR状态反馈 - 基于线性化模型
    float theta_error = -leg->equivalent_angle;  // 期望角度为0（垂直）
    float theta_dot_error = -leg->equivalent_vel;
    float pitch_error = -chassis->pitch;
    float pitch_dot_error = -chassis->pitch_w;
    
    // 计算虚拟控制力 - 状态反馈
    float F_virtual = gains->k_theta * theta_error +
                       gains->k_theta_dot * theta_dot_error +
                       gains->k_length * length_error +
                       gains->k_pitch * pitch_error +
                       gains->k_pitch_dot * pitch_dot_error;
    
    float T_virtual = 0.1f * F_virtual;  // 力矩与力成比例关系
    
    // 应用VMC映射 - 将抽象力转换为关节力矩
    SerialLegVMCProject(leg, F_virtual, T_virtual);
}

/**
 * @brief 串联腿步态规划 - 生成足部轨迹
 * @param leg 串联腿参数结构体
 * @param gait 步态参数结构体
 * @param phase 当前步态相位 [0, 2π]
 * @param velocity 期望速度 [m/s]
 * @note 生成平滑的摆动相和支撑相轨迹 - 关键运动规划
 */
void SerialLegGaitPlanning(SerialLegParam *leg, SerialGaitParam_s *gait, 
                          float phase, float velocity)
{
    // 归一化相位到[0, 2π]
    while (phase > 2.0f * PI) phase -= 2.0f * PI;
    while (phase < 0.0f) phase += 2.0f * PI;
    
    // 步态参数计算
    float swing_ratio = gait->swing_time / (gait->swing_time + gait->stance_time);
    float stance_ratio = 1.0f - swing_ratio;
    
    if (phase < 2.0f * PI * swing_ratio) {
        // 摆动相 - 足部离地运动
        leg->stance_phase = 0;
        float swing_phase = phase / (2.0f * PI * swing_ratio);
        
        // 使用贝塞尔曲线生成平滑轨迹
        float t = swing_phase;
        float t2 = t * t;
        float t3 = t2 * t;
        
        // X方向：从后极限到前极限
        float x_start = -gait->step_length * 0.5f;
        float x_end = gait->step_length * 0.5f;
        float foot_x = x_start + (x_end - x_start) * (3.0f * t2 - 2.0f * t3);
        
        // Y方向：抬腿和落地
        float y_nominal = -SERIAL_NOMINAL_LEG_LENGTH;  // 基准高度为负
        float y_lift = y_nominal + gait->step_height * sinf(PI * t);
        float foot_y = y_lift;
        
        // 使用逆运动学计算关节角度目标
        SerialLegInverseKinematics(leg, foot_x, foot_y);
        
        // 设置期望速度（用于前馈）
        leg->foot_velocity_x = (x_end - x_start) / gait->swing_time;
        leg->foot_velocity_y = gait->step_height * PI * cosf(PI * t) / gait->swing_time;
        
    } else {
        // 支撑相 - 足部接触地面
        leg->stance_phase = 1;
        float stance_phase = (phase - 2.0f * PI * swing_ratio) / (2.0f * PI * stance_ratio);
        
        // X方向：从后极限到前极限（身体相对运动）
        float x_start = gait->step_length * 0.5f;
        float x_end = -gait->step_length * 0.5f;
        float foot_x = x_start + (x_end - x_start) * stance_phase;
        
        // Y方向：保持基准高度
        float foot_y = -SERIAL_NOMINAL_LEG_LENGTH;
        
        // 使用逆运动学计算关节角度目标
        SerialLegInverseKinematics(leg, foot_x, foot_y);
        
        // 设置期望速度
        leg->foot_velocity_x = (x_end - x_start) / gait->stance_time;
        leg->foot_velocity_y = 0.0f;
    }
}

/**
 * @brief 串联腿触地检测 - 多传感器融合
 * @param leg 串联腿参数结构体
 * @param force_threshold 触地力阈值 [N] - 调参要点33
 * @param velocity_threshold 触地速度阈值 [m/s] - 调参要点34
 * @return 触地状态 [0/1]
 * @note 结合力传感器、速度变化率和关节力矩突变判断
 */
uint8_t SerialLegTouchdownDetection(SerialLegParam *leg, float force_threshold, 
                                   float velocity_threshold)
{
    // 多传感器触地检测 - 提高可靠性
    uint8_t force_detect = 0;
    uint8_t velocity_detect = 0;
    uint8_t torque_detect = 0;
    
    // 1. 力传感器检测（如果有）
    if (leg->ground_reaction_force > force_threshold) {
        force_detect = 1;
    }
    
    // 2. 速度变化率检测 - 足部速度突然减小
    static float last_velocity_y = 0.0f;
    float velocity_change = last_velocity_y - leg->foot_velocity_y;
    if (velocity_change > velocity_threshold) {
        velocity_detect = 1;
    }
    last_velocity_y = leg->foot_velocity_y;
    
    // 3. 关节力矩突变检测 - 触地时关节力矩会突然增大
    static float last_torque_sum = 0.0f;
    float current_torque_sum = fabsf(leg->hip_torque) + fabsf(leg->knee_torque) + fabsf(leg->ankle_torque);
    float torque_change = current_torque_sum - last_torque_sum;
    if (torque_change > 5.0f) {  // 力矩变化阈值 - 调参要点35
        torque_detect = 1;
    }
    last_torque_sum = current_torque_sum;
    
    // 融合判断 - 多数表决
    return (force_detect + velocity_detect + torque_detect) >= 2;
}

/**
 * @brief 串联腿状态初始化 - 设置默认参数
 * @param leg 串联腿参数结构体
 * @note 设置默认的关节角度和控制参数 - 必须调用
 */
void SerialLegInit(SerialLegParam *leg)
{
    // 设置默认关节角度 - 站立姿态
    leg->hip_angle = 0.0f;  // 髋关节垂直
    leg->knee_angle = -30.0f * DEGREE_2_RAD;  // 膝关节微屈 - 缓冲
    leg->ankle_angle = 30.0f * DEGREE_2_RAD;  // 踝关节补偿
    
    // 设置目标角度
    leg->target_hip_angle = leg->hip_angle;
    leg->target_knee_angle = leg->knee_angle;
    leg->target_ankle_angle = leg->ankle_angle;
    
    // 初始化速度
    leg->hip_velocity = 0.0f;
    leg->knee_velocity = 0.0f;
    leg->ankle_velocity = 0.0f;
    
    // 初始化末端状态
    SerialLegForwardKinematics(leg);
    
    // 初始化控制输出
    leg->hip_torque = 0.0f;
    leg->knee_torque = 0.0f;
    leg->ankle_torque = 0.0f;
    
    // 初始化状态
    leg->stance_phase = 1;  // 默认支撑相
    leg->ground_reaction_force = SERIAL_LEG_MASS * 9.81f;  // 体重
}

/**
 * @brief 串联腿安全检查 - 多重保护机制
 * @param leg 串联腿参数结构体
 * @return 安全状态 [0/1] - 0表示不安全，需要急停
 * @note 检查关节角度、角速度、力矩是否超限 - 安全关键
 */
uint8_t SerialLegSafetyCheck(SerialLegParam *leg)
{
    // 关节角度限位检查 - 防止机械损坏
    if (leg->hip_angle < HIP_MIN_ANGLE * DEGREE_2_RAD || leg->hip_angle > HIP_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 髋关节超限
    }
    if (leg->knee_angle < KNEE_MIN_ANGLE * DEGREE_2_RAD || leg->knee_angle > KNEE_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 膝关节超限
    }
    if (leg->ankle_angle < ANKLE_MIN_ANGLE * DEGREE_2_RAD || leg->ankle_angle > ANKLE_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 踝关节超限
    }
    
    // 关节角速度限位检查 - 防止过快运动
    float max_velocity = 10.0f;  // 最大角速度 [rad/s] - 调参要点36
    if (fabsf(leg->hip_velocity) > max_velocity ||
        fabsf(leg->knee_velocity) > max_velocity ||
        fabsf(leg->ankle_velocity) > max_velocity) {
        return 0;  // 角速度超限
    }
    
    // 关节力矩限位检查 - 防止电机过载
    float max_torque = 15.0f;  // 最大力矩 [Nm] - 调参要点37
    if (fabsf(leg->hip_torque) > max_torque ||
        fabsf(leg->knee_torque) > max_torque ||
        fabsf(leg->ankle_torque) > max_torque) {
        return 0;  // 力矩超限
    }
    
    // 末端位置检查 - 防止足部到达不合理位置
    if (leg->foot_y > 0.0f) {  // 足部高于髋关节
        return 0;  // 足部位置异常
    }
    
    if (fabsf(leg->foot_x) > 0.3f) {  // 足部X方向位移过大 - 调参要点38
        return 0;  // 步幅过大
    }
    
    // 等效腿长检查 - 防止过度拉伸或压缩
    if (leg->equivalent_length > SERIAL_MAX_LEG_LENGTH || leg->equivalent_length < SERIAL_MIN_LEG_LENGTH) {
        return 0;  // 腿长异常
    }
    
    return 1;  // 所有检查通过，状态安全
}