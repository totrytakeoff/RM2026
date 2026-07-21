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
#ifndef MINIMAL_DEBUG_ENABLE
#define MINIMAL_DEBUG_ENABLE                        0U /* 通用文本/RTT日志总开关 */
#endif

#define MINIMAL_DEBUG_MODE_TEXT                     (1U << 0)
#define MINIMAL_DEBUG_MODE_VOFA                     (1U << 1)
// #define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_TEXT | MINIMAL_DEBUG_MODE_VOFA)
#define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_TEXT )
// #define MINIMAL_DEBUG_MODE                          (MINIMAL_DEBUG_MODE_VOFA )

#define MINIMAL_DEBUG_UART_PORT                     6U   /* ET08调试固件: USART6, 115200 8N1 */
#define MINIMAL_DEBUG_UART_TIMEOUT_MS               100U
#define MINIMAL_DEBUG_UART_BAUDRATE                 115200U
#define MINIMAL_DEBUG_ALLOW_MIXED_STREAM            0U   /* 0: 单串口禁止Text+VOFA混发 */

#define MINIMAL_DEBUG_MOD_SYSTEM                    1U
#define MINIMAL_DEBUG_MOD_INPUT                     1U
#define MINIMAL_DEBUG_MOD_CHASSIS                   1U
#define MINIMAL_DEBUG_MOD_GIMBAL                    1U
#define MINIMAL_DEBUG_MOD_SHOOT                     0U

#define MINIMAL_DEBUG_TEXT_PERIOD_MS                200U /* 5Hz，给底盘/云台调参留出串口带宽 */
#define MINIMAL_DEBUG_VOFA_PERIOD_MS                20U  /* 50Hz */
#define GIMBAL_DEBUG_DETAIL_PERIOD_MS               100U /* 10Hz */

/*
 * 独立调参遥测：不复用通用日志任务，不执行字符串格式化。
 * 当前使用 UART6 向 VOFA+ 发送 JustFloat 自定义通道。
 */
#ifndef INFANTRY_TUNING_TELEMETRY_ENABLE
#define INFANTRY_TUNING_TELEMETRY_ENABLE            1U
#endif
#define INFANTRY_TUNING_UART_PORT                    6U
#define INFANTRY_TUNING_UART_BAUDRATE                115200U
#define INFANTRY_TUNING_TASK_PERIOD_MS               20U

/* VOFA+ 主调试轴：两轴共用同一套字段，切换时无需修改组帧代码。 */
#define INFANTRY_TUNING_GIMBAL_AXIS_YAW               0U
#define INFANTRY_TUNING_GIMBAL_AXIS_PITCH             1U
#ifndef INFANTRY_TUNING_GIMBAL_AXIS
#define INFANTRY_TUNING_GIMBAL_AXIS INFANTRY_TUNING_GIMBAL_AXIS_PITCH
#endif

#if (INFANTRY_TUNING_GIMBAL_AXIS != INFANTRY_TUNING_GIMBAL_AXIS_YAW) && \
    (INFANTRY_TUNING_GIMBAL_AXIS != INFANTRY_TUNING_GIMBAL_AXIS_PITCH)
#error "INFANTRY_TUNING_GIMBAL_AXIS must select YAW or PITCH"
#endif

#if INFANTRY_TUNING_UART_PORT == 1U
#define INFANTRY_TUNING_UART_HANDLE huart1
#elif INFANTRY_TUNING_UART_PORT == 3U
#define INFANTRY_TUNING_UART_HANDLE huart3
#elif INFANTRY_TUNING_UART_PORT == 6U
#define INFANTRY_TUNING_UART_HANDLE huart6
#else
#error "Unsupported INFANTRY_TUNING_UART_PORT, use 1/3/6."
#endif

/* 桥接到bsp_log */
#ifndef BSP_LOG_USE_UART
#define BSP_LOG_USE_UART                            0 /* 通用日志不得占用调参串口 */
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
 * 遥控后端：编译期只允许选择一个，不进行运行时仲裁或自动回退。
 *============================================================================*/
#define INFANTRY_REMOTE_BACKEND_ET08 1U
#define INFANTRY_REMOTE_BACKEND_DT7  2U
#define INFANTRY_REMOTE_BACKEND_VT   3U

#ifndef INFANTRY_REMOTE_BACKEND
#define INFANTRY_REMOTE_BACKEND INFANTRY_REMOTE_BACKEND_ET08
#endif

#if (INFANTRY_REMOTE_BACKEND != INFANTRY_REMOTE_BACKEND_ET08) && \
    (INFANTRY_REMOTE_BACKEND != INFANTRY_REMOTE_BACKEND_DT7) &&  \
    (INFANTRY_REMOTE_BACKEND != INFANTRY_REMOTE_BACKEND_VT)
#error "INFANTRY_REMOTE_BACKEND must select exactly one supported backend"
#endif

#if MINIMAL_DEBUG_ENABLE &&                                            \
    ((((INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_ET08) ||    \
       (INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_DT7)) &&    \
      (MINIMAL_DEBUG_UART_PORT == 3U)) ||                              \
     ((INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_VT) &&       \
      (MINIMAL_DEBUG_UART_PORT == 6U)))
#error "Debug UART conflicts with the selected remote-control backend"
#endif

#if INFANTRY_TUNING_TELEMETRY_ENABLE &&                               \
    ((((INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_ET08) ||   \
       (INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_DT7)) &&   \
      (INFANTRY_TUNING_UART_PORT == 3U)) ||                           \
     ((INFANTRY_REMOTE_BACKEND == INFANTRY_REMOTE_BACKEND_VT) &&      \
      (INFANTRY_TUNING_UART_PORT == 6U)))
#error "Tuning UART conflicts with the selected remote-control backend"
#endif

#if MINIMAL_DEBUG_ENABLE && INFANTRY_TUNING_TELEMETRY_ENABLE &&       \
    (MINIMAL_DEBUG_UART_PORT == INFANTRY_TUNING_UART_PORT)
#error "Generic debug and tuning telemetry cannot share one UART"
#endif

/*============================================================================
 * 系统配置
 *============================================================================*/
#define MAIN_LOOP_PERIOD_MS     20U     // 主循环周期(ms), 与omni_demo对齐
#define MOTOR_STABILIZE_TIME_MS 2000U   // 上电后电机稳定等待时间
#define INFANTRY_CPU_FREQUENCY_MHZ       168U
#define INFANTRY_IMU_INIT_TIMEOUT_MS      15000U

/* FreeRTOS迁移阶段任务周期：控制环保持minimal基线，快环独立调度。 */
#define INFANTRY_INS_TASK_PERIOD_MS         1U
#define INFANTRY_MOTOR_TASK_PERIOD_MS       5U
#define INFANTRY_HEALTH_TASK_PERIOD_MS      5U
#define INFANTRY_DIAGNOSTICS_TASK_PERIOD_MS 10U

/* 20 ms 控制任务失活时，5 ms 电机任务独立执行的命令租约。 */
#define INFANTRY_MOTOR_COMMAND_TIMEOUT_MS    100U

#if INFANTRY_MOTOR_COMMAND_TIMEOUT_MS <= MAIN_LOOP_PERIOD_MS
#error "INFANTRY_MOTOR_COMMAND_TIMEOUT_MS must exceed one control period"
#endif
#if INFANTRY_MOTOR_COMMAND_TIMEOUT_MS >= 0x80000000UL
#error "INFANTRY_MOTOR_COMMAND_TIMEOUT_MS exceeds the wrap-safe range"
#endif

/* 任务健康判定：连续超期、心跳丢失或栈余量不足均触发安全停机。 */
#define INFANTRY_TASK_STARTUP_GRACE_MS          100U
#define INFANTRY_TASK_HEARTBEAT_PERIODS         4U
#define INFANTRY_TASK_DEADLINE_TOLERANCE_PERCENT 25U
#define INFANTRY_TASK_MAX_CONSECUTIVE_OVERRUNS  3U
#define INFANTRY_TASK_MIN_STACK_FREE_WORDS      64U
#define INFANTRY_TASK_STACK_SAMPLE_INTERVAL     100U

/* Only the health task refreshes IWDG; a stalled scheduler or critical task resets. */
#define INFANTRY_HARDWARE_WATCHDOG_TIMEOUT_MS   1000U

/*
 * 调试阶段安全策略：SA 使用直接电平门；单个电机掉线只隔离该电机并报警，
 * 不连带关闭其余在线执行器。遥控失联、初始化失败、INS/任务故障仍会停机。
 */
#define INFANTRY_SAFETY_REQUIRE_EXPLICIT_REARM  0U
#define INFANTRY_SAFETY_GATE_ON_MOTOR_HEALTH    0U

#if (INFANTRY_SAFETY_REQUIRE_EXPLICIT_REARM > 1U) || \
    (INFANTRY_SAFETY_GATE_ON_MOTOR_HEALTH > 1U)
#error "INFANTRY_SAFETY_* options must be 0 or 1"
#endif

#if INFANTRY_HARDWARE_WATCHDOG_TIMEOUT_MS <= INFANTRY_HEALTH_TASK_PERIOD_MS
#error "INFANTRY_HARDWARE_WATCHDOG_TIMEOUT_MS must exceed one health period"
#endif

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
#define ET08_ONLINE_TIMEOUT_MS  100U
#define DT7_ONLINE_TIMEOUT_MS   100U
#define ET08_STICK_DEADZONE_RAW 100     /* 当前ET08中位实测最大偏移约74 */
#define ET08_STICK_FULL_SCALE_RAW 660.0f
#define DT7_STICK_DEADZONE_RAW  10
#define DT7_STICK_FULL_SCALE_RAW 660.0f
#define VT_STICK_DEADZONE_RAW   10
#define VT_STICK_FULL_SCALE_RAW 660.0f

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
#define VT_CHASSIS_BASE_INTENT  0.60f   // 无量纲键盘基础意图
#define VT_CHASSIS_FAST_MULT    (1.0f / VT_CHASSIS_BASE_INTENT)
#define VT_CHASSIS_SLOW_MULT    0.50f

/* VT云台灵敏度 */
#define VT_MOUSE_YAW_INTENT_PER_COUNT   0.010f
#define VT_MOUSE_PITCH_INTENT_PER_COUNT 0.010f

/*============================================================================
 * 裁判系统(只读联锁)配置
 *============================================================================*/
#define REFEREE_ENABLE                   0U
#define REFEREE_UART                     huart1
#define REFEREE_CHASSIS_POWER_NOMINAL    80.0f
#define REFEREE_CHASSIS_SCALE_MIN        0.30f
#define REFEREE_HEAT_STOP_RATIO          0.95f
#define REFEREE_FRICTION_SLOW_RATIO      0.80f

#if REFEREE_ENABLE && MINIMAL_DEBUG_ENABLE && (MINIMAL_DEBUG_UART_PORT == 1U)
#error "Debug UART conflicts with the enabled referee link"
#endif

/*============================================================================
 * 底盘参数
 *============================================================================*/
/* 正式应用的电机速度单位契约。 */
#define M3508_ROTOR_SPEED_LIMIT_RAD_S 523.5987756f /* 5000 rpm */
#define GM6020_SPEED_LIMIT_DEG_S      3600.0f      /* 位置串级仍使用 deg/deg/s */
#define GM6020_COMMAND_LIMIT          30000.0f      /* CAN 电压/电流命令绝对上限 */

#define CHASSIS_WHEEL_RADIUS    0.075f  // 轮子半径(m)
#define CHASSIS_WHEEL_BASE      0.34f   // 轮距(m)

/* 标准 M3508 行星减速箱；轮端速度换算必须使用真实减速比。 */
#define CHASSIS_MOTOR_REDUCTION_RATIO 19.20320856f

/*
 * 满意图直接映射到 M3508 配置转速上限，不再设置一层保守“遥控速度”。
 * 平移+旋转叠加超限时，最终四轮在执行层统一等比例缩放。
 */
#define CHASSIS_MAX_TRANSLATION_SPEED \
    (M3508_ROTOR_SPEED_LIMIT_RAD_S * CHASSIS_WHEEL_RADIUS / \
     CHASSIS_MOTOR_REDUCTION_RATIO)
#define CHASSIS_MAX_ROTATION_SPEED_RAD_S \
    (CHASSIS_MAX_TRANSLATION_SPEED / (CHASSIS_WHEEL_BASE * 0.5f))

#define CHASSIS_MOTOR_SPEED_DEADZONE_RAD_S 2.09439510f
#define CHASSIS_DEADZONE_VX     0.08f   // 底盘输入死区(vx)
#define CHASSIS_DEADZONE_VY     0.08f   // 底盘输入死区(vy)
#define CHASSIS_DEADZONE_WZ     0.10f   // 底盘输入死区(wz)
#define CHASSIS_SPEED_FILTER_COEF 0.15f // 一阶低通，0为不滤波，越大响应越慢

/* 底盘M3508转子速度环，参考、反馈和误差统一使用rad/s。 */
#define CHASSIS_SPEED_KP        229.18312f
#define CHASSIS_SPEED_KI        2.291831f
#define CHASSIS_SPEED_KD        0.0f
#define CHASSIS_SPEED_MAX_OUT   15000.0f

/* 底盘环路策略 */
#define CHASSIS_INIT_LOOP              SPEED_LOOP
#define CHASSIS_RUN_LOOP_STOP          SPEED_LOOP
#define CHASSIS_RUN_LOOP_NORMAL        SPEED_LOOP

/* 底盘-云台相对姿态标定 */
#define YAW_CHASSIS_ALIGN_ECD          7821U
#define YAW_ALIGN_ANGLE_DEG            (YAW_CHASSIS_ALIGN_ECD * ECD_ANGLE_COEF_DJI)
#define IMU_YAW_LOGIC_ZERO_TOTAL_DEG   364.30f
#define YAW_OFFSET_LOGIC_ZERO_DEG      0.0f

/* FOLLOW模式: 相对夹角闭环生成底盘角速度 */
#define CHASSIS_FOLLOW_WZ_KP           30.0f
#define CHASSIS_FOLLOW_WZ_KI           0.0f
#define CHASSIS_FOLLOW_WZ_KD           0.80f
#define CHASSIS_FOLLOW_WZ_MAX          10.0f
#define CHASSIS_FOLLOW_WZ_I_MAX_RAD_S  0.26179939f
#define CHASSIS_FOLLOW_MOTOR_SPEED_DEADZONE_RAD_S 0.34906585f

/* 小陀螺 */
#define CHASSIS_SPIN_SPEED_RATIO       0.60f
#define CHASSIS_SPIN_SPEED_RAD_S       \
    (CHASSIS_MAX_ROTATION_SPEED_RAD_S * CHASSIS_SPIN_SPEED_RATIO)

/*============================================================================
 * 云台参数
 *============================================================================*/
/* 满意图对应的执行层速度；沿用已上机 demo 的 0.23/0.15 速度比例。 */
#define GIMBAL_YAW_MAX_SPEED_DEG_S      (GM6020_SPEED_LIMIT_DEG_S * 0.23f)
#define GIMBAL_PITCH_MAX_SPEED_DEG_S    (GM6020_SPEED_LIMIT_DEG_S * 0.15f)

/* 执行层机械角度限位，对遥控、视觉和自动控制目标统一生效。 */
#define GIMBAL_PITCH_MIN_DEG             -30.0f  /* 抬头限位 */
#define GIMBAL_PITCH_MAX_DEG              20.0f  /* 低头限位 */

/* 本车正控制轴输出需经驱动反向后才产生正 IMU Pitch。 */
#define PITCH_MOTOR_OUTPUT_REVERSED         1U

/* 安全门重新放行后的 Pitch 目标；采用 IMU 物理角，不依赖编码器零点。 */
#define PITCH_STARTUP_CENTER_DEG             0.0f

#define YAW_BRAKE_SPEED_EPS     10.0f
#define YAW_BRAKE_STABLE_COUNT  10U
#define YAW_BRAKE_TIMEOUT_MS    220U

/* 防止控制任务异常延迟后一次性推进过大的 Pitch 角度目标。 */
#define PITCH_TARGET_INTEGRATION_MAX_DT_MS 40U

/* 云台电机PID */
#define YAW_ANGLE_KP            15.0f
#define YAW_ANGLE_KI            0.00f
#define YAW_ANGLE_KD            2.00f
#define YAW_ANGLE_MAX_OUT       5000.0f

#define YAW_SPEED_KP            18.0f
#define YAW_SPEED_KI            10.0f
#define YAW_SPEED_KD            0.0f
#define YAW_SPEED_MAX_OUT       15000.0f
#define YAW_SPEED_I_MAX         4000.0f

/* 小陀螺下底盘基座角速度对 Yaw 的电流扰动补偿。 */
#define YAW_BASE_RATE_CURRENT_FF_K          900.0f // 前馈
#define YAW_BASE_RATE_CURRENT_FF_MAX        10000.0f
#define YAW_BASE_RATE_FF_LPF                0.85f
#define YAW_BASE_RATE_FF_DEADBAND_RAD_S     0.20f

#define PITCH_ANGLE_KP          100.0f
#define PITCH_ANGLE_KI          0.0f
#define PITCH_ANGLE_KD          0.50f
#define PITCH_ANGLE_MAX_OUT     5000.0f

#define PITCH_SPEED_KP          10.0f
#define PITCH_SPEED_KI          15.0f
#define PITCH_SPEED_KD          0.0f
#define PITCH_SPEED_MAX_OUT     15000.0f
#define PITCH_SPEED_I_MAX       5000.0f

/* Pitch重力补偿
 * 约定:
 * - 控制坐标统一为 IMU Pitch 正方向；
 * - 本车最终电机执行输出反向，因此保持本车水平姿态需要负的控制轴前馈；
 * - 当前 K=0 是去掉重力补偿后的临时采样配置，不代表最终标定值。
 */
#define PITCH_GRAVITY_FF_K             (-0.0f)
#define PITCH_GRAVITY_FF_MAX           4000.0f
#define PITCH_GRAVITY_HORIZONTAL_DEG   0.0f
#define PITCH_FF_LPF                   0.8f

/* 云台模式切换/目标更新 */
#define GIMBAL_SEPARATE_YAW_MAX_ANGLE    1080.0f

/*============================================================================
 * 发射参数
 *============================================================================*/
#define FRICTION_TARGET_SPEED_RAD_S (-M3508_ROTOR_SPEED_LIMIT_RAD_S)
#define LOADER_CONTINUOUS_SPEED_DEG_S 20000.0f      // M2006位置串级速度
#define LOADER_ANGLE_STEP       585.0f       // 单发角度步进 (45° * 13减速比)
#define LOADER_SINGLE_SETTLE_EPS 10.0f
#define LOADER_SINGLE_TIMEOUT_MS 350U
#define LOADER_CONTINUOUS_SLEW_PER_MS 40.0f

#define SHOOT_INTERVAL_MS       2000U        // 发射间隔

/* 发射电机PID */
#define FRICTION_SPEED_KP       572.95780f /* M3508速度误差单位rad/s */
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

#endif /* INFANTRY_CONFIG_H */
