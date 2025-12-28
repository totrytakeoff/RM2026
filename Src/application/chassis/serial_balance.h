/**
 * @file serial_balance.h
 * @brief 串联腿平衡底盘控制头文件
 * @details 定义串联腿控制系统的接口和数据结构
 */

#ifndef SERIAL_BALANCE_H
#define SERIAL_BALANCE_H

#include "stdint.h"
#include "main.h"
#include "robot_def.h"
#include "serial_leg.h"

// 串联腿控制任务频率定义
#define SERIAL_CONTROL_FREQ 1000.0f  // 控制频率 1000Hz
#define SERIAL_CONTROL_PERIOD (1.0f / SERIAL_CONTROL_FREQ)  // 控制周期 1ms

// 串联腿初始化函数
/**
 * @brief 串联腿平衡底盘初始化
 * @details 初始化所有硬件模块、控制参数和状态变量
 * 包括：IMU、电机、CAN通信、超级电容、裁判系统等
 */
void SerialBalanceInit(void);

/**
 * @brief 串联腿平衡底盘主控制任务
 * @details 串联腿控制主循环，按固定频率运行
 * 包含完整的控制流程：状态估计、步态规划、LQR控制、输出映射等
 */
void SerialBalanceTask(void);

// 串联腿状态获取函数
/**
 * @brief 获取串联腿当前控制模式
 * @return 当前控制模式枚举值
 */
SerialLegControlMode_e SerialGetControlMode(void);

/**
 * @brief 获取串联腿步态相位
 * @return 步态相位 [0, 2π]
 */
float SerialGetGaitPhase(void);

/**
 * @brief 获取串联腿期望速度
 * @return 期望速度 [m/s]
 */
float SerialGetDesiredVelocity(void);

/**
 * @brief 获取串联腿实际速度
 * @return 实际速度 [m/s]
 */
float SerialGetActualVelocity(void);

/**
 * @brief 获取串联腿机体姿态
 * @param pitch 俯仰角指针 [rad]
 * @param roll 横滚角指针 [rad] 
 * @param yaw 偏航角指针 [rad]
 */
void SerialGetChassisAttitude(float *pitch, float *roll, float *yaw);

// 串联腿状态设置函数
/**
 * @brief 设置串联腿期望速度
 * @param velocity 期望速度 [m/s]
 */
void SerialSetDesiredVelocity(float velocity);

/**
 * @brief 设置串联腿控制模式
 * @param mode 控制模式枚举值
 */
void SerialSetControlMode(SerialLegControlMode_e mode);

/**
 * @brief 设置串联腿步态参数
 * @param params 步态参数结构体指针
 */
void SerialSetGaitParams(const SerialGaitParam_s *params);

/**
 * @brief 设置串联腿LQR增益
 * @param gains LQR增益结构体指针
 */
void SerialSetLQRGains(const SerialLQRGains_s *gains);

// 串联腿安全控制函数
/**
 * @brief 紧急停止串联腿
 * @details 立即停止所有电机输出，进入安全状态
 */
void SerialEmergencyStop(void);

/**
 * @brief 串联腿安全检查
 * @return 安全状态 [0/1] - 0表示不安全
 */
uint8_t SerialSafetyCheck(void);

/**
 * @brief 串联腿故障诊断
 * @param error_code 错误代码输出
 * @return 故障状态 [0/1] - 0表示无故障
 */
uint8_t SerialFaultDiagnosis(uint16_t *error_code);

// 串联腿数据记录函数
/**
 * @brief 记录串联腿运行数据
 * @details 将关键状态变量记录到缓冲区，用于后续分析
 */
void SerialDataLogging(void);

/**
 * @brief 获取串联腿数据记录
 * @param buffer 数据缓冲区指针
 * @param size 缓冲区大小
 * @return 实际记录的数据长度
 */
uint32_t SerialGetLoggedData(uint8_t *buffer, uint32_t size);

// 串联腿调试接口
/**
 * @brief 串联腿单关节调试模式
 * @param leg_side 腿侧选择 [0:左腿, 1:右腿]
 * @param joint_id 关节ID [0:髋, 1:膝, 2:踝]
 * @param enable 使能状态 [0/1]
 */
void SerialSingleJointDebug(uint8_t leg_side, uint8_t joint_id, uint8_t enable);

/**
 * @brief 设置单关节目标角度
 * @param leg_side 腿侧选择 [0:左腿, 1:右腿]
 * @param joint_id 关节ID [0:髋, 1:膝, 2:踝] 
 * @param angle 目标角度 [rad]
 */
void SerialSetSingleJointAngle(uint8_t leg_side, uint8_t joint_id, float angle);

/**
 * @brief 串联腿步态相位重置
 * @param phase 新的步态相位 [0, 2π]
 */
void SerialResetGaitPhase(float phase);

/**
 * @brief 串联腿状态复位
 * @details 将所有状态变量复位到初始值
 */
void SerialStateReset(void);

// 串联腿性能监控
/**
 * @brief 获取串联腿控制性能指标
 * @param settling_time 稳定时间输出 [ms]
 * @param overshoot 超调量输出 [%]
 * @param steady_error 稳态误差输出 [rad]
 */
void SerialGetPerformanceMetrics(float *settling_time, float *overshoot, float *steady_error);

/**
 * @brief 获取串联腿电机状态
 * @param motor_currents 电机电流数组 [A]
 * @param motor_temperatures 电机温度数组 [°C]
 * @param motor_speeds 电机速度数组 [rad/s]
 */
void SerialGetMotorStatus(float *motor_currents, float *motor_temperatures, float *motor_speeds);

/**
 * @brief 获取串联腿能耗统计
 * @param total_energy 总能量消耗 [J]
 * @param average_power 平均功率 [W]
 * @param peak_power 峰值功率 [W]
 */
void SerialGetPowerConsumption(float *total_energy, float *average_power, float *peak_power);

// 串联腿通信接口
/**
 * @brief 串联腿CAN通信处理
 * @details 处理来自上位机的控制指令
 */
void SerialCANCommHandler(void);

/**
 * @brief 串联腿数据上传
 * @details 将状态数据上传到上位机
 */
void SerialDataUpload(void);

// 串联腿高级功能
/**
 * @brief 串联腿自适应控制使能
 * @param enable 使能状态 [0/1]
 */
void SerialAdaptiveControlEnable(uint8_t enable);

/**
 * @brief 串联腿学习控制使能
 * @param enable 使能状态 [0/1]
 */
void SerialLearningControlEnable(uint8_t enable);

/**
 * @brief 串联腿故障恢复
 * @param recovery_mode 恢复模式
 * @return 恢复结果 [0/1] - 0表示失败
 */
uint8_t SerialFaultRecovery(uint8_t recovery_mode);

// 串联腿版本信息
/**
 * @brief 获取串联腿固件版本
 * @return 版本号
 */
uint32_t SerialGetFirmwareVersion(void);

/**
 * @brief 获取串联腿硬件版本
 * @return 硬件版本号
 */
uint32_t SerialGetHardwareVersion(void);

/**
 * @brief 获取串联腿编译时间
 * @return 编译时间戳
 */
uint32_t SerialGetBuildTime(void);

// 串联腿配置参数
/**
 * @brief 串联腿参数保存
 * @details 将当前参数保存到Flash存储
 * @return 保存结果 [0/1] - 0表示失败
 */
uint8_t SerialParamsSave(void);

/**
 * @brief 串联腿参数加载
 * @details 从Flash存储加载参数
 * @return 加载结果 [0/1] - 0表示失败
 */
uint8_t SerialParamsLoad(void);

/**
 * @brief 串联腿参数恢复出厂设置
 * @details 将所有参数恢复为默认值
 */
void SerialParamsReset(void);

// 串联腿测试模式
/**
 * @brief 串联腿自检模式
 * @details 执行完整的硬件和软件自检
 * @return 自检结果 [0/1] - 0表示有故障
 */
uint8_t SerialSelfTest(void);

/**
 * @brief 串联腿老化测试
 * @param duration 测试持续时间 [s]
 * @param test_mode 测试模式选择
 */
void SerialAgingTest(uint32_t duration, uint8_t test_mode);

/**
 * @brief 串联腿性能测试
 * @details 执行标准化的性能测试
 * @param test_results 测试结果结构体指针
 */
void SerialPerformanceTest(SerialPerformanceResults_s *test_results);

// 错误代码定义
#define SERIAL_ERROR_NONE                0x0000  // 无错误
#define SERIAL_ERROR_IMU_TIMEOUT         0x0001  // IMU通信超时
#define SERIAL_ERROR_MOTOR_TIMEOUT       0x0002  // 电机通信超时
#define SERIAL_ERROR_JOINT_LIMIT         0x0004  // 关节限位触发
#define SERIAL_ERROR_ATTITUDE_OVERFLOW   0x0008  // 姿态角超限
#define SERIAL_ERROR_GAIT_FAULT          0x0010  // 步态异常
#define SERIAL_ERROR_POWER_FAULT         0x0020  // 电源故障
#define SERIAL_ERROR_TEMPERATURE_HIGH    0x0040  // 温度过高
#define SERIAL_ERROR_CURRENT_HIGH        0x0080  // 电流过大
#define SERIAL_ERROR_CAN_COMM_FAULT      0x0100  // CAN通信故障
#define SERIAL_ERROR_SUPER_CAP_FAULT     0x0200  // 超级电容故障

// 性能指标定义
#define SERIAL_PERFORMANCE_EXCELLENT     0  // 优秀
#define SERIAL_PERFORMANCE_GOOD          1  // 良好  
#define SERIAL_PERFORMANCE_AVERAGE       2  // 一般
#define SERIAL_PERFORMANCE_POOR          3  // 较差

// 测试模式定义
#define SERIAL_TEST_MODE_STATIC          0  // 静态测试
#define SERIAL_TEST_MODE_DYNAMIC         1  // 动态测试
#define SERIAL_TEST_MODE_STRESS          2  // 压力测试
#define SERIAL_TEST_MODE_ENDURANCE       3  // 耐久测试

// 恢复模式定义
#define SERIAL_RECOVERY_MODE_SOFT        0  // 软恢复
#define SERIAL_RECOVERY_MODE_HARD        1  // 硬恢复
#define SERIAL_RECOVERY_MODE_EMERGENCY   2  // 紧急恢复

// 宏定义
#define SERIAL_DEG2RAD(x)                ((x) * 0.01745329251994329576923690768489f)
#define SERIAL_RAD2DEG(x)                ((x) * 57.295779513082320876798154814105f)
#define SERIAL_MAX(a, b)                 (((a) > (b)) ? (a) : (b))
#define SERIAL_MIN(a, b)                 (((a) < (b)) ? (a) : (b))
#define SERIAL_CLAMP(x, min, max)        (SERIAL_MIN(SERIAL_MAX((x), (min)), (max)))
#define SERIAL_SIGN(x)                   (((x) > 0) ? 1 : (((x) < 0) ? -1 : 0))

// 调试宏
#ifdef SERIAL_LEG_DEBUG
    #define SERIAL_DEBUG_LOG(fmt, ...)     LOGINFO("[SERIAL] " fmt, ##__VA_ARGS__)
    #define SERIAL_DEBUG_ERROR(fmt, ...)   LOGERROR("[SERIAL] " fmt, ##__VA_ARGS__)
#else
    #define SERIAL_DEBUG_LOG(fmt, ...)
    #define SERIAL_DEBUG_ERROR(fmt, ...)
#endif

// 性能监控结构体定义
typedef struct {
    float max_settling_time;      // 最大稳定时间 [ms]
    float max_overshoot;          // 最大超调量 [%]
    float max_steady_error;       // 最大稳态误差 [rad]
    float average_power_consumption;  // 平均功耗 [W]
    float max_motor_temperature;  // 最高电机温度 [°C]
    uint32_t fault_count;         // 故障次数
    uint8_t overall_performance;  // 综合性能评级
} SerialPerformanceResults_s;

// 数据记录结构体定义
typedef struct {
    uint32_t timestamp;           // 时间戳 [ms]
    float pitch;                  // 俯仰角 [rad]
    float pitch_rate;             // 俯仰角速度 [rad/s]
    float roll;                   // 横滚角 [rad]
    float roll_rate;              // 横滚角速度 [rad/s]
    float yaw;                    // 偏航角 [rad]
    float yaw_rate;               // 偏航角速度 [rad/s]
    float velocity;               // 机体速度 [m/s]
    float left_leg_length;        // 左腿长度 [m]
    float right_leg_length;       // 右腿长度 [m]
    float left_foot_force;        // 左足力 [N]
    float right_foot_force;       // 右足力 [N]
    uint8_t gait_phase;           // 步态相位
    uint8_t control_mode;         // 控制模式
    uint16_t error_flags;         // 错误标志
} SerialLogData_s;

#endif // SERIAL_BALANCE_H