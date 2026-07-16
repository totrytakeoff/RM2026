/**
 * @file infantry_config.h
 * @brief 步兵应用配置文件 - 所有参数集中于此
 * @note 修改参数后重新编译即可
 */

#ifndef INFANTRY_CONFIG_H
#define INFANTRY_CONFIG_H

#include "main.h"

/*============================================================================
 * 步兵应用调试系统配置
 *============================================================================*/
#define MINIMAL_DEBUG_ENABLE                        1U //控制日志开关,注意VT冲突

#define MINIMAL_DEBUG_MODE_TEXT                     (1U << 0)
#define MINIMAL_DEBUG_MODE_VOFA                     (1U << 1)
// #define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_TEXT | MINIMAL_DEBUG_MODE_VOFA)
#define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_TEXT )
// #define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_VOFA )

#define MINIMAL_DEBUG_UART_PORT                     1U   /* 1/3/6 */
#define MINIMAL_DEBUG_UART_TIMEOUT_MS               20U
#define MINIMAL_DEBUG_UART_BAUDRATE                 115200U
#define MINIMAL_DEBUG_ALLOW_MIXED_STREAM            0U   /* 0: 单串口禁止Text+VOFA混发 */

#define MINIMAL_DEBUG_MOD_SYSTEM                    0U
#define MINIMAL_DEBUG_MOD_INPUT                     1U
#define MINIMAL_DEBUG_MOD_CHASSIS                   0U
#define MINIMAL_DEBUG_MOD_GIMBAL                    0U
#define MINIMAL_DEBUG_MOD_SHOOT                     0U

#define MINIMAL_DEBUG_TEXT_PERIOD_MS                50U  /* 20Hz */
#define MINIMAL_DEBUG_VOFA_PERIOD_MS                20U  /* 50Hz */
#define GIMBAL_DEBUG_DETAIL_PERIOD_MS               50U  /* 20Hz */

#define MINIMAL_DEBUG_DISABLE_VT_ON_UART_CONFLICT   1U

/* 桥接到bsp_log */
#ifndef BSP_LOG_USE_UART
#define BSP_LOG_USE_UART                            1
#endif
#ifndef BSP_LOG_UART_PORT
#define BSP_LOG_UART_PORT                           MINIMAL_DEBUG_UART_PORT
#endif

#if (!MINIMAL_DEBUG_ENABLE) || ((MINIMAL_DEBUG_MODE & MINIMAL_DEBUG_MODE_TEXT) == 0)
#ifndef DISABLE_LOG_SYSTEM
#define DISABLE_LOG_SYSTEM 1
#endif
#endif

#if MINIMAL_DEBUG_UART_PORT == 1U
#define MINIMAL_DEBUG_UART_HANDLE huart1
#elif MINIMAL_DEBUG_UART_PORT == 3U
#define MINIMAL_DEBUG_UART_HANDLE huart3
#elif MINIMAL_DEBUG_UART_PORT == 6U
#define MINIMAL_DEBUG_UART_HANDLE huart6
#else
#error "Unsupported MINIMAL_DEBUG_UART_PORT, use 1/3/6."
#endif

/*============================================================================
 * 控制源选择 (二选一)
 *============================================================================*/
#define INPUT_SOURCE_ET08       0U      // ET08遥控器
#define INPUT_SOURCE_VT         1U      // VT图传键鼠

/* 兼容旧逻辑: INPUT_SOURCE仅用于回放/调试,实际运行采用双输入仲裁 */
#define INPUT_SOURCE            INPUT_SOURCE_VT

/* 双输入仲裁配置 */
#define INPUT_VT_PRIMARY                1U
#define INPUT_ET08_TAKEOVER_SD_UP       1U
#define INPUT_FAILSAFE_HOLD_MS          1000U

/*============================================================================
 * 系统配置
 *============================================================================*/
#define MAIN_LOOP_PERIOD_MS     20U     // 主循环周期(ms), 与omni_demo对齐
#define MOTOR_STABILIZE_TIME_MS 2000U   // 上电后电机稳定等待时间
#define INFANTRY_CPU_FREQUENCY_MHZ       168U

/* FreeRTOS迁移阶段任务周期：控制环保持minimal基线，快环独立调度。 */
#define INFANTRY_INS_TASK_PERIOD_MS         1U
#define INFANTRY_MOTOR_TASK_PERIOD_MS       5U
#define INFANTRY_HEALTH_TASK_PERIOD_MS      5U
#define INFANTRY_DIAGNOSTICS_TASK_PERIOD_MS 10U

/* 任务健康判定：连续超期、心跳丢失或栈余量不足均触发安全停机。 */
#define INFANTRY_TASK_STARTUP_GRACE_MS          100U
#define INFANTRY_TASK_HEARTBEAT_PERIODS         4U
#define INFANTRY_TASK_DEADLINE_TOLERANCE_PERCENT 25U
#define INFANTRY_TASK_MAX_CONSECUTIVE_OVERRUNS  3U
#define INFANTRY_TASK_MIN_STACK_FREE_WORDS      64U
#define INFANTRY_TASK_STACK_SAMPLE_INTERVAL     100U

/*============================================================================
 * CAN总线电机配置
 *============================================================================*/
/* 底盘电机 - CAN1, M3508 */
#define CHASSIS_CAN             hcan1
#define CHASSIS_MOTOR_FR_ID     3U      // 前右
#define CHASSIS_MOTOR_FL_ID     2U      // 前左
#define CHASSIS_MOTOR_BR_ID     4U      // 后右
#define CHASSIS_MOTOR_BL_ID     1U      // 后左

/* 云台电机 - GM6020 */
#define YAW_CAN                 hcan1
#define YAW_MOTOR_ID            1U
#define PITCH_CAN               hcan2
#define PITCH_MOTOR_ID          1U

/* 发射电机 - CAN2 */
#define FRICTION_CAN            hcan2
#define FRICTION_LEFT_ID        1U
#define FRICTION_RIGHT_ID       2U
#define LOADER_CAN              hcan2
#define LOADER_MOTOR_ID         6U

/*============================================================================
 * 遥控配置 - ET08 SBUS (UART3)
 *============================================================================*/
#define RC_UART                 huart3
#define RC_ONLINE_TIMEOUT_MS    1000U
#define RC_DEADZONE             50
#define RC_STICK_SCALE          660.0f

/* 摇杆映射模式: 0=左CH3/CH4, 1=左CH1/CH2 */
#define RC_MAPPING_MODE         0U
#define ET08_GIMBAL_YAW_SPEED_SCALE 0.23f

/*============================================================================
 * 图传链路配置 - VT03/VT13 (USART6, 921600bps)
 *============================================================================*/
#define VT_UART                 huart6
#define VT_UART_PORT            6U
#define VT_BAUDRATE             921600U
#define VT_PROTOCOL_FRAME_SIZE  21U
#define VT_ONLINE_TIMEOUT_MS    200U

/* 键盘按键位定义 */
#define VT_KEY_W                (1U << 0)
#define VT_KEY_S                (1U << 1)
#define VT_KEY_A                (1U << 2)
#define VT_KEY_D                (1U << 3)
#define VT_KEY_SHIFT            (1U << 4)
#define VT_KEY_CTRL             (1U << 5)
#define VT_KEY_Q                (1U << 6)
#define VT_KEY_E                (1U << 7)
#define VT_KEY_R                (1U << 8)
#define VT_KEY_F                (1U << 9)
#define VT_KEY_G                (1U << 10)
#define VT_KEY_Z                (1U << 11)
#define VT_KEY_X                (1U << 12)
#define VT_KEY_C                (1U << 13)
#define VT_KEY_V                (1U << 14)
#define VT_KEY_B                (1U << 15)

/* 底盘速度参数 */
#define VT_CHASSIS_BASE_SPEED   8.0f    // 基础速度 (m/s)
#define VT_CHASSIS_FAST_MULT    1.5f    // Shift加速倍数
#define VT_CHASSIS_SLOW_MULT    0.4f    // Ctrl减速倍数
#define VT_CHASSIS_ROTATE_SPEED 6.0f    // Q/E旋转速度 (rad/s)

/* VT云台灵敏度 */
#define VT_MOUSE_YAW_SENSITIVITY      0.30f   // Yaw鼠标灵敏度(deg/s per count)
#define VT_MOUSE_PITCH_SENSITIVITY    0.24f   // Pitch鼠标灵敏度(deg/s per count)
#define VT_MOUSE_PITCH_MODE_FULL_SCALE 400.0f // 鼠标Pitch进入速度环判定满量程
#define VT_STICK_YAW_SPEED_SCALE      36.0f   // VT摇杆Yaw速度增益(deg/s)
#define VT_STICK_PITCH_SPEED_SCALE    (GM6020_SPEED_MAX * 0.15f / 660.0f)
#define VT_PITCH_MAX_ANGLE      35.0f   // Pitch最大角度限制
#define VT_PITCH_MIN_ANGLE      -25.0f  // Pitch最小角度限制

/*============================================================================
 * 裁判系统(只读联锁)配置
 *============================================================================*/
#define REFEREE_ENABLE                   0U
#define REFEREE_UART                     huart1
#define REFEREE_CHASSIS_POWER_NOMINAL    80.0f
#define REFEREE_CHASSIS_SCALE_MIN        0.30f
#define REFEREE_HEAT_STOP_RATIO          0.95f
#define REFEREE_FRICTION_SLOW_RATIO      0.80f

/*============================================================================
 * 底盘参数
 *============================================================================*/
#define CHASSIS_WHEEL_RADIUS    0.075f  // 轮子半径(m)
#define CHASSIS_WHEEL_BASE      0.34f   // 轮距(m)

#define CHASSIS_MAX_VX          20.0f   // 最大纵向速度(m/s)
#define CHASSIS_MAX_VY          20.0f   // 最大横向速度(m/s)
#define CHASSIS_MAX_WZ          47.0f   // 最大旋转角速度(rad/s)

#define CHASSIS_SPEED_SCALE     3.0f    // 速度缩放系数
#define CHASSIS_SPEED_DEADZONE  120.0f  // 速度死区
#define CHASSIS_DEADZONE_VX     0.08f   // 底盘输入死区(vx)
#define CHASSIS_DEADZONE_VY     0.08f   // 底盘输入死区(vy)
#define CHASSIS_DEADZONE_WZ     0.10f   // 底盘输入死区(wz)
#define CHASSIS_SPEED_FILTER_COEF 0.50f // 一阶低通滤波系数

/* 底盘电机PID */
#define CHASSIS_SPEED_KP        4.0f
#define CHASSIS_SPEED_KI        0.04f
#define CHASSIS_SPEED_KD        0.0f
#define CHASSIS_SPEED_MAX_OUT   15000.0f

/* 底盘环路策略 */
#define CHASSIS_INIT_LOOP              SPEED_LOOP
#define CHASSIS_RUN_LOOP_STOP          SPEED_LOOP
#define CHASSIS_RUN_LOOP_NORMAL        SPEED_LOOP

/* 底盘-云台相对姿态标定 */
#define YAW_CHASSIS_ALIGN_ECD          2711U
#define YAW_ALIGN_ANGLE_DEG            (YAW_CHASSIS_ALIGN_ECD * ECD_ANGLE_COEF_DJI)
#define IMU_YAW_LOGIC_ZERO_TOTAL_DEG   364.30f
#define YAW_OFFSET_LOGIC_ZERO_DEG      (-118.915f)

/* FOLLOW模式: 相对夹角闭环生成底盘角速度 */
#define CHASSIS_FOLLOW_WZ_KP           (-25.0f)
#define CHASSIS_FOLLOW_WZ_KI           3.0f
#define CHASSIS_FOLLOW_WZ_KD           0.15f
#define CHASSIS_FOLLOW_WZ_MAX          50.0f
#define CHASSIS_FOLLOW_WZ_I_MAX        15.0f
#define CHASSIS_FOLLOW_SPEED_DEADZONE  20.0f

/* 小陀螺 */
#define SPIN_ROTATE_SPEED_RAD_S        40.0f

/*============================================================================
 * 云台参数
 *============================================================================*/
#define GIMBAL_SPEED_DEADZONE_ET08   30.0f
#define GIMBAL_SPEED_DEADZONE_VT_MOUSE 1.0f
#define GIMBAL_SPEED_DEADZONE_VT_STICK 30.0f
#define GIMBAL_RC_DEADZONE      50
#define ET08_PITCH_SPEED_SCALE  0.15f
#define GIMBAL_NO_FOLLOW_SCALE  4.5f

#define YAW_BRAKE_SPEED_EPS     10.0f
#define YAW_BRAKE_STABLE_COUNT  10U
#define YAW_BRAKE_TIMEOUT_MS    220U

#define PITCH_BRAKE_SPEED_EPS   20.0f
#define PITCH_BRAKE_STABLE_COUNT 3U
#define PITCH_BRAKE_TIMEOUT_MS  120U
#define PITCH_RELEASE_SPEED_PREDICT_GAIN 0.02f

/* 云台电机PID */
#define YAW_FOLLOW_ANGLE_KP     10.0f
#define YAW_FOLLOW_ANGLE_KI     0.0f
#define YAW_FOLLOW_ANGLE_KD     0.35f
#define YAW_FOLLOW_ANGLE_MAX_OUT 2000.0f

#define YAW_FOLLOW_SPEED_KP     10.0f
#define YAW_FOLLOW_SPEED_KI     3.0f
#define YAW_FOLLOW_SPEED_KD     0.0f
#define YAW_FOLLOW_SPEED_MAX_OUT 10000.0f

#define YAW_SEPARATE_ANGLE_KP   15.0f
#define YAW_SEPARATE_ANGLE_KI   0.00f
#define YAW_SEPARATE_ANGLE_KD   0.45f
#define YAW_SEPARATE_ANGLE_MAX_OUT 5000.0f

#define YAW_SEPARATE_SPEED_KP   18.0f
#define YAW_SEPARATE_SPEED_KI   2.0f
#define YAW_SEPARATE_SPEED_KD   0.0f
#define YAW_SEPARATE_SPEED_MAX_OUT 15000.0f

#define PITCH_ANGLE_KP          24.0f
#define PITCH_ANGLE_KI          0.05f
#define PITCH_ANGLE_KD          0.50f
#define PITCH_ANGLE_MAX_OUT     5000.0f

#define PITCH_SPEED_KP          9.0f
// #define PITCH_SPEED_KI          20.0f
#define PITCH_SPEED_KI          2.0f
#define PITCH_SPEED_KD          0.0f
#define PITCH_SPEED_MAX_OUT     15000.0f

/* Pitch重力补偿
 * 约定:
 * - 电机逆时针输出为正
 * - Pitch角度正方向为正
 * 调参时优先改 K 的大小; 若整体方向反了, 再改符号。
 */
#define PITCH_GRAVITY_FF_K      (+15000.0f)
#define PITCH_GRAVITY_FF_MAX    120000.0f
#define PITCH_GRAVITY_FF_OFFSET_DEG 15.0f
#define PITCH_FF_LPF            0.9f
#define PITCH_HOLD_KP           12.0f
#define PITCH_HOLD_KI           0.8f
#define PITCH_HOLD_KD           0.35f
#define PITCH_HOLD_I_LIMIT      800.0f

/* 云台环路策略 */
#define GIMBAL_YAW_INIT_LOOP          SPEED_LOOP
#define GIMBAL_YAW_RUN_LOOP_FOLLOW    ANGLE_LOOP
#define GIMBAL_YAW_RUN_LOOP_SEPARATE  ANGLE_LOOP
#define GIMBAL_PITCH_INIT_LOOP        SPEED_LOOP
#define GIMBAL_PITCH_RUN_LOOP_MANUAL  SPEED_LOOP
#define GIMBAL_PITCH_RUN_LOOP_HOLD    ANGLE_LOOP

/* 云台模式切换/目标更新 */
#define GIMBAL_SEPARATE_YAW_MAX_ANGLE    1080.0f
#define GIMBAL_SEPARATE_PITCH_MAX_ANGLE  VT_PITCH_MAX_ANGLE
#define GIMBAL_SEPARATE_PITCH_MIN_ANGLE  VT_PITCH_MIN_ANGLE

/*============================================================================
 * 发射参数
 *============================================================================*/
#define FRICTION_SPEED_TARGET   (-30000.0f)  // 摩擦轮目标转速
#define LOADER_SPEED_CONTINUOUS 20000.0f     // 连发拨盘速度
#define LOADER_SPEED_SINGLE     7000.0f      // 单发拨盘速度
#define LOADER_ANGLE_STEP       585.0f       // 单发角度步进 (45° * 13减速比)
#define LOADER_SINGLE_SETTLE_EPS 10.0f
#define LOADER_SINGLE_TIMEOUT_MS 350U
#define LOADER_CONTINUOUS_SLEW_PER_MS 40.0f

#define SHOOT_INTERVAL_MS       2000U        // 发射间隔

/* 发射电机PID */
#define FRICTION_SPEED_KP       10.0f
#define FRICTION_SPEED_KI       0.0f
#define FRICTION_SPEED_KD       0.0f
#define FRICTION_SPEED_MAX_OUT  12000.0f

#define LOADER_SPEED_KP         4.5f
#define LOADER_SPEED_KI         0.0f
#define LOADER_SPEED_KD         0.2f
#define LOADER_SPEED_MAX_OUT    22000.0f

#define LOADER_ANGLE_KP         3.5f
#define LOADER_ANGLE_KI         0.0f
#define LOADER_ANGLE_KD         0.5f
#define LOADER_ANGLE_MAX_OUT    12000.0f

/* 发射环路策略 */
#define FRICTION_INIT_LOOP               SPEED_LOOP
#define FRICTION_RUN_LOOP_STOP           SPEED_LOOP
#define FRICTION_RUN_LOOP_ON             SPEED_LOOP
#define LOADER_INIT_LOOP                 SPEED_LOOP
#define LOADER_RUN_LOOP_STOP             SPEED_LOOP
#define LOADER_RUN_LOOP_SINGLE           ANGLE_LOOP
#define LOADER_RUN_LOOP_CONTINUOUS       SPEED_LOOP

/*============================================================================
 * 电机通用参数
 *============================================================================*/
#define M3508_SPEED_MAX         30000.0f
#define GM6020_SPEED_MAX        3600.0f

/* 发射参数补充 */
#define FRICTION_TARGET_SPEED   (-30000.0f)
#define LOADER_CONTINUOUS_SPEED 20000.0f
#define LOADER_SINGLE_SPEED     7000.0f


#endif /* INFANTRY_CONFIG_H */
