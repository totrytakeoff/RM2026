# 串联腿机器人开发核心规划方案

## 🎯 开发目标
基于现有并联腿框架，快速实现串联腿控制，跳过复杂理论，专注实战调参。

## ⚡ 核心开发阶段（总计7-10天）

### 阶段1：快速移植（1-2天）
**目标**：让串联腿能站起来，不摔倒

#### 关键修改点
1. **电机配置扩展** - 从2个增加到6个关节电机
2. **运动学替换** - 用`SerialLegInverseKinematics`替换`Link2Leg`
3. **参数组装适配** - 读取6个关节角度和速度

#### 核心代码修改
```c
// 在serial_balance.c中修改ParamAssemble函数
void SerialParamAssemble() {
    // 读取IMU数据 - 与并联腿相同
    serial_chassis.pitch = Serial_Chassis_IMU_data->pitch;
    serial_chassis.roll = Serial_Chassis_IMU_data->roll;
    serial_chassis.yaw = Serial_Chassis_IMU_data->yaw;
    serial_chassis.w_pitch = Serial_Chassis_IMU_data->gyro[0];
    serial_chassis.w_roll = Serial_Chassis_IMU_data->gyro[1];
    serial_chassis.w_yaw = Serial_Chassis_IMU_data->gyro[2];
    
    // 读取6个关节电机数据 - 串联腿特有
    serial_l_leg.hip_angle = HTMotorGetAngle(serial_lf_motor);
    serial_l_leg.hip_velocity = HTMotorGetOmega(serial_lf_motor);
    serial_l_leg.knee_angle = HTMotorGetAngle(serial_lk_motor);
    serial_l_leg.knee_velocity = HTMotorGetOmega(serial_lk_motor);
    serial_l_leg.ankle_angle = HTMotorGetAngle(serial_la_motor);
    serial_l_leg.ankle_velocity = HTMotorGetOmega(serial_la_motor);
    
    // 右腿同理 - 注意方向相反
    serial_r_leg.hip_angle = -HTMotorGetAngle(serial_rf_motor);
    serial_r_leg.hip_velocity = -HTMotorGetOmega(serial_rf_motor);
    serial_r_leg.knee_angle = -HTMotorGetAngle(serial_rk_motor);
    serial_r_leg.knee_velocity = -HTMotorGetOmega(serial_rk_motor);
    serial_r_leg.ankle_angle = -HTMotorGetAngle(serial_ra_motor);
    serial_r_leg.ankle_velocity = -HTMotorGetOmega(serial_ra_motor);
    
    // 计算正运动学
    SerialLegForwardKinematics(&serial_l_leg);
    SerialLegForwardKinematics(&serial_r_leg);
}
```

#### 调试目标
- [ ] 6个电机正常上电
- [ ] 关节角度读取正确
- [ ] 正运动学计算正确

### 阶段2：基础平衡（2-3天）
**目标**：能静态站立，轻微扰动不摔倒

#### 关键参数调节
```c
// 在SerialBalanceInit()中设置LQR增益
serial_lqr_gains.k_theta = 80.0f;        // 角度反馈增益 ❗重点调
serial_lqr_gains.k_theta_dot = 15.0f;    // 角速度反馈增益  
serial_lqr_gains.k_length = 200.0f;      // 腿长反馈增益 ❗重点调
serial_lqr_gains.k_length_dot = 25.0f;   // 腿长变化率反馈增益
serial_lqr_gains.k_pitch = 60.0f;        // 机体俯仰角反馈增益
serial_lqr_gains.k_pitch_dot = 8.0f;     // 机体俯仰角速度反馈增益
```

#### 调试顺序
1. **先调k_theta** - 让腿能撑住身体（80→120→160）
2. **再调k_length** - 让腿长稳定（200→300→400）
3. **最后调k_theta_dot** - 抑制振荡（15→25→35）

#### 平衡控制核心代码
```c
// LQR控制律实现
void SerialLegCalcLQR(SerialLegParam *leg, ChassisParam *chassis, SerialLQRGains_s *gains) {
    // 计算误差
    float theta_error = leg->equivalent_angle - 0.0f;  // 期望垂直
    float theta_dot_error = leg->equivalent_vel - 0.0f;
    float length_error = leg->equivalent_length - SERIAL_NOMINAL_LEG_LENGTH;
    float length_dot_error = (leg->foot_velocity_x * leg->foot_x + leg->foot_velocity_y * leg->foot_y) / leg->equivalent_length;
    float pitch_error = chassis->pitch - 0.0f;
    float pitch_dot_error = chassis->w_pitch - 0.0f;
    
    // 计算虚拟力
    float F_virtual = -gains->k_theta * theta_error 
                      - gains->k_theta_dot * theta_dot_error
                      - gains->k_length * length_error
                      - gains->k_length_dot * length_dot_error;
    
    float T_virtual = -gains->k_pitch * pitch_error
                      - gains->k_pitch_dot * pitch_dot_error;
    
    // VMC映射到关节力矩
    SerialLegVMCProject(leg, F_virtual, T_virtual);
}
```

#### 调试目标
- [ ] 能静态站立10秒
- [ ] 轻推不摔倒
- [ ] 无持续振荡

### 阶段3：简单步态（3-5天）
**目标**：能前后移动，不追求速度

#### 步态参数配置
```c
// 初始化步态参数
serial_gait_params.swing_time = 0.3f;        // 摆动相持续时间 [s] ❗重点调
serial_gait_params.stance_time = 0.3f;       // 支撑相持续时间 [s] ❗重点调  
serial_gait_params.step_height = 0.05f;     // 步高 [m] ❗重点调
serial_gait_params.step_length = 0.08f;      // 步长 [m] ❗重点调
serial_gait_params.touchdown_velocity = 0.1f; // 触地速度 [m/s]
serial_gait_params.lift_off_velocity = 0.15f; // 离地速度 [m/s]
```

#### 步态规划核心代码
```c
// 步态控制主函数
void SerialGaitControl() {
    if (!serial_gait_enabled) {
        // 平衡模式 - 保持足底位置
        serial_l_leg.target_foot_x = 0.0f;
        serial_l_leg.target_foot_y = -SERIAL_NOMINAL_LEG_LENGTH;
        serial_r_leg.target_foot_x = 0.0f;
        serial_r_leg.target_foot_y = -SERIAL_NOMINAL_LEG_LENGTH;
    } else {
        // 步态模式 - 生成周期性轨迹
        float phase_offset = PI;  // 对角步态，相位差180度
        
        // 左腿步态规划
        SerialLegGaitPlanning(&serial_l_leg, &serial_gait_params, 
                             serial_gait_phase, serial_desired_velocity);
        
        // 右腿步态规划，相位偏移
        SerialLegGaitPlanning(&serial_r_leg, &serial_gait_params, 
                             serial_gait_phase + phase_offset, serial_desired_velocity);
        
        // 更新步态相位
        float phase_increment = 2.0f * PI / (serial_gait_params.swing_time + serial_gait_params.stance_time);
        serial_gait_phase += phase_increment * serial_del_t;
        if (serial_gait_phase > 2.0f * PI) {
            serial_gait_phase -= 2.0f * PI;
        }
    }
    
    // 逆运动学求解
    SerialLegInverseKinematics(&serial_l_leg, serial_l_leg.target_foot_x, serial_l_leg.target_foot_y);
    SerialLegInverseKinematics(&serial_r_leg, serial_r_leg.target_foot_x, serial_r_leg.target_foot_y);
}
```

#### 步态轨迹生成
```c
// 足部轨迹生成函数
void SerialLegGaitPlanning(SerialLegParam *leg, SerialGaitParam_s *gait, float phase, float velocity) {
    float swing_phase, stance_phase;
    float total_time = gait->swing_time + gait->stance_time;
    float normalized_phase = phase / (2.0f * PI);
    
    if (normalized_phase < gait->stance_time / total_time) {
        // 支撑相
        stance_phase = normalized_phase * total_time / gait->stance_time;
        leg->target_foot_x = velocity * gait->stance_time * (stance_phase - 0.5f);
        leg->target_foot_y = -SERIAL_NOMINAL_LEG_LENGTH;
        leg->stance_phase = 1;
    } else {
        // 摆动相
        swing_phase = (normalized_phase - gait->stance_time / total_time) * total_time / gait->swing_time;
        
        // 使用贝塞尔曲线生成平滑轨迹
        float t = swing_phase;
        float lift_off_x = velocity * gait->stance_time * 0.5f;
        float touchdown_x = velocity * gait->stance_time * (-0.5f);
        
        // 三次贝塞尔曲线控制点
        float p0_x = lift_off_x, p0_y = -SERIAL_NOMINAL_LEG_LENGTH;
        float p1_x = lift_off_x + velocity * gait->swing_time * 0.25f, p1_y = -SERIAL_NOMINAL_LEG_LENGTH + gait->step_height;
        float p2_x = touchdown_x - velocity * gait->swing_time * 0.25f, p2_y = -SERIAL_NOMINAL_LEG_LENGTH + gait->step_height;
        float p3_x = touchdown_x, p3_y = -SERIAL_NOMINAL_LEG_LENGTH;
        
        // 贝塞尔曲线插值
        leg->target_foot_x = (1-t)*(1-t)*(1-t)*p0_x + 3*(1-t)*(1-t)*t*p1_x + 3*(1-t)*t*t*p2_x + t*t*t*p3_x;
        leg->target_foot_y = (1-t)*(1-t)*(1-t)*p0_y + 3*(1-t)*(1-t)*t*p1_y + 3*(1-t)*t*t*p2_y + t*t*t*p3_y;
        leg->stance_phase = 0;
    }
}
```

#### 调试技巧
- **步高太高** → 走路有跳跃感，减小`step_height`
- **步高太低** → 拖地步态，增大`step_height`  
- **时间太长** → 走路缓慢，减小`swing_time`和`stance_time`
- **时间太短** → 控制跟不上，增大时间参数

#### 调试目标
- [ ] 能前后移动
- [ ] 移动中不摔倒
- [ ] 步态平滑无跳跃

### 阶段4：安全保护（1天）
**目标**：确保机器人不会损坏

#### 关键安全限位
```c
// 机械限位 - 绝对不能超
#define HIP_MIN_ANGLE -60.0f      // 髋关节最小角度 [deg]
#define HIP_MAX_ANGLE 60.0f       // 髋关节最大角度 [deg] 
#define KNEE_MIN_ANGLE -150.0f    // 膝关节最小角度 [deg]
#define KNEE_MAX_ANGLE 10.0f      // 膝关节最大角度 [deg]
#define ANKLE_MIN_ANGLE -45.0f    // 踝关节最小角度 [deg]
#define ANKLE_MAX_ANGLE 45.0f     // 踝关节最大角度 [deg]
```

#### 安全检查函数
```c
// 串联腿安全检查 - 多重保护机制
uint8_t SerialLegSafetyCheck(SerialLegParam *leg) {
    // 关节角度限位检查
    if (leg->hip_angle < HIP_MIN_ANGLE * DEGREE_2_RAD || 
        leg->hip_angle > HIP_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 髋关节超限
    }
    
    if (leg->knee_angle < KNEE_MIN_ANGLE * DEGREE_2_RAD || 
        leg->knee_angle > KNEE_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 膝关节超限
    }
    
    if (leg->ankle_angle < ANKLE_MIN_ANGLE * DEGREE_2_RAD || 
        leg->ankle_angle > ANKLE_MAX_ANGLE * DEGREE_2_RAD) {
        return 0;  // 踝关节超限
    }
    
    // 角速度限位检查
    float max_joint_velocity = 3.0f;  // 最大关节角速度 [rad/s]
    if (fabsf(leg->hip_velocity) > max_joint_velocity ||
        fabsf(leg->knee_velocity) > max_joint_velocity ||
        fabsf(leg->ankle_velocity) > max_joint_velocity) {
        return 0;  // 角速度超限
    }
    
    // 腿长限位检查
    if (leg->equivalent_length > SERIAL_MAX_LEG_LENGTH ||
        leg->equivalent_length < SERIAL_MIN_LEG_LENGTH) {
        return 0;  // 腿长超限
    }
    
    // 力矩限位检查
    float max_torque = 5.0f;  // 最大关节力矩 [Nm]
    if (fabsf(leg->hip_torque) > max_torque ||
        fabsf(leg->knee_torque) > max_torque ||
        fabsf(leg->ankle_torque) > max_torque) {
        return 0;  // 力矩超限
    }
    
    return 1;  // 安全检查通过
}
```

#### 机体姿态安全检查
```c
// 机体姿态检查 - 防止倾倒
uint8_t SerialSafetyCheck() {
    // 左右腿安全检查
    uint8_t l_safe = SerialLegSafetyCheck(&serial_l_leg);
    uint8_t r_safe = SerialLegSafetyCheck(&serial_r_leg);
    
    if (!l_safe || !r_safe) {
        return 0;  // 任何一条腿不安全都要急停
    }
    
    // 机体姿态检查
    if (fabsf(serial_chassis.pitch) > 30.0f * DEGREE_2_RAD ||  // 俯仰角过大
        fabsf(serial_chassis.roll) > 25.0f * DEGREE_2_RAD) {   // 横滚角过大
        return 0;
    }
    
    return 1;  // 安全检查通过
}
```

#### 紧急停止机制
```c
// 紧急停止函数
void EmergencyStop() {
    // 停止所有关节电机
    for (int i = 0; i < 6; i++) {
        HTMotorStop(serial_leg_motors[i]);
    }
    
    // 停止驱动轮
    LKMotorStop(serial_l_driven);
    LKMotorStop(serial_r_driven);
    
    // 设置控制模式为复位
    serial_control_mode = SERIAL_LEG_RESET;
    
    // 报警提示
    BuzzerWarning();
}
```

#### 调试目标
- [ ] 关节限位生效
- [ ] 姿态异常检测
- [ ] 紧急停止可靠
- [ ] 故障报警正常

## 🔧 现场调试工具箱

### 1. 实时监控代码
```c
// 在SerialBalanceTask()中添加实时监控
static int debug_counter = 0;
if (++debug_counter % 100 == 0) {  // 每100ms打印一次
    printk("P:%.1f R:%.1f L_leg:%.3f R_leg:%.3f ", 
           serial_chassis.pitch*RAD_2_DEGREE,
           serial_chassis.roll*RAD_2_DEGREE,
           serial_l_leg.equivalent_length,
           serial_r_leg.equivalent_length);
    printk("L:%d R:%d V:%.2f\n", 
           serial_l_leg.stance_phase,
           serial_r_leg.stance_phase,
           serial_desired_velocity);
}
```

### 2. 单参数调节函数
```c
// 快速调节LQR增益
void QuickTuneLQR(float k_theta_scale, float k_length_scale) {
    serial_lqr_gains.k_theta *= k_theta_scale;
    serial_lqr_gains.k_length *= k_length_scale;
    printk("LQR tuned: k_theta=%.1f k_length=%.1f\n", 
           serial_lqr_gains.k_theta, serial_lqr_gains.k_length);
}

// 快速调节步态参数
void QuickTuneGait(float height_scale, float time_scale) {
    serial_gait_params.step_height *= height_scale;
    serial_gait_params.swing_time *= time_scale;
    serial_gait_params.stance_time *= time_scale;
    printk("Gait tuned: height=%.3f swing=%.2f stance=%.2f\n", 
           serial_gait_params.step_height,
           serial_gait_params.swing_time,
           serial_gait_params.stance_time);
}
```

### 3. 单腿测试模式
```c
// 左腿单独测试
void TestLeftLeg() {
    static float test_angle = 0.0f;
    test_angle += 0.01f;
    
    // 设置左腿目标角度
    serial_l_leg.target_hip_angle = 10.0f * DEGREE_2_RAD * sinf(test_angle);
    serial_l_leg.target_knee_angle = -20.0f * DEGREE_2_RAD * sinf(test_angle);
    serial_l_leg.target_ankle_angle = 10.0f * DEGREE_2_RAD * sinf(test_angle);
    
    // 右腿保持静止
    serial_r_leg.target_hip_angle = 0.0f;
    serial_r_leg.target_knee_angle = 0.0f;
    serial_r_leg.target_ankle_angle = 0.0f;
    
    printk("Left leg test: hip=%.1f knee=%.1f ankle=%.1f\n",
           serial_l_leg.target_hip_angle * RAD_2_DEGREE,
           serial_l_leg.target_knee_angle * RAD_2_DEGREE,
           serial_l_leg.target_ankle_angle * RAD_2_DEGREE);
}
```

## ⚠️ 常见问题快速解决方案

### 问题1：机器人完全站不起来
**现象**：机器人一上电就软倒
**解决**：
```c
// 紧急增大支撑力
serial_lqr_gains.k_theta *= 2.0f;      // 角度增益×2
serial_lqr_gains.k_length *= 2.0f;     // 腿长增益×2
// 如果还不行，继续×1.5，直到能撑住
```

### 问题2：机器人持续振荡
**现象**：站立时上下抖动或左右摇摆
**解决**：
```c
// 增加阻尼
serial_lqr_gains.k_theta_dot *= 3.0f;    // 角速度阻尼×3
serial_lqr_gains.k_length_dot *= 3.0f;  // 腿长阻尼×3
// 如果振荡加剧，反向调节×0.5
```

### 问题3：走路一瘸一拐
**现象**：左右腿不对称，步态不流畅
**解决**：
```c
// 检查步态对称性
serial_gait_params.swing_time = 0.25f;   // 确保左右时间一致
serial_gait_params.step_length = 0.08f; // 步长一致
// 检查左右腿电机方向是否相反
```

### 问题4：关节限位频繁触发
**现象**：电机突然停止，报限位错误
**解决**：
```c
// 检查机械安装角度
#define HIP_OFFSET 0.0f    // 增加偏移量
#define KNEE_OFFSET 0.0f   // 根据实际情况调整
// 重新校准零位
```

### 问题5：控制输出饱和
**现象**：电机输出达到最大值，控制无力
**解决**：
```c
// 减小控制增益
serial_lqr_gains.k_theta *= 0.7f;
serial_lqr_gains.k_length *= 0.7f;
// 或增大输出限制
joint_conf.controller_param_init_config.angle_PID.MaxOut = 12.0f;
```

## 📋 3天速成检查清单

### Day 1: 站起来 ✅
- [ ] 6个电机正常上电
- [ ] 关节角度读取正确
- [ ] 基本平衡参数调好
- [ ] 能静态站立10秒
- [ ] 轻推不摔倒

### Day 2: 走起来 ✅  
- [ ] 步态参数调好
- [ ] 能前后移动
- [ ] 移动中不摔倒
- [ ] 步态平滑无跳跃

### Day 3: 稳起来 ✅
- [ ] 抗轻度推搡
- [ ] 速度控制平滑
- [ ] 安全限位生效
- [ ] 紧急停止可靠
- [ ] 数据监控正常

## 🎯 核心参数速查表

| 参数类别 | 参数名称 | 初始值 | 调节范围 | 调节效果 |
|---------|---------|--------|----------|----------|
| **平衡控制** | k_theta | 80.0f | 40-200 | 越大越硬，易振荡 |
| **平衡控制** | k_length | 200.0f | 100-500 | 越大支撑越强 |
| **平衡控制** | k_theta_dot | 15.0f | 8-40 | 阻尼，抑制振荡 |
| **步态参数** | step_height | 0.05f | 0.02-0.10 | 步高，影响跨越能力 |
| **步态参数** | swing_time | 0.3f | 0.2-0.5 | 摆动时间，影响步频 |
| **步态参数** | stance_time | 0.3f | 0.2-0.5 | 支撑时间，影响稳定性 |
| **安全限位** | MAX_PITCH | 30° | 20-45° | 倾倒检测阈值 |
| **安全限位** | MAX_TORQUE | 5.0f | 3-8Nm | 力矩限制，保护电机 |

## 🔍 调试数据记录模板

```
调试日期: ____年__月__日
调试人员: _____________
机械配置: 大腿___mm 小腿___mm 踝高___mm

=== 基础平衡调试 ===
k_theta初始: _____ 最终: _____ 效果: _____________
k_length初始: _____ 最终: _____ 效果: _____________
k_theta_dot初始: _____ 最终: _____ 效果: _____________
站立稳定性: □优秀 □良好 □一般 □差
抗扰动能力: □强 □中 □弱

=== 步态调试 ===  
step_height初始: _____ 最终: _____ 效果: _____________
swing_time初始: _____ 最终: _____ 效果: _____________
stance_time初始: _____ 最终: _____ 效果: _____________
步态流畅性: □优秀 □良好 □一般 □差
速度范围: 最小___m/s 最大___m/s

=== 安全测试 ===
倾倒检测: □正常 □误触发 □不触发
限位保护: □正常 □误触发 □不触发
紧急停止: □正常 □失效

=== 最终性能 ===
最大速度: _____m/s
续航时间: _____min
抗推搡能力: □强 □中 □弱
整体评价: □优秀 □良好 □一般 □差
备注: __________________________________________
```

## ⚡ 最终验收标准

### 基本功能 ✅
- [ ] 静态站立 > 60秒
- [ ] 前后移动 > 1m
- [ ] 左右转向 > 45°
- [ ] 抗轻度推搡（10%体重）

### 安全保护 ✅  
- [ ] 倾倒检测 < 1秒响应
- [ ] 限位保护 100%生效
- [ ] 紧急停止 < 0.5秒
- [ ] 故障报警正常

### 性能指标 ✅
- [ ] 最大速度 > 0.5m/s
- [ ] 步态周期稳定
- [ ] 控制频率 1000Hz
- [ ] 数据刷新 100Hz

## 📞 调试支持

如遇到调试问题，按以下顺序排查：
1. **检查机械安装** - 关节零位、连杆长度
2. **检查电机方向** - 左右腿是否对称
3. **检查传感器数据** - IMU、编码器读数
4. **检查控制输出** - 是否饱和、限位触发
5. **检查安全保护** - 是否误触发
6. **逐步调节参数** - 从小到大，从慢到快

记住：**先站稳，再走稳，最后跑稳！** 🚀