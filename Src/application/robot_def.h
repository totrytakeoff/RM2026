/**
 * @file robot_def.h
 * @author YZControl/myself
 * @version 0.2
 * @date 2026-2-28
 *
 * @copyright Copyright (c) YZControl
 *
 */
#pragma once // 可以用#pragma once代替#ifndef ROBOT_DEF_H(header guard)
#ifndef ROBOT_DEF_H
#define ROBOT_DEF_H

#include "ins_task.h"
#include "robot_types.h"
#include "master_process.h"
#include "stdint.h"

/* 开发板类型定义,烧录时注意不要弄错对应功能;修改定义后需要重新编译,只能存在一个定义! */
#define ONE_BOARD // 单板控制整车
// #define CHASSIS_BOARD //底盘板
// #define GIMBAL_BOARD  //云台板


// 为了解耦,这个玩意儿暂时在 robot_types.h或者直接在 master_process.h中定义!!!
// #define VISION_USE_VCP  // 使用虚拟串口发送视觉数据
// #define VISION_USE_UART // 使用串口发送视觉数据

/* 机器人重要参数定义,注意根据不同机器人进行修改,浮点数需要以.0或f结尾,无符号以u结尾 */
// 云台参数
#define YAW_CHASSIS_ALIGN_ECD 2711  // 云台和底盘对齐指向相同方向时的电机编码器值,若对云台有机械改动需要修改
#define YAW_ECD_GREATER_THAN_4096 0 // ALIGN_ECD值是否大于4096,是为1,否为0;用于计算云台偏转角度
#define PITCH_HORIZON_ECD 3412      // 云台处于水平位置时编码器值,若对云台有机械改动需要修改
#define PITCH_MAX_ANGLE 0           // 云台竖直方向最大角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
#define PITCH_MIN_ANGLE 0           // 云台竖直方向最小角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
// 发射参数
#define ONE_BULLET_DELTA_ANGLE 45    // 发射一发弹丸拨盘转动的距离,由机械设计图纸给出
#define REDUCTION_RATIO_LOADER 49.0f // 拨盘电机的减速比,英雄需要修改为3508的19.0f
#define NUM_PER_CIRCLE 10            // 拨盘一圈的装载量
// 机器人底盘修改的参数,单位为mm(毫米)
#define WHEEL_BASE 440             // 纵向轴距(前进后退方向)
#define TRACK_WIDTH 440             // 横向轮距(左右平移方向)
#define CENTER_GIMBAL_OFFSET_X 0    // 云台旋转中心距底盘几何中心的距离,前后方向,云台位于正中心时默认设为0
#define CENTER_GIMBAL_OFFSET_Y 0    // 云台旋转中心距底盘几何中心的距离,左右方向,云台位于正中心时默认设为0
#define RADIUS_WHEEL 73            // 轮子半径
#define REDUCTION_RATIO_WHEEL 19.0f // 电机减速比,因为编码器量测的是转子的速度而不是输出轴的速度故需进行转换

#define GYRO2GIMBAL_DIR_YAW 1   // 陀螺仪数据相较于云台的yaw的方向,1为相同,-1为相反
#define GYRO2GIMBAL_DIR_PITCH 1 // 陀螺仪数据相较于云台的pitch的方向,1为相同,-1为相反
#define GYRO2GIMBAL_DIR_ROLL 1  // 陀螺仪数据相较于云台的roll的方向,1为相同,-1为相反

/* -------------------------robot_cmd输入源配置------------------------- */
#define ROBOT_CMD_INPUT_SRC_DT7 1u
#define ROBOT_CMD_INPUT_SRC_ET08 2u
#define ROBOT_CMD_INPUT_SRC_DATALINK 3u
// 本阶段按已跑通demo默认启用ET08
#define ROBOT_CMD_INPUT_SOURCE ROBOT_CMD_INPUT_SRC_ET08

/* robot_cmd外设接口,统一由配置层指定 */
#define ROBOT_CMD_UART_ET08_HANDLE huart3
#define ROBOT_CMD_UART_DT7_HANDLE huart3
#define ROBOT_CMD_UART_DATALINK_HANDLE huart3
#define ROBOT_CMD_UART_VISION_HANDLE huart1
#define CHASSIS_REFEREE_UART_HANDLE huart6

/* 双板通信接口(当前阶段单板优先,此处用于保留可移植配置入口) */
#define ROBOT_CMD_CANCOMM_CAN_HANDLE hcan1
#define ROBOT_CMD_CANCOMM_TX_ID 0x312u
#define ROBOT_CMD_CANCOMM_RX_ID 0x311u
#define CHASSIS_CANCOMM_CAN_HANDLE hcan2
#define CHASSIS_CANCOMM_TX_ID 0x311u
#define CHASSIS_CANCOMM_RX_ID 0x312u

/* ET08输入映射参数(默认采用当前demo映射) */
#define CMD_ET08_STICK_SCALE_DEN 660.0f
#define CMD_ET08_CHASSIS_RC_DEADZONE 50
#define CMD_ET08_GIMBAL_RC_DEADZONE 50
#define CMD_ET08_CHASSIS_VEL_SCALE 10.0f
#define CMD_ET08_CHASSIS_WZ_SCALE 6.0f
#define CMD_ET08_GIMBAL_PITCH_SCALE 0.001f
#define CMD_ET08_GIMBAL_YAW_STEP 0.4f
#define CMD_ET08_SINGLE_SHOT_HOLD_MS 150u
#define CMD_ET08_SWITCH_DEBOUNCE_MS 30u
#define CMD_ET08_SHOOT_RATE 8.0f
#define CMD_ET08_SD_UP_CHASSIS_MODE CHASSIS_FOLLOW_GIMBAL_YAW
#define CMD_ET08_SD_DEFAULT_CHASSIS_MODE CHASSIS_NO_FOLLOW
#define CMD_ET08_SD_UP_GIMBAL_MODE GIMBAL_GYRO_MODE
#define CMD_ET08_SD_DEFAULT_GIMBAL_MODE GIMBAL_GYRO_MODE

/* cmd通用默认参数 */
#define CMD_DEFAULT_SHOOT_RATE CMD_ET08_SHOOT_RATE
#define CMD_DEFAULT_BULLET_SPEED SMALL_AMU_30
#define CMD_DEFAULT_LID_MODE LID_CLOSE

/* DT7输入参数 */
#define CMD_DT7_GIMBAL_YAW_STEP_SCALE 0.005f
#define CMD_DT7_GIMBAL_PITCH_STEP_SCALE 0.001f
#define CMD_DT7_CHASSIS_VEL_SCALE 10.0f
#define CMD_DT7_DIAL_FRICTION_ON_THRESHOLD (-100)
#define CMD_DT7_DIAL_BURST_THRESHOLD (-500)
#define CMD_DT7_SHOOT_RATE 8.0f

/* VT03/VT13图传遥控输入参数 */
#define CMD_VT_STICK_SCALE_DEN 660.0f
#define CMD_VT_CHASSIS_RC_DEADZONE 50
#define CMD_VT_GIMBAL_RC_DEADZONE 50
#define CMD_VT_MOUSE_WZ_DEADZONE 8
#define CMD_VT_GIMBAL_YAW_STEP_SCALE 0.005f
#define CMD_VT_GIMBAL_PITCH_STEP_SCALE 0.001f
#define CMD_VT_CHASSIS_VEL_SCALE 10.0f
#define CMD_VT_CHASSIS_WZ_SCALE 6.0f
#define CMD_VT_CHASSIS_WZ_MOUSE_SCALE 0.003f
#define CMD_VT_DIAL_FRICTION_ON_THRESHOLD (-100)
#define CMD_VT_DIAL_BURST_THRESHOLD (-500)
#define CMD_VT_GEAR_S_CHASSIS_MODE CHASSIS_ROTATE
#define CMD_VT_GEAR_S_GIMBAL_MODE GIMBAL_GYRO_MODE
#define CMD_VT_GEAR_N_CHASSIS_MODE CHASSIS_NO_FOLLOW
#define CMD_VT_GEAR_N_GIMBAL_MODE GIMBAL_FREE_MODE
#define CMD_VT_GEAR_DEFAULT_CHASSIS_MODE CHASSIS_NO_FOLLOW
#define CMD_VT_GEAR_DEFAULT_GIMBAL_MODE GIMBAL_GYRO_MODE

/* 统一发射参数 */
#define SHOOT_LOADER_SINGLE_DEADTIME_MS 150.0f
#define SHOOT_LOADER_THREE_DEADTIME_MS 300.0f
#define SHOOT_LOADER_CONTINUOUS_SLEW_PER_MS 40.0f
#define SHOOT_FRICTION_SPEED_DEFAULT 30000.0f
#define SHOOT_FRICTION_SPEED_15 30000.0f
#define SHOOT_FRICTION_SPEED_18 30000.0f
#define SHOOT_FRICTION_SPEED_30 30000.0f
#define SHOOT_FRICTION_STOP_REF 0.0f
#define SHOOT_LOADER_STOP_REF 0.0f

/* 发射闭环参数 */
#define SHOOT_FRICTION_MOTOR_TYPE M3508
#define SHOOT_FRICTION_LEFT_DIR MOTOR_DIRECTION_NORMAL
#define SHOOT_FRICTION_RIGHT_DIR MOTOR_DIRECTION_REVERSE
#define SHOOT_FRICTION_SPEED_PID_KP 0.0f
#define SHOOT_FRICTION_SPEED_PID_KI 0.0f
#define SHOOT_FRICTION_SPEED_PID_KD 0.0f
#define SHOOT_FRICTION_SPEED_PID_IMPROVE PID_Integral_Limit
#define SHOOT_FRICTION_SPEED_PID_INTEGRAL_LIMIT 10000.0f
#define SHOOT_FRICTION_SPEED_PID_MAX_OUT 15000.0f
#define SHOOT_FRICTION_CURRENT_PID_KP 0.0f
#define SHOOT_FRICTION_CURRENT_PID_KI 0.0f
#define SHOOT_FRICTION_CURRENT_PID_KD 0.0f
#define SHOOT_FRICTION_CURRENT_PID_IMPROVE PID_Integral_Limit
#define SHOOT_FRICTION_CURRENT_PID_INTEGRAL_LIMIT 10000.0f
#define SHOOT_FRICTION_CURRENT_PID_MAX_OUT 15000.0f
#define SHOOT_FRICTION_INIT_OUTER_LOOP SPEED_LOOP
#define SHOOT_FRICTION_INIT_CLOSE_LOOP (SPEED_LOOP | CURRENT_LOOP)

#define SHOOT_LOADER_MOTOR_TYPE M2006
#define SHOOT_LOADER_DIR MOTOR_DIRECTION_NORMAL
#define SHOOT_LOADER_ANGLE_PID_KP 0.0f
#define SHOOT_LOADER_ANGLE_PID_KI 0.0f
#define SHOOT_LOADER_ANGLE_PID_KD 0.0f
#define SHOOT_LOADER_ANGLE_PID_IMPROVE 0
#define SHOOT_LOADER_ANGLE_PID_MAX_OUT 200.0f
#define SHOOT_LOADER_SPEED_PID_KP 0.0f
#define SHOOT_LOADER_SPEED_PID_KI 0.0f
#define SHOOT_LOADER_SPEED_PID_KD 0.0f
#define SHOOT_LOADER_SPEED_PID_IMPROVE PID_Integral_Limit
#define SHOOT_LOADER_SPEED_PID_INTEGRAL_LIMIT 5000.0f
#define SHOOT_LOADER_SPEED_PID_MAX_OUT 5000.0f
#define SHOOT_LOADER_CURRENT_PID_KP 0.0f
#define SHOOT_LOADER_CURRENT_PID_KI 0.0f
#define SHOOT_LOADER_CURRENT_PID_KD 0.0f
#define SHOOT_LOADER_CURRENT_PID_IMPROVE PID_Integral_Limit
#define SHOOT_LOADER_CURRENT_PID_INTEGRAL_LIMIT 5000.0f
#define SHOOT_LOADER_CURRENT_PID_MAX_OUT 5000.0f
#define SHOOT_LOADER_INIT_OUTER_LOOP SPEED_LOOP
#define SHOOT_LOADER_INIT_CLOSE_LOOP (CURRENT_LOOP | SPEED_LOOP)

/* 发射运行态外环策略 */
#define SHOOT_LOADER_LOOP_COOLING ANGLE_LOOP
#define SHOOT_LOADER_LOOP_STOP SPEED_LOOP
#define SHOOT_LOADER_LOOP_SINGLE ANGLE_LOOP
#define SHOOT_LOADER_LOOP_THREE ANGLE_LOOP
#define SHOOT_LOADER_LOOP_BURST SPEED_LOOP
#define SHOOT_LOADER_LOOP_REVERSE SPEED_LOOP

/* 云台指令限幅开关(使用机械限位时建议开启并写入正确角度) */
#define GIMBAL_PITCH_LIMIT_ENABLE 0u

/* 底盘控制参数 */
#define CHASSIS_FOLLOW_WZ_GAIN 1.5f
#define CHASSIS_ROTATE_WZ_REF 4000.0f

/* 底盘电机闭环参数 */
#define CHASSIS_SPEED_PID_KP 10.0f
#define CHASSIS_SPEED_PID_KI 0.0f
#define CHASSIS_SPEED_PID_KD 0.0f
#define CHASSIS_SPEED_PID_INTEGRAL_LIMIT 3000.0f
#define CHASSIS_SPEED_PID_MAX_OUT 12000.0f
#define CHASSIS_CURRENT_PID_KP 0.5f
#define CHASSIS_CURRENT_PID_KI 0.0f
#define CHASSIS_CURRENT_PID_KD 0.0f
#define CHASSIS_CURRENT_PID_INTEGRAL_LIMIT 3000.0f
#define CHASSIS_CURRENT_PID_MAX_OUT 15000.0f

/* 云台闭环参数 */
#define GIMBAL_YAW_ANGLE_PID_KP 8.0f
#define GIMBAL_YAW_ANGLE_PID_KI 0.0f
#define GIMBAL_YAW_ANGLE_PID_KD 0.0f
#define GIMBAL_YAW_ANGLE_PID_DEADBAND 0.1f
#define GIMBAL_YAW_ANGLE_PID_INTEGRAL_LIMIT 100.0f
#define GIMBAL_YAW_ANGLE_PID_MAX_OUT 500.0f
#define GIMBAL_YAW_SPEED_PID_KP 50.0f
#define GIMBAL_YAW_SPEED_PID_KI 200.0f
#define GIMBAL_YAW_SPEED_PID_KD 0.0f
#define GIMBAL_YAW_SPEED_PID_INTEGRAL_LIMIT 3000.0f
#define GIMBAL_YAW_SPEED_PID_MAX_OUT 20000.0f

#define GIMBAL_PITCH_ANGLE_PID_KP 10.0f
#define GIMBAL_PITCH_ANGLE_PID_KI 0.0f
#define GIMBAL_PITCH_ANGLE_PID_KD 0.0f
#define GIMBAL_PITCH_ANGLE_PID_INTEGRAL_LIMIT 100.0f
#define GIMBAL_PITCH_ANGLE_PID_MAX_OUT 500.0f
#define GIMBAL_PITCH_SPEED_PID_KP 50.0f
#define GIMBAL_PITCH_SPEED_PID_KI 350.0f
#define GIMBAL_PITCH_SPEED_PID_KD 0.0f
#define GIMBAL_PITCH_SPEED_PID_INTEGRAL_LIMIT 2500.0f
#define GIMBAL_PITCH_SPEED_PID_MAX_OUT 20000.0f

/* 底盘电机方向(全向轮步兵基线可在此快速调整) */
#define CHASSIS_MOTOR_LF_DIR MOTOR_DIRECTION_REVERSE
#define CHASSIS_MOTOR_RF_DIR MOTOR_DIRECTION_REVERSE
#define CHASSIS_MOTOR_LB_DIR MOTOR_DIRECTION_REVERSE
#define CHASSIS_MOTOR_RB_DIR MOTOR_DIRECTION_REVERSE

/* 底盘超级电容接口 */
#define CHASSIS_SUPERCAP_CAN_HANDLE hcan2
#define CHASSIS_SUPERCAP_TX_ID 0x302u
#define CHASSIS_SUPERCAP_RX_ID 0x301u

/* 向后兼容旧命名 */
#define CMD_STICK_SCALE_DEN CMD_ET08_STICK_SCALE_DEN

/* -------------------------电机CAN/ID配置(以infantry_omni_demo为基线)------------------------- */
/* gimbal */
#define GIMBAL_YAW_CAN_HANDLE hcan1
#define GIMBAL_YAW_MOTOR_ID 1u
#define GIMBAL_PITCH_CAN_HANDLE hcan2
#define GIMBAL_PITCH_MOTOR_ID 1u

/* shoot */
#define SHOOT_FRICTION_CAN_HANDLE hcan2
#define SHOOT_FRICTION_LEFT_MOTOR_ID 1u
#define SHOOT_FRICTION_RIGHT_MOTOR_ID 2u
#define SHOOT_LOADER_CAN_HANDLE hcan2
#define SHOOT_LOADER_MOTOR_ID 6u

/* chassis (single board infantry omni) */
#define CHASSIS_CAN_HANDLE hcan1
#define CHASSIS_MOTOR_LF_ID 1u
#define CHASSIS_MOTOR_RF_ID 2u
#define CHASSIS_MOTOR_LB_ID 4u
#define CHASSIS_MOTOR_RB_ID 3u

// 检查是否出现主控板定义冲突,只允许一个开发板定义存在,否则编译会自动报错
#if (defined(ONE_BOARD) && defined(CHASSIS_BOARD)) || \
    (defined(ONE_BOARD) && defined(GIMBAL_BOARD)) ||  \
    (defined(CHASSIS_BOARD) && defined(GIMBAL_BOARD))
#error Conflict board definition! You can only define one board type.
#endif

#if (ROBOT_CMD_INPUT_SOURCE != ROBOT_CMD_INPUT_SRC_DT7) && \
    (ROBOT_CMD_INPUT_SOURCE != ROBOT_CMD_INPUT_SRC_ET08) && \
    (ROBOT_CMD_INPUT_SOURCE != ROBOT_CMD_INPUT_SRC_DATALINK)
#error Invalid ROBOT_CMD_INPUT_SOURCE setting in robot_def.h
#endif

#pragma pack(1) // 压缩结构体,取消字节对齐,下面的数据都可能被传输
/* -------------------------基本控制模式和数据类型定义-------------------------*/

/* ----------------CMD应用发布的控制数据,应当由gimbal/chassis/shoot订阅---------------- */
/**
 * @brief 对于双板情况,遥控器和pc在云台,裁判系统在底盘
 *
 */
// cmd发布的底盘控制数据,由chassis订阅
typedef struct
{
    // 控制部分
    float vx;           // 前进方向速度
    float vy;           // 横移方向速度
    float wz;           // 旋转速度
    float offset_angle; // 底盘和归中位置的夹角
    chassis_mode_e chassis_mode;
    int chassis_speed_buff;
    // UI部分
    //  ...

} Chassis_Ctrl_Cmd_s;

// cmd发布的云台控制数据,由gimbal订阅
typedef struct
{ // 云台角度控制
    float yaw;
    float pitch;
    float chassis_rotate_wz;

    gimbal_mode_e gimbal_mode;
} Gimbal_Ctrl_Cmd_s;

// cmd发布的发射控制数据,由shoot订阅
typedef struct
{
    shoot_mode_e shoot_mode;
    loader_mode_e load_mode;
    lid_mode_e lid_mode;
    friction_mode_e friction_mode;
    Bullet_Speed_e bullet_speed; // 弹速枚举
    uint8_t rest_heat;
    float shoot_rate; // 连续发射的射频,unit per s,发/秒
} Shoot_Ctrl_Cmd_s;

/* ----------------gimbal/shoot/chassis发布的反馈数据----------------*/
/**
 * @brief 由cmd订阅,其他应用也可以根据需要获取.
 *
 */

typedef struct
{
#if defined(CHASSIS_BOARD) || defined(GIMBAL_BOARD) // 非单板的时候底盘还将imu数据回传(若有必要)
    // attitude_t chassis_imu_data;
#endif
    // 后续增加底盘的真实速度
    // float real_vx;
    // float real_vy;
    // float real_wz;

    uint8_t rest_heat;           // 剩余枪口热量
    Bullet_Speed_e bullet_speed; // 弹速限制
    Enemy_Color_e enemy_color;   // 0 for blue, 1 for red

} Chassis_Upload_Data_s;


typedef struct
{
    attitude_t gimbal_imu_data;
    uint16_t yaw_motor_single_round_angle;
} Gimbal_Upload_Data_s;

typedef struct
{
    // code to go here
    // ...
} Shoot_Upload_Data_s;

#pragma pack() // 开启字节对齐,结束前面的#pragma pack(1)

#endif // !ROBOT_DEF_H
