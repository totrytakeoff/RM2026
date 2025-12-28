/**
 * @file serial_balance.c
 * @brief 串联腿平衡底盘控制主程序 - 基于并联腿控制策略改造
 * @details 控制架构：
 * 1. 状态估计：IMU + 关节编码器 → 机体姿态和速度
 * 2. 步态规划：根据速度和模式生成足部轨迹  
 * 3. 逆运动学：足部轨迹 → 关节角度目标
 * 4. LQR控制：基于简化模型计算关节力矩
 * 5. VMC映射：抽象力 → 实际关节力矩 + 重力补偿
 * 6. 安全保护：限位检测 + 紧急停止
 */

#include "serial_leg.h"
#include "balance.h"
#include "robot_def.h"
#include "general_def.h"
#include "ins_task.h"
#include "HT04.h"
#include "LK9025.h"
#include "controller.h"
#include "can_comm.h"
#include "super_cap.h"
#include "user_lib.h"
#include "remote_control.h"
#include "referee_task.h"
#include "stdint.h"
#include "arm_math.h"
#include "bsp_dwt.h"
#include "bsp_log.h"
#include "speed_estimation.h"
#include "fly_detection.h"
#include "buzzer.h"

// 计时变量
static uint32_t serial_balance_dwt_cnt;
static float serial_del_t;

// 底盘实例模块 - 串联腿配置
static INS_t *Serial_Chassis_IMU_data;
static RC_ctrl_t *serial_rc_data;
static Chassis_Ctrl_Cmd_s serial_chassis_cmd_recv;
static Chassis_Upload_Data_s serial_chassis_feedback_data;
static Chassis_Can_Comm serial_chassis_can_recv;

// 串联腿关节电机配置 - 每侧3个关节
static HTMotorInstance *serial_lf_motor, *serial_lk_motor, *serial_la_motor;  // 左腿髋膝踝
static HTMotorInstance *serial_rf_motor, *serial_rk_motor, *serial_ra_motor;  // 右腿髋膝踝
static HTMotorInstance *serial_leg_motors[6];  // 6个关节电机数组

// 驱动轮电机 - 与并联腿相同
static LKMotorInstance *serial_l_driven, *serial_r_driven, *serial_driven_motors[2];

// 串联腿参数结构体
static SerialLegParam serial_l_leg, serial_r_leg;
static ChassisParam serial_chassis;

// 串联腿LQR增益 - 增益调度
static SerialLQRGains_s serial_lqr_gains;

// 串联腿步态参数
static SerialGaitParam_s serial_gait_params;

// 步态状态变量
static float serial_gait_phase = 0.0f;          // 步态相位 [0, 2π]
static uint8_t serial_gait_enabled = 0;         // 步态使能
static float serial_desired_velocity = 0.0f;    // 期望速度

// 串联腿控制模式
static SerialLegControlMode_e serial_control_mode = SERIAL_LEG_BALANCE;

// 裁判系统通信
static referee_info_t *serial_referee_data;
static Referee_Interactive_info_t serial_ui_data;

// CAN通信
static CANCommInstance *serial_cmd_can_comm;

// 超级电容
static SuperCapInstance *serial_cap;
static uint16_t serial_DataSend2Cap[4] = {0, 0, 0, 0};

// 串联腿状态机
static Robot_Status_e serial_chassis_status;

/**
 * @brief 串联腿初始化 - 配置所有硬件和参数
 * @note 初始化顺序很重要，必须按照依赖关系进行
 */
void SerialBalanceInit()
{
    // 基础模块初始化 - 与并联腿相同
    serial_rc_data = RemoteControlInit(&huart3);
    Serial_Chassis_IMU_data = INS_Init();
    serial_referee_data = UITaskInit(&huart6, &serial_ui_data);
    
    // 超级电容初始化
    SuperCap_Init_Config_s cap_conf = {
        .can_config = {
            .can_handle = &hcan2,
            .tx_id = 0x302,
            .rx_id = 0x301,
        }
    };
    serial_cap = SuperCapInit(&cap_conf);
    
    // CAN通信初始化
    CANComm_Init_Config_s comm_conf = {
        .can_config = {
            .can_handle = &hcan2,
            .tx_id = 0x311,
            .rx_id = 0x312,
        },
        .daemon_count = 100,
        .recv_data_len = sizeof(Chassis_Ctrl_Cmd_s),
        .send_data_len = sizeof(Chassis_Upload_Data_s),
    };
    serial_cmd_can_comm = CANCommInit(&comm_conf);

    // 串联腿关节电机初始化 - 6个关节电机
    Motor_Init_Config_s joint_conf = {
        .can_init_config = {
            .can_handle = &hcan1
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = 2.0f,      // 串联腿需要更强的位置控制 - 调参要点39
                .Kd = 0.1f,
                .Ki = 0.0f,
                .DeadBand = 0.001f,
                .Improve = PID_DerivativeFilter | PID_Derivative_On_Measurement,
                .MaxOut = 8.0f,   // 串联腿需要更大的输出范围 - 调参要点40
                .Derivative_LPF_RC = 0.02f,
            }
        },
        .controller_setting_init_config = {
            .close_loop_type = ANGLE_LOOP,
            .outer_loop_type = OPEN_LOOP,
            .motor_reverse_flag = FEEDBACK_DIRECTION_NORMAL,
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
        },
        .motor_type = HT04
    };
    
    // 左腿关节电机
    joint_conf.can_init_config.tx_id = 1;
    joint_conf.can_init_config.rx_id = 11;
    serial_leg_motors[0] = serial_lf_motor = HTMotorInit(&joint_conf);  // 左髋
    
    joint_conf.can_init_config.tx_id = 2;
    joint_conf.can_init_config.rx_id = 12;
    serial_leg_motors[1] = serial_lk_motor = HTMotorInit(&joint_conf);  // 左膝
    
    joint_conf.can_init_config.tx_id = 3;
    joint_conf.can_init_config.rx_id = 13;
    serial_leg_motors[2] = serial_la_motor = HTMotorInit(&joint_conf);  // 左踝
    
    // 右腿关节电机
    joint_conf.can_init_config.tx_id = 4;
    joint_conf.can_init_config.rx_id = 14;
    serial_leg_motors[3] = serial_rf_motor = HTMotorInit(&joint_conf);  // 右髋
    
    joint_conf.can_init_config.tx_id = 5;
    joint_conf.can_init_config.rx_id = 15;
    serial_leg_motors[4] = serial_rk_motor = HTMotorInit(&joint_conf);  // 右膝
    
    joint_conf.can_init_config.tx_id = 6;
    joint_conf.can_init_config.rx_id = 16;
    serial_leg_motors[5] = serial_ra_motor = HTMotorInit(&joint_conf);  // 右踝

    // 驱动轮电机初始化 - 与并联腿相同
    Motor_Init_Config_s driven_conf = {
        .can_init_config.can_handle = &hcan2,
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = OPEN_LOOP,
            .close_loop_type = OPEN_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
        .motor_type = LK9025,
    };
    driven_conf.can_init_config.tx_id = 1;
    serial_driven_motors[0] = serial_l_driven = LKMotorInit(&driven_conf);
    driven_conf.can_init_config.tx_id = 2;
    serial_driven_motors[1] = serial_r_driven = LKMotorInit(&driven_conf);

    // 初始化串联腿参数
    SerialLegInit(&serial_l_leg);
    SerialLegInit(&serial_r_leg);

    // 初始化LQR增益 - 基于线性化模型设计
    serial_lqr_gains.k_theta = 80.0f;        // 角度反馈增益 - 调参要点41
    serial_lqr_gains.k_theta_dot = 15.0f;     // 角速度反馈增益 - 调参要点42
    serial_lqr_gains.k_length = 200.0f;       // 腿长反馈增益 - 调参要点43
    serial_lqr_gains.k_length_dot = 25.0f;    // 腿长变化率反馈增益 - 调参要点44
    serial_lqr_gains.k_pitch = 60.0f;         // 机体俯仰角反馈增益 - 调参要点45
    serial_lqr_gains.k_pitch_dot = 8.0f;      // 机体俯仰角速度反馈增益 - 调参要点46

    // 初始化步态参数
    serial_gait_params.swing_time = 0.4f;     // 摆动相时间 [s] - 调参要点47
    serial_gait_params.stance_time = 0.6f;    // 支撑相时间 [s] - 调参要点48
    serial_gait_params.step_height = 0.08f;  // 步高 [m] - 调参要点49
    serial_gait_params.step_length = 0.15f;  // 步长 [m] - 调参要点50
    serial_gait_params.touchdown_velocity = 0.5f;  // 触地速度 [m/s] - 调参要点51
    serial_gait_params.lift_off_velocity = 0.3f;   // 离地速度 [m/s] - 调参要点52

    serial_chassis_status = ROBOT_READY;
}

/**
 * @brief 串联腿参数组装 - 从传感器数据到控制参数
 * @note 将电机编码器和IMU数据转换为串联腿状态变量
 */
static void SerialParamAssemble()
{
    // 机体参数 - 与并联腿相同
    serial_chassis.pitch = Serial_Chassis_IMU_data->Pitch * DEGREE_2_RAD;
    serial_chassis.pitch_w = Serial_Chassis_IMU_data->Gyro[0];
    serial_chassis.yaw = Serial_Chassis_IMU_data->YawTotalAngle * DEGREE_2_RAD;
    serial_chassis.wz = Serial_Chassis_IMU_data->Gyro[2];
    serial_chassis.roll = Serial_Chassis_IMU_data->Roll * DEGREE_2_RAD;
    serial_chassis.roll_w = Serial_Chassis_IMU_data->Gyro[1];

    // 串联腿关节角度 - 需要根据电机安装方向调整符号
    // 左腿关节角度
    serial_l_leg.hip_angle = serial_lf_motor->measure.total_angle;      // 左髋关节
    serial_l_leg.hip_velocity = serial_lf_motor->measure.speed_rads;
    serial_l_leg.knee_angle = serial_lk_motor->measure.total_angle;     // 左膝关节
    serial_l_leg.knee_velocity = serial_lk_motor->measure.speed_rads;
    serial_l_leg.ankle_angle = serial_la_motor->measure.total_angle;    // 左踝关节
    serial_l_leg.ankle_velocity = serial_la_motor->measure.speed_rads;
    
    // 右腿关节角度 - 注意对称性
    serial_r_leg.hip_angle = -serial_rf_motor->measure.total_angle;     // 右髋关节（反向）
    serial_r_leg.hip_velocity = -serial_rf_motor->measure.speed_rads;
    serial_r_leg.knee_angle = -serial_rk_motor->measure.total_angle;  // 右膝关节（反向）
    serial_r_leg.knee_velocity = -serial_rk_motor->measure.speed_rads;
    serial_r_leg.ankle_angle = -serial_ra_motor->measure.total_angle; // 右踝关节（反向）
    serial_r_leg.ankle_velocity = -serial_ra_motor->measure.speed_rads;
    
    // 驱动轮速度
    serial_l_leg.w_ecd = serial_l_driven->measure.speed_rads;
    serial_r_leg.w_ecd = -serial_r_driven->measure.speed_rads;  // 右轮反向

    // 计算正运动学 - 更新末端位置
    SerialLegForwardKinematics(&serial_l_leg);
    SerialLegForwardKinematics(&serial_r_leg);
}

/**
 * @brief 串联腿步态控制 - 根据模式生成步态
 * @note 核心步态生成算法，支持多种步态模式
 */
static void SerialGaitControl()
{
    switch (serial_control_mode) {
        case SERIAL_LEG_BALANCE:
            // 平衡模式 - 静止站立
            serial_gait_enabled = 0;
            serial_desired_velocity = 0.0f;
            // 保持当前足部位置
            serial_l_leg.target_foot_x = serial_l_leg.foot_x;
            serial_l_leg.target_foot_y = serial_l_leg.foot_y;
            serial_r_leg.target_foot_x = serial_r_leg.foot_x;
            serial_r_leg.target_foot_y = serial_r_leg.foot_y;
            break;
            
        case SERIAL_LEG_WALKING:
            // 行走模式 - 对角步态
            serial_gait_enabled = 1;
            // 更新步态相位
            serial_gait_phase += 2.0f * PI * serial_del_t / (serial_gait_params.swing_time + serial_gait_params.stance_time);
            if (serial_gait_phase > 2.0f * PI) serial_gait_phase -= 2.0f * PI;
            
            // 左右腿相位差π - 对角步态
            float left_phase = serial_gait_phase;
            float right_phase = serial_gait_phase + PI;
            if (right_phase > 2.0f * PI) right_phase -= 2.0f * PI;
            
            // 生成步态轨迹
            SerialLegGaitPlanning(&serial_l_leg, &serial_gait_params, left_phase, serial_desired_velocity);
            SerialLegGaitPlanning(&serial_r_leg, &serial_gait_params, right_phase, serial_desired_velocity);
            break;
            
        case SERIAL_LEG_TROT:
            // 小跑步态 - 对角同步
            serial_gait_enabled = 1;
            serial_gait_phase += 2.0f * PI * serial_del_t / (serial_gait_params.swing_time + serial_gait_params.stance_time);
            if (serial_gait_phase > 2.0f * PI) serial_gait_phase -= 2.0f * PI;
            
            // 对角同步 - 同时摆动或支撑
            SerialLegGaitPlanning(&serial_l_leg, &serial_gait_params, serial_gait_phase, serial_desired_velocity);
            SerialLegGaitPlanning(&serial_r_leg, &serial_gait_params, serial_gait_phase, serial_desired_velocity);
            break;
            
        case SERIAL_LEG_BOUND:
            // 跳跃步态 - 双足同时离地
            serial_gait_enabled = 1;
            serial_gait_phase += 2.0f * PI * serial_del_t / (serial_gait_params.swing_time + serial_gait_params.stance_time);
            if (serial_gait_phase > 2.0f * PI) serial_gait_phase -= 2.0f * PI;
            
            // 双足同步
            SerialLegGaitPlanning(&serial_l_leg, &serial_gait_params, serial_gait_phase, serial_desired_velocity);
            SerialLegGaitPlanning(&serial_r_leg, &serial_gait_params, serial_gait_phase, serial_desired_velocity);
            break;
            
        case SERIAL_LEG_RESET:
            // 复位模式 - 回到初始姿态
            serial_gait_enabled = 0;
            // 使用逆运动学计算复位位置
            SerialLegInverseKinematics(&serial_l_leg, 0.0f, -SERIAL_NOMINAL_LEG_LENGTH);
            SerialLegInverseKinematics(&serial_r_leg, 0.0f, -SERIAL_NOMINAL_LEG_LENGTH);
            break;
    }
}

/**
 * @brief 串联腿LQR控制 - 计算关节力矩
 * @note 基于简化模型和增益调度的反馈控制
 */
static void SerialLQRControl()
{
    // 根据步态模式调整LQR增益
    SerialLQRGains_s current_gains = serial_lqr_gains;
    
    if (serial_gait_enabled) {
        // 步态模式下增强反馈增益 - 适应动态运动
        current_gains.k_theta *= 1.2f;
        current_gains.k_theta_dot *= 1.5f;
        current_gains.k_length *= 0.8f;  // 步态模式下腿长控制相对减弱
    }
    
    // 计算左右腿的LQR控制力矩
    SerialLegCalcLQR(&serial_l_leg, &serial_chassis, &current_gains);
    SerialLegCalcLQR(&serial_r_leg, &serial_chassis, &current_gains);
    
    // 触地检测 - 更新支撑相状态
    static uint32_t touchdown_counter = 0;
    touchdown_counter++;
    
    if (touchdown_counter >= 10) {  // 每10ms检测一次 - 调参要点53
        uint8_t l_touchdown = SerialLegTouchdownDetection(&serial_l_leg, 20.0f, 0.3f);
        uint8_t r_touchdown = SerialLegTouchdownDetection(&serial_r_leg, 20.0f, 0.3f);
        
        // 更新支撑相状态
        if (l_touchdown) serial_l_leg.stance_phase = 1;
        if (r_touchdown) serial_r_leg.stance_phase = 1;
        
        touchdown_counter = 0;
    }
}

/**
 * @brief 串联腿转向控制 - 差分驱动控制
 * @note 通过左右轮速差实现转向，与并联腿类似
 */
static void SerialSteeringControl()
{
    // 转向控制 - 与并联腿相同的策略
    static PIDInstance serial_steer_p_pid, serial_steer_v_pid;
    static uint8_t steer_pid_init = 0;
    
    if (!steer_pid_init) {
        PID_Init_Config_s steer_p_conf = {
            .Kp = 6.0f,      // 串联腿转向增益稍大 - 调参要点54
            .Kd = 0.0f,
            .Ki = 0.0f,
            .MaxOut = 4.0f,
            .DeadBand = 0.001f,
            .Improve = PID_DerivativeFilter | PID_Derivative_On_Measurement,
            .Derivative_LPF_RC = 0.05f,
        };
        PIDInit(&serial_steer_p_pid, &steer_p_conf);
        
        PID_Init_Config_s steer_v_conf = {
            .Kp = 4.0f,
            .Kd = 0.0f,
            .Ki = 0.0f,
            .MaxOut = 25.0f,
            .DeadBand = 0.0f,
            .Improve = PID_DerivativeFilter | PID_Derivative_On_Measurement,
            .Derivative_LPF_RC = 0.05f,
        };
        PIDInit(&serial_steer_v_pid, &steer_v_conf);
        steer_pid_init = 1;
    }
    
    // 根据控制模式选择转向策略
    if (serial_chassis_cmd_recv.chassis_mode == CHASSIS_FREE_DEBUG ||
        serial_chassis_cmd_recv.chassis_mode == CHASSIS_FOLLOW_GIMBAL_YAW) {
        float p_ref = PIDCalculate(&serial_steer_p_pid, serial_chassis.yaw, serial_chassis.target_yaw);
        PIDCalculate(&serial_steer_v_pid, serial_chassis.wz, p_ref);
    }
    else if (serial_chassis_cmd_recv.chassis_mode == CHASSIS_ROTATE) {
        PIDCalculate(&serial_steer_v_pid, serial_chassis.wz, (float)serial_chassis_cmd_recv.rotate_w);
    }
    
    // 应用转向力矩到驱动轮
    serial_l_leg.T_wheel -= serial_steer_v_pid.Output;
    serial_r_leg.T_wheel += serial_steer_v_pid.Output;
}

/**
 * @brief 串联腿安全检查 - 多重保护机制
 * @return 安全状态 [0/1] - 0表示不安全需要急停
 * @note 串联腿安全检查比并联腿更严格
 */
static uint8_t SerialSafetyCheck()
{
    // 左右腿安全检查
    uint8_t l_safe = SerialLegSafetyCheck(&serial_l_leg);
    uint8_t r_safe = SerialLegSafetyCheck(&serial_r_leg);
    
    if (!l_safe || !r_safe) {
        // 任何一条腿不安全都要急停
        return 0;
    }
    
    // 机体姿态检查 - 防止倾倒
    if (fabsf(serial_chassis.pitch) > 30.0f * DEGREE_2_RAD ||  // 俯仰角过大 - 调参要点55
        fabsf(serial_chassis.roll) > 25.0f * DEGREE_2_RAD) {   // 横滚角过大 - 调参要点56
        return 0;
    }
    
    return 1;  // 安全检查通过
}

/**
 * @brief 串联腿输出映射 - 将控制力矩映射到电机
 * @note 最后一步：将计算得到的力矩转换为电机指令
 */
static void SerialOutputMapping()
{
    // 安全检查
    if (!SerialSafetyCheck()) {
        // 安全异常，停止所有电机
        for (int i = 0; i < 6; i++) {
            HTMotorStop(serial_leg_motors[i]);
        }
        LKMotorStop(serial_l_driven);
        LKMotorStop(serial_r_driven);
        return;
    }
    
    // 关节电机输出映射 - 考虑扭矩常数和安全系数
    float torque_to_current = 0.25f;  // 力矩到电流的转换系数 - 调参要点57
    
    // 左腿关节
    HTMotorSetRef(serial_lf_motor, torque_to_current * serial_l_leg.hip_torque);
    HTMotorSetRef(serial_lk_motor, torque_to_current * serial_l_leg.knee_torque);
    HTMotorSetRef(serial_la_motor, torque_to_current * serial_l_leg.ankle_torque);
    
    // 右腿关节 - 注意方向
    HTMotorSetRef(serial_rf_motor, -torque_to_current * serial_r_leg.hip_torque);
    HTMotorSetRef(serial_rk_motor, -torque_to_current * serial_r_leg.knee_torque);
    HTMotorSetRef(serial_ra_motor, -torque_to_current * serial_r_leg.ankle_torque);
    
    // 驱动轮输出 - 与并联腿相同
    float wheel_torque_gain = 195.3125f;  // 与并联腿相同的增益
    LKMotorSetRef(serial_l_driven, wheel_torque_gain * serial_l_leg.T_wheel);
    LKMotorSetRef(serial_r_driven, wheel_torque_gain * (-serial_r_leg.T_wheel));  // 右轮反向
}

/**
 * @brief 串联腿通信和UI更新 - 与外部系统通信
 * @note 负责裁判系统通信、UI显示和数据上传
 */
static void SerialCommNUI()
{
    // 更新UI数据 - 显示串联腿状态
    serial_ui_data.direction = serial_chassis_cmd_recv.direction;
    serial_ui_data.friction_mode = serial_chassis_cmd_recv.friction_mode;
    serial_ui_data.loader_mode = serial_chassis_cmd_recv.loader_mode;
    serial_ui_data.chassis_mode = serial_chassis_cmd_recv.chassis_mode;
    serial_ui_data.ui_mode = serial_chassis_cmd_recv.ui_mode;
    serial_ui_data.vision_mode = serial_chassis_cmd_recv.vision_mode;
    
    // 串联腿特有数据
    serial_ui_data.coord[0] = serial_l_leg.foot_x;      // 左足X坐标
    serial_ui_data.coord[1] = serial_l_leg.foot_y;      // 左足Y坐标
    serial_ui_data.coord[2] = serial_r_leg.foot_x;      // 右足X坐标
    serial_ui_data.coord[3] = serial_r_leg.foot_y;      // 右足Y坐标
    serial_ui_data.coord[4] = serial_control_mode;      // 控制模式
    serial_ui_data.coord[5] = serial_gait_phase;        // 步态相位
}

/**
 * @brief 串联腿工作模式设置 - 根据指令切换模式
 * @note 处理不同的控制模式和状态切换
 */
static void SerialWorkingModeSet()
{
    if (serial_chassis_cmd_recv.chassis_mode == CHASSIS_RESET) {
        // 复位模式
        serial_control_mode = SERIAL_LEG_RESET;
        return;
    }
    else if (serial_chassis_cmd_recv.chassis_mode == CHASSIS_ZERO_FORCE) {
        // 急停模式
        serial_control_mode = SERIAL_LEG_BALANCE;
        serial_desired_velocity = 0.0f;
        serial_gait_enabled = 0;
        
        // 停止所有电机
        for (int i = 0; i < 6; i++) {
            HTMotorStop(serial_leg_motors[i]);
        }
        LKMotorStop(serial_l_driven);
        LKMotorStop(serial_r_driven);
        return;
    }
    
    // 根据速度指令选择控制模式
    float target_speed = serial_chassis_cmd_recv.vx;
    
    if (fabsf(target_speed) < 0.1f) {
        serial_control_mode = SERIAL_LEG_BALANCE;      // 低速平衡
    } else if (fabsf(target_speed) < 0.8f) {
        serial_control_mode = SERIAL_LEG_WALKING;      // 行走步态
    } else if (fabsf(target_speed) < 1.5f) {
        serial_control_mode = SERIAL_LEG_TROT;         // 小跑步态
    } else {
        serial_control_mode = SERIAL_LEG_BOUND;        // 跳跃步态
    }
    
    // 设置期望速度 - 带加速度限幅
    float max_acc = 0.8f;  // 最大加速度 [m/s²] - 调参要点58
    float speed_error = target_speed - serial_desired_velocity;
    serial_desired_velocity += sign(speed_error) * max_acc * serial_del_t;
    
    // 角度目标设置
    if (serial_chassis_cmd_recv.chassis_mode == CHASSIS_FREE_DEBUG) {
        serial_chassis.target_yaw = serial_chassis_cmd_recv.offset_angle;
    }
    serial_chassis.target_yaw = serial_chassis.yaw + serial_chassis_cmd_recv.offset_angle * DEGREE_2_RAD;
}

/**
 * @brief 串联腿速度估计 - 基于IMU和轮速的卡尔曼滤波
 * @note 与并联腿类似，但考虑串联腿的特殊运动学
 */
static void SerialSpeedEstimation()
{
    // 使用与并联腿相同的速度估计算法
    // 但需要根据串联腿的运动学特性进行调整
    
    // 计算修正的轮速 - 考虑腿部运动的影响
    float l_wheel_speed = serial_l_leg.w_ecd + serial_l_leg.knee_velocity * 0.1f;  // 膝关节影响
    float r_wheel_speed = serial_r_leg.w_ecd + serial_r_leg.knee_velocity * 0.1f;
    
    // 使用并联腿的速度估计算法框架
    SpeedEstimation(&serial_l_leg, &serial_r_leg, &serial_chassis, Serial_Chassis_IMU_data, serial_del_t);
    
    // 串联腿特有的速度修正 - 考虑步态相位
    if (serial_gait_enabled) {
        // 在步态运动中，需要根据支撑相状态调整速度估计
        if (serial_l_leg.stance_phase && serial_r_leg.stance_phase) {
            // 双足支撑 - 完全相信轮速
            serial_chassis.vel = (serial_l_leg.body_v + serial_r_leg.body_v) / 2.0f;
        } else if (serial_l_leg.stance_phase) {
            // 仅左腿支撑 - 更相信左腿轮速
            serial_chassis.vel = serial_l_leg.body_v;
        } else if (serial_r_leg.stance_phase) {
            // 仅右腿支撑 - 更相信右腿轮速
            serial_chassis.vel = serial_r_leg.body_v;
        } else {
            // 双足摆动 - 主要依靠IMU预测
            // 保持原有估计值
        }
    }
}

/**
 * @brief 串联腿主控制任务 - 主循环
 * @note 串联腿控制主循环，按固定频率运行
 */
void SerialBalanceTask()
{
    // 计算时间间隔
    serial_del_t = DWT_GetDeltaT(&serial_balance_dwt_cnt);
    
    // 基础功能
    BuzzerOn();
    
    // 控制模式切换
    SerialWorkingModeSet();
    
    // 参数组装
    SerialParamAssemble();
    
    // 通信和UI更新
    SerialCommNUI();
    
    // 步态规划和控制
    SerialGaitControl();
    
    // 速度估计 - 卡尔曼滤波
    SerialSpeedEstimation();
    
    // LQR控制律计算
    SerialLQRControl();
    
    // 转向控制
    SerialSteeringControl();
    
    // 安全检查失败时停止输出
    if (!SerialSafetyCheck()) {
        return;
    }
    
    // 输出映射到电机
    SerialOutputMapping();
}