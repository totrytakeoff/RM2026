/**
 * @file minimal_types.h
 * @brief 最小框架类型定义
 */

#ifndef MINIMAL_TYPES_H
#define MINIMAL_TYPES_H

#include <stdint.h>
#include "minimal_config.h"  // 先包含配置文件,获取INPUT_SOURCE等宏定义
#include "minimal_referee_types.h"

/* INPUT_SOURCE相关宏已在minimal_config.h中定义 */

/*============================================================================
 * 控制模式枚举
 *============================================================================*/
typedef enum {
    MODE_ZERO_FORCE = 0,    // 停止
    MODE_NORMAL,            // 正常工作
} WorkMode_e;

typedef enum {
    CHASSIS_FOLLOW = 0,     // 云台跟随底盘
    CHASSIS_NO_FOLLOW,      // 底盘独立
} ChassisMode_e;

typedef enum {
    GIMBAL_FOLLOW_CHASSIS = 0,  // 云台跟随底盘(编码器闭环)
    GIMBAL_SEPARATE,            // 云台分离(IMU闭环,不随底盘)
} GimbalMode_e;

typedef enum {
    FRICTION_OFF = 0,
    FRICTION_ON,
} FrictionMode_e;

typedef enum {
    LOADER_STOP = 0,
    LOADER_SINGLE,
    LOADER_DOUBLE,
    LOADER_CONTINUOUS,
} LoaderMode_e;

typedef enum {
    CTRL_ZERO_FORCE = 0,
    CTRL_ENABLE,
} ControlMode_e;

typedef enum {
    REF_SPEED = 0,
    REF_ANGLE,
} RefType_e;

typedef enum {
    INPUT_ACTIVE_NONE = 0,
    INPUT_ACTIVE_VT,
    INPUT_ACTIVE_ET08,
} InputActive_e;

/*============================================================================
 * ET08遥控数据结构
 *============================================================================*/
typedef struct {
    int16_t left_x;
    int16_t left_y;
    int16_t right_x;
    int16_t right_y;
    uint8_t sa_pos;         // SA开关: 0=上, 1=中, 2=下
    uint8_t sb_pos;         // SB开关
    uint8_t sd_pos;         // SD开关
    uint8_t online;         // 在线状态
} RC_ET08_Data_t;

/*============================================================================
 * VT图传键鼠数据结构
 *============================================================================*/
typedef struct {
    /* 通道数据 (centered: -660 ~ +660) */
    int16_t ch0_c;          // 右摇杆X
    int16_t ch1_c;          // 右摇杆Y
    int16_t ch2_c;          // 左摇杆Y
    int16_t ch3_c;          // 左摇杆X
    int16_t dial_c;         // 拨轮
    
    /* 开关状态 */
    uint8_t gear;           // 档位: 0=C, 1=N, 2=S
    uint8_t pause;          // Pause键
    uint8_t custom_l;       // 左自定义键
    uint8_t custom_r;       // 右自定义键
    uint8_t trigger;        // 触发键
    
    /* 鼠标数据 */
    int16_t mouse_x;        // 鼠标X增量
    int16_t mouse_y;        // 鼠标Y增量
    int16_t mouse_z;        // 鼠标滚轮
    uint8_t mouse_l;        // 左键
    uint8_t mouse_r;        // 右键
    uint8_t mouse_m;        // 中键
    
    /* 键盘数据 */
    uint16_t keyboard;      // 16位按键状态
    
    /* 在线状态 */
    uint8_t online;
} VT_Data_t;

/*============================================================================
 * 统一输入数据结构 (抽象ET08和VT)
 *============================================================================*/
typedef struct {
    /* 底盘控制量 */
    float vx;               // 纵向速度 (m/s)
    float vy;               // 横向速度 (m/s)
    float wz;               // 旋转角速度 (rad/s)
    
    /* 云台控制量 */
    float yaw_speed;        // Yaw角速度指令
    float pitch_speed;      // Pitch角速度指令
    float pitch_angle;      // Pitch目标角度 (用于分离模式)
    
    /* 发射控制 */
    FrictionMode_e friction;
    LoaderMode_e loader;
    uint8_t shoot_state;        // 发射状态 (ShootState_e)
    
    /* 模式控制 */
    GimbalMode_e gimbal_mode;   // 云台模式
    uint8_t emergency_stop;     // 急停
    
    /* 状态 */
    uint8_t online;
    uint8_t gear;              // 档位(仅VT有效)
    InputActive_e active_input;
    
    RC_ET08_Data_t rc_raw;
    VT_Data_t vt_raw;
    /* 原始数据(调试用) */
} Input_Data_t;

/*============================================================================
 * 底盘控制数据
 *============================================================================*/
typedef struct {
    float vx;               // 纵向速度 (m/s)
    float vy;               // 横向速度 (m/s)
    float wz;               // 旋转角速度 (rad/s)
    ChassisMode_e mode;
    ControlMode_e control_mode;
    RefType_e ref_type;
} Chassis_Cmd_t;

/*============================================================================
 * 云台控制数据
 *============================================================================*/
typedef struct {
    float yaw_speed;        // Yaw角速度指令
    float pitch_speed;      // Pitch角速度指令
    float yaw_angle;        // Yaw目标角度(分离模式)
    float pitch_angle;      // Pitch目标角度
    GimbalMode_e mode;      // 跟随/分离模式
    uint8_t manual_pitch;   // 手动Pitch模式
    ControlMode_e control_mode;
    RefType_e ref_type;
} Gimbal_Cmd_t;

/*============================================================================
 * 发射控制数据
 *============================================================================*/
typedef struct {
    FrictionMode_e friction;
    LoaderMode_e loader;
    ControlMode_e control_mode;
    RefType_e ref_type;
} Shoot_Cmd_t;

/*============================================================================
 * 全局控制上下文 - 所有模块共享
 *============================================================================*/
typedef struct {
    Input_Data_t input;
    Chassis_Cmd_t chassis;
    Gimbal_Cmd_t gimbal;
    Shoot_Cmd_t shoot;
    
    uint8_t emergency_stop; // 紧急停止标志
    uint8_t initialized;    // 初始化完成标志
    MinimalRefereeData_t referee;
} Robot_Context_t;

/* 全局上下文声明 */
extern Robot_Context_t g_robot;

#endif /* MINIMAL_TYPES_H */
