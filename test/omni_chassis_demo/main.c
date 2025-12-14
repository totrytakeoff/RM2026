/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : 十字全向轮底盘遥控控制演示
 ******************************************************************************
 * @attention
 *
 * 本测试固件整合了遥控接收和电机控制，实现十字全向轮底盘的遥控控制。
 * 遥控器左摇杆控制底盘前进/后退和左转/右转，右摇杆控制底盘左平移/右平移。
 *
 * 电机布局（俯视）：
 *       2
 *       |
 *   1---+---3
 *       |
 *       4
 *
 * 1：右前轮，2：左前轮，3：右后轮，4：左后轮
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "cmsis_os.h"
#include "crc.h"
#include "dac.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp_init.h"
#include "bsp_log.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "remote_control.h"
#include "dji_motor.h"
#include "user_lib.h"

/* Private define ------------------------------------------------------------*/
#define CHASSIS_MOTOR_COUNT 4U
#define CHASSIS_UPDATE_INTERVAL_MS 20U  // 50Hz更新频率
#define CHASSIS_WHEEL_RADIUS 0.075f    // 轮子半径(m)
#define CHASSIS_WHEEL_BASE 0.34f        // 轮距(m)  轴距 25cm, 轮距 34cm
#define CHASSIS_MAX_VEL 16.68f         // 最大速度(m/s)，平动翻倍
#define CHASSIS_MAX_ROTATE 39.27f     // 最大旋转速度(rad/s)，旋转翻三倍
#define CHASSIS_BOOST_VEL 25.0f       // 冲刺速度(m/s)，比最大速度更快
#define CHASSIS_BRAKE_FACTOR 0.2f     // 刹车因子，0表示完全停止，1表示不刹车

#define M3508_SPEED_MAX 30000.0f   // deg/s, ~20 rps
#define M3508_SPEED_MIN (-M3508_SPEED_MAX)

/* 电机ID定义 - 与实际硬件连接对应 */
#define MOTOR_FRONT_RIGHT  1U
#define MOTOR_FRONT_LEFT   2U
#define MOTOR_BACK_RIGHT   3U
#define MOTOR_BACK_LEFT    4U

/* Private variables ---------------------------------------------------------*/
static RC_ctrl_t *rc_data = NULL;
static DJIMotorInstance *chassis_motors[CHASSIS_MOTOR_COUNT] = {NULL};

/* 底盘运动变量 */
static float chassis_vx = 0.0f;   // X轴速度(m/s)
static float chassis_vy = 0.0f;   // Y轴速度(m/s)
static float chassis_wz = 0.0f;   // Z轴旋转速度(rad/s)
static float boost_factor = 1.0f; // 冲刺因子，1.0表示正常速度，>1.0表示冲刺
static float brake_factor = 1.0f;  // 刹车因子，1.0表示不刹车，<1.0表示刹车

/* 运动学解算结果 - 四个轮子的速度 */
static float wheel_speeds[CHASSIS_MOTOR_COUNT] = {0.0f};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void Debug_DisableWatchdogs(void);
static void ChassisMotorsInit(void);
static void UpdateChassisKinematics(void);
static void ProcessRemoteControl(void);
static void SendChassisInfo(void);
static void StopAllMotors(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief 在调试模式下冻结 IWDG/WWDG，避免单步调试时复位
 */
static void Debug_DisableWatchdogs(void)
{
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}

/**
 * @brief 初始化底盘电机
 */
static void ChassisMotorsInit(void)
{
    const uint8_t motor_ids[CHASSIS_MOTOR_COUNT] = {
        MOTOR_FRONT_RIGHT, MOTOR_FRONT_LEFT, MOTOR_BACK_RIGHT, MOTOR_BACK_LEFT
    };
    
    for (uint8_t i = 0; i < CHASSIS_MOTOR_COUNT; i++) {
        Motor_Init_Config_s config = {
            .can_init_config = {
                .can_handle = &hcan1,
                .tx_id = motor_ids[i],
            },
            .controller_param_init_config = {
                .angle_PID = {
                    .Kp = 5.0f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .MaxOut = M3508_SPEED_MAX,
                    .IntegralLimit = 500.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                },
                .speed_PID = {
                    .Kp = 10.0f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .IntegralLimit = 3000.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                    .MaxOut = 12000.0f,
                },
                .current_PID = {
                    .Kp = 0.5f,
                    .Ki = 0.0f,
                    .Kd = 0.0f,
                    .IntegralLimit = 3000.0f,
                    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                    .MaxOut = 15000.0f,
                },
            },
            .controller_setting_init_config = {
                .angle_feedback_source = MOTOR_FEED,
                .speed_feedback_source = MOTOR_FEED,
                .outer_loop_type = SPEED_LOOP,
                .close_loop_type = ANGLE_LOOP | SPEED_LOOP | CURRENT_LOOP,
                .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
            },
            .motor_type = M3508,
        };
        
        chassis_motors[i] = DJIMotorInit(&config);
        LOGINFO("[chassis] Motor %d (ID %u) initialized", i, motor_ids[i]);
        
        if (chassis_motors[i] != NULL) {
            // 设置速度控制模式
            DJIMotorOuterLoop(chassis_motors[i], SPEED_LOOP);
            DJIMotorEnable(chassis_motors[i]);
        }
    }
}

/**
 * @brief 十字全向轮运动学解算
 * 
 * 输入：底盘期望速度 (vx, vy, wz)
 * 输出：四个轮子的速度 [右前, 左前, 右后, 左后]
 * 
 * 十字全向轮布局（俯视）：
 *       2 (前)
 *       |
 *   1---+---3
 *       |
 *       4 (后)
 * 
 * 十字全向轮正确运动学模型：
 * v1 = vy - wx + (L * wz)      // 右前轮
 * v2 = vx + wy + (L * wz)      // 左前轮  
 * v3 = -vy + wx + (L * wz)     // 右后轮
 * v4 = -vx - wy + (L * wz)     // 左后轮
 * 
 * 其中：vx为前进方向，vy为左移方向，wz为旋转角速度
 * L为底盘中心到轮子的距离
 */
static void UpdateChassisKinematics(void)
{
    // 运动学解算 - 修正十字全向轮公式
    float L = CHASSIS_WHEEL_BASE / 2.0f;
    
    // 十字全向轮的正确运动学解算
    // 注意：这里假设vy为前进方向，vx为左移方向
    // 修复旋转方向，使其符合直觉
    float v1 = chassis_vy - chassis_vx - (L * chassis_wz);  // 右前轮
    float v2 = chassis_vy + chassis_vx - (L * chassis_wz);  // 左前轮  
    float v3 = -chassis_vy + chassis_vx - (L * chassis_wz); // 右后轮
    float v4 = -chassis_vy - chassis_vx - (L * chassis_wz); // 左后轮
    
    // 将线速度转换为角速度 (rad/s)
    wheel_speeds[0] = v1 / CHASSIS_WHEEL_RADIUS; // 右前轮
    wheel_speeds[1] = v2 / CHASSIS_WHEEL_RADIUS; // 左前轮
    wheel_speeds[2] = v3 / CHASSIS_WHEEL_RADIUS; // 右后轮
    wheel_speeds[3] = v4 / CHASSIS_WHEEL_RADIUS; // 左后轮
    
    // 转换为度/秒，因为DJI电机控制期望速度单位为度/秒
    for (int i = 0; i < CHASSIS_MOTOR_COUNT; i++) {
        wheel_speeds[i] = wheel_speeds[i] * 180.0f / PI;
        
        // 限制在电机最大速度范围内
        wheel_speeds[i] = float_constrain(wheel_speeds[i], M3508_SPEED_MIN, M3508_SPEED_MAX);
        
        // 设置电机速度
        if (chassis_motors[i] != NULL) {
            DJIMotorSetRef(chassis_motors[i], wheel_speeds[i]);
        }
    }
}

/**
 * @brief 处理遥控器输入，转换为底盘速度命令
 * 
 * 摇杆控制映射：
 * - 左摇杆：前后左右控制底盘平动（上下为前进后退，左右为左右平移）
 * - 右摇杆左右：控制底盘旋转
 * - 右摇杆上下：控制冲刺和刹车
 */
static void ProcessRemoteControl(void)
{
    if (rc_data == NULL || !RemoteControlIsOnline()) {
        // 遥控器未连接，停止底盘
        chassis_vx = 0.0f;
        chassis_vy = 0.0f;
        chassis_wz = 0.0f;
        boost_factor = 1.0f;
        brake_factor = 1.0f;
        return;
    }
    
    const RC_ctrl_t *rc = &rc_data[TEMP];
    
    // 左摇杆控制底盘平动
    // 左右：rocker_l_，范围-660~660，映射到-CHASSIS_MAX_VEL~CHASSIS_MAX_VEL（控制vx左右平移）
    chassis_vx = -rc->rc.rocker_l_ / 660.0f * CHASSIS_MAX_VEL;
    
    // 前后：rocker_l1，范围-660~660，映射到-CHASSIS_MAX_VEL~CHASSIS_MAX_VEL（控制vy前进后退）
    chassis_vy = -rc->rc.rocker_l1 / 660.0f * CHASSIS_MAX_VEL;
    
    // 右摇杆左右控制底盘旋转
    // 旋转：rocker_r_，范围-660~660，映射到-CHASSIS_MAX_ROTATE~CHASSIS_MAX_ROTATE
    // 修复旋转方向，使其符合直觉
    chassis_wz = rc->rc.rocker_r_ / 660.0f * CHASSIS_MAX_ROTATE;
    
    // 右摇杆上下控制冲刺和刹车
    // rocker_r1范围-660~660，正值向上推（冲刺），负值向下推（刹车）
    if (rc->rc.rocker_r1 > 100) {  // 向上推，冲刺
        // 计算冲刺因子，从1.0到(CHASSIS_BOOST_VEL/CHASSIS_MAX_VEL)
        boost_factor = 1.0f + (rc->rc.rocker_r1 / 660.0f) * (CHASSIS_BOOST_VEL/CHASSIS_MAX_VEL - 1.0f);
        brake_factor = 1.0f;  // 不刹车
    } else if (rc->rc.rocker_r1 < -100) {  // 向下推，刹车
        // 计算刹车因子，从1.0到CHASSIS_BRAKE_FACTOR
        brake_factor = 1.0f + (rc->rc.rocker_r1 / 660.0f) * (1.0f - CHASSIS_BRAKE_FACTOR);
        boost_factor = 1.0f;  // 不冲刺
    } else {
        // 中间位置，不冲刺不刹车
        boost_factor = 1.0f;
        brake_factor = 1.0f;
    }
    
    // 如果按键按下，可以执行特定动作（可选）
    if (rc->key[KEY_PRESS].w) {
        // W键按下：向前
        chassis_vy = CHASSIS_MAX_VEL;
    } else if (rc->key[KEY_PRESS].s) {
        // S键按下：向后
        chassis_vy = -CHASSIS_MAX_VEL;
    }
    
    if (rc->key[KEY_PRESS].a) {
        // A键按下：向左平移
        chassis_vx = -CHASSIS_MAX_VEL;
    } else if (rc->key[KEY_PRESS].d) {
        // D键按下：向右平移
        chassis_vx = CHASSIS_MAX_VEL;
    }
    
    if (rc->key[KEY_PRESS].q) {
        // Q键按下：顺时针旋转
        chassis_wz = CHASSIS_MAX_ROTATE;
    } else if (rc->key[KEY_PRESS].e) {
        // E键按下：逆时针旋转
        chassis_wz = -CHASSIS_MAX_ROTATE;
    }
    
    // 应用冲刺和刹车因子
    chassis_vx *= boost_factor * brake_factor;
    chassis_vy *= boost_factor * brake_factor;
    
    // 速度死区处理，避免微小抖动
    chassis_vx = float_deadband(chassis_vx, -0.1f, 0.1f);
    chassis_vy = float_deadband(chassis_vy, -0.1f, 0.1f);
    chassis_wz = float_deadband(chassis_wz, -0.1f, 0.1f);
}

/**
 * @brief 停止所有底盘电机
 */
static void StopAllMotors(void)
{
    for (uint8_t i = 0; i < CHASSIS_MOTOR_COUNT; i++) {
        if (chassis_motors[i] != NULL) {
            DJIMotorStop(chassis_motors[i]);
        }
    }
}

/**
 * @brief 发送底盘信息（调试用）
 */
static void SendChassisInfo(void)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = HAL_GetTick();
    
    // 每隔500ms发送一次底盘状态
    if (now - last_send_tick >= 500U) {
        last_send_tick = now;
        
        // 通过USART6发送底盘状态（如果有遥测串口）
        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer),
            "[chassis] vx=%.2f vy=%.2f wz=%.2f\r\n"
            "Boost=%.2f Brake=%.2f\r\n"
            "Wheels[0]=%.1f [1]=%.1f [2]=%.1f [3]=%.1f\r\n",
            chassis_vx, chassis_vy, chassis_wz, boost_factor, brake_factor,
            wheel_speeds[0], wheel_speeds[1], wheel_speeds[2], wheel_speeds[3]);
        
        if (len > 0) {
            // 这里可以选择性地通过某个串口发送遥测数据
            // USARTSend(telemetry_usart, (uint8_t *)buffer, (uint16_t)len, USART_TRANSFER_BLOCKING);
        }
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    Debug_DisableWatchdogs();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    MX_SPI1_Init();
    MX_TIM4_Init();
    MX_TIM5_Init();
    MX_USART3_UART_Init();
    MX_RNG_Init();
    MX_RTC_Init();
    MX_TIM1_Init();
    MX_TIM10_Init();
    MX_USART1_UART_Init();
    MX_USART6_UART_Init();
    MX_TIM8_Init();
    MX_I2C2_Init();
    MX_I2C3_Init();
    MX_SPI2_Init();
    MX_CRC_Init();
    MX_DAC_Init();

    BSPInit();
    
    // 初始化底盘电机
    ChassisMotorsInit();
    
    // 初始化遥控器
    rc_data = RemoteControlInit(&huart3);

    LOGINFO("[chassis] Omni-directional chassis demo initialized");
    LOGINFO("[chassis] Using left stick for translation (up/down for forward/back, left/right for strafe)");
    LOGINFO("[chassis] Using right stick left/right for rotation, up/down for boost/brake");
    LOGINFO("[chassis] Keys: W/S for forward/back, A/D for strafe, Q/E for rotation");

    uint32_t last_update_tick = 0;

    while (1)
    {
        // 更新后台任务（包括电机控制）
        DaemonTask();
        DJIMotorControl();
        
        uint32_t now = HAL_GetTick();
        
        // 每隔CHASSIS_UPDATE_INTERVAL_MS更新一次底盘控制
        if (now - last_update_tick >= CHASSIS_UPDATE_INTERVAL_MS) {
            last_update_tick = now;
            
            // 处理遥控器输入
            ProcessRemoteControl();
            
            // 更新运动学解算
            UpdateChassisKinematics();
            
            // 发送底盘状态
            SendChassisInfo();
        }

        HAL_Delay(5);
    }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/**
 * @brief TIM14 1ms 中断回调，用于累加 HAL 的系统节拍
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14)
    {
        HAL_IncTick();
    }
}
/* USER CODE END 4 */