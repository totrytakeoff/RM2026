/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Infantry ET08 minimal chassis demo
 ******************************************************************************
 * @attention
 *
 * 仅保留底盘测试链路：
 * ET08(SBUS) -> vx/vy/wz -> 十字全向轮逆解 -> M3508速度环
 *
 * - SBUS 输入: USART3
 * - 调试输出: USART6
 * - 底盘电机: CAN1 ID 1/2/3/4 (FR/FL/BR/BL)
 *
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
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "dji_motor.h"
#include "et08_remote.h"
#include "minimal_config.h"

/* Private define ------------------------------------------------------------*/
#define CHASSIS_MOTOR_COUNT 4U
#define TELEMETRY_RX_DUMMY 32U
#define TELEMETRY_PERIOD_MS 100U
#define RC_INPUT_DEADZONE 50
#define WHEEL_SPEED_LIMIT_DPS 30000.0f

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    float vx;
    float vy;
    float wz;
} ChassisCmd_t;

/* Private variables ---------------------------------------------------------*/
static ET08_Ctrl_t *et08_ctrl = NULL;
static USARTInstance *telemetry_usart = NULL;

static DJIMotorInstance *motor_fr = NULL;
static DJIMotorInstance *motor_fl = NULL;
static DJIMotorInstance *motor_br = NULL;
static DJIMotorInstance *motor_bl = NULL;

static uint8_t chassis_enabled = 0U;
static uint8_t last_online_state = 0U;
static float wheel_speed_ref[CHASSIS_MOTOR_COUNT] = {0.0f};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

static void Debug_DisableWatchdogs(void);
static void InitTelemetryUsart(void);
static void TelemetrySendBuffer(const uint8_t *buffer, uint16_t len);
static void TelemetrySendString(const char *str);
static void TelemetryPrintf(const char *fmt, ...);

static int16_t ApplyDeadzone(int16_t v, int16_t deadzone);
static float ClampFloat(float value, float min_value, float max_value);
static void OmniInverseKinematics(float vx, float vy, float wz, float out[4]);

static void ChassisMotorsInit(void);
static void ChassisStop(void);
static void ChassisApplyCommand(const ChassisCmd_t *cmd);
static bool BuildChassisCommandFromEt08(ChassisCmd_t *cmd);
static void SendTelemetry(void);

/* Private user code ---------------------------------------------------------*/
static void Debug_DisableWatchdogs(void)
{
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}

static void InitTelemetryUsart(void)
{
    USART_Init_Config_s config = {
        .module_callback = NULL,
        .recv_buff_size = TELEMETRY_RX_DUMMY,
        .usart_handle = &huart6,
    };
    telemetry_usart = USARTRegister(&config);
    TelemetrySendString("[chs_et08] USART6 telemetry ready\\r\\n");
}

static void TelemetrySendBuffer(const uint8_t *buffer, uint16_t len)
{
    if (telemetry_usart == NULL || buffer == NULL || len == 0U)
        return;

    USARTSend(telemetry_usart, (uint8_t *)buffer, len, USART_TRANSFER_BLOCKING);
}

static void TelemetrySendString(const char *str)
{
    if (str == NULL)
        return;
    TelemetrySendBuffer((const uint8_t *)str, (uint16_t)strlen(str));
}

static void TelemetryPrintf(const char *fmt, ...)
{
    if (telemetry_usart == NULL || fmt == NULL)
        return;

    char msg[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (n <= 0)
        return;

    uint16_t len = (n < (int)sizeof(msg)) ? (uint16_t)n : (uint16_t)(sizeof(msg) - 1U);
    TelemetrySendBuffer((const uint8_t *)msg, len);
}

static int16_t ApplyDeadzone(int16_t v, int16_t deadzone)
{
    if (v > -deadzone && v < deadzone) {
        return 0;
    }
    return v;
}

static float ClampFloat(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static void OmniInverseKinematics(float vx, float vy, float wz, float out[4])
{
    const float l = CHASSIS_WHEEL_BASE * 0.5f;
    const float v_fr = vy - vx - l * wz;
    const float v_fl = vy + vx - l * wz;
    const float v_br = -vy + vx - l * wz;
    const float v_bl = -vy - vx - l * wz;

    out[0] = v_fr / CHASSIS_WHEEL_RADIUS;
    out[1] = v_fl / CHASSIS_WHEEL_RADIUS;
    out[2] = v_br / CHASSIS_WHEEL_RADIUS;
    out[3] = v_bl / CHASSIS_WHEEL_RADIUS;
}

static void ChassisMotorsInit(void)
{
    Motor_Init_Config_s config = {
        .motor_type = M3508,
        .can_init_config = {
            .can_handle = &CHASSIS_CAN,
            .tx_id = CHASSIS_MOTOR_FR_ID,
        },
        .controller_param_init_config = {
            .speed_PID = {
                .Kp = CHASSIS_SPEED_KP,
                .Ki = CHASSIS_SPEED_KI,
                .Kd = CHASSIS_SPEED_KD,
                .MaxOut = CHASSIS_SPEED_MAX_OUT,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
            },
            .current_PID = {
                .Kp = 0.35f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .IntegralLimit = 1000.0f,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .MaxOut = 8000.0f,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = SPEED_LOOP | CURRENT_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
    };

    motor_fr = DJIMotorInit(&config);
    if (motor_fr) {
        DJIMotorOuterLoop(motor_fr, SPEED_LOOP);
        DJIMotorStop(motor_fr);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_FL_ID;
    motor_fl = DJIMotorInit(&config);
    if (motor_fl) {
        DJIMotorOuterLoop(motor_fl, SPEED_LOOP);
        DJIMotorStop(motor_fl);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_BR_ID;
    motor_br = DJIMotorInit(&config);
    if (motor_br) {
        DJIMotorOuterLoop(motor_br, SPEED_LOOP);
        DJIMotorStop(motor_br);
    }

    config.can_init_config.tx_id = CHASSIS_MOTOR_BL_ID;
    motor_bl = DJIMotorInit(&config);
    if (motor_bl) {
        DJIMotorOuterLoop(motor_bl, SPEED_LOOP);
        DJIMotorStop(motor_bl);
    }

    TelemetrySendString("[chs_et08] chassis motors initialized\\r\\n");
}

static void ChassisStop(void)
{
    wheel_speed_ref[0] = 0.0f;
    wheel_speed_ref[1] = 0.0f;
    wheel_speed_ref[2] = 0.0f;
    wheel_speed_ref[3] = 0.0f;

    chassis_enabled = 0U;

    if (motor_fr)
        DJIMotorStop(motor_fr);
    if (motor_fl)
        DJIMotorStop(motor_fl);
    if (motor_br)
        DJIMotorStop(motor_br);
    if (motor_bl)
        DJIMotorStop(motor_bl);
}

static void ChassisApplyCommand(const ChassisCmd_t *cmd)
{
    if (cmd == NULL)
        return;

    if (!chassis_enabled) {
        if (motor_fr)
            DJIMotorEnable(motor_fr);
        if (motor_fl)
            DJIMotorEnable(motor_fl);
        if (motor_br)
            DJIMotorEnable(motor_br);
        if (motor_bl)
            DJIMotorEnable(motor_bl);
        chassis_enabled = 1U;
    }

    float wheel_speed_rad_s[CHASSIS_MOTOR_COUNT] = {0.0f};
    OmniInverseKinematics(cmd->vx, cmd->vy, cmd->wz, wheel_speed_rad_s);

    for (uint8_t i = 0; i < CHASSIS_MOTOR_COUNT; i++) {
        float speed_dps = wheel_speed_rad_s[i] * 180.0f / (float)M_PI;
        speed_dps *= CHASSIS_SPEED_SCALE;

        if (fabsf(speed_dps) < CHASSIS_SPEED_DEADZONE) {
            speed_dps = 0.0f;
        }
        wheel_speed_ref[i] = ClampFloat(speed_dps, -WHEEL_SPEED_LIMIT_DPS, WHEEL_SPEED_LIMIT_DPS);
    }

    if (motor_fr)
        DJIMotorSetRef(motor_fr, wheel_speed_ref[0]);
    if (motor_fl)
        DJIMotorSetRef(motor_fl, wheel_speed_ref[1]);
    if (motor_br)
        DJIMotorSetRef(motor_br, wheel_speed_ref[2]);
    if (motor_bl)
        DJIMotorSetRef(motor_bl, wheel_speed_ref[3]);
}

static bool BuildChassisCommandFromEt08(ChassisCmd_t *cmd)
{
    if (cmd == NULL || et08_ctrl == NULL) {
        return false;
    }

    if (!ET08_IsOnline() || et08_ctrl->failsafe || et08_ctrl->frame_lost) {
        return false;
    }

    const int16_t left_x = ApplyDeadzone(et08_ctrl->left.x, RC_INPUT_DEADZONE);
    const int16_t left_y = ApplyDeadzone(et08_ctrl->left.y, RC_INPUT_DEADZONE);
    const int16_t right_x = ApplyDeadzone(et08_ctrl->right.x, RC_INPUT_DEADZONE);

    cmd->vx = ClampFloat((float)left_x / RC_STICK_SCALE * CHASSIS_MAX_VX, -CHASSIS_MAX_VX, CHASSIS_MAX_VX);
    cmd->vy = ClampFloat(-(float)left_y / RC_STICK_SCALE * CHASSIS_MAX_VY, -CHASSIS_MAX_VY, CHASSIS_MAX_VY);
    cmd->wz = ClampFloat((float)right_x / RC_STICK_SCALE * CHASSIS_MAX_WZ, -CHASSIS_MAX_WZ, CHASSIS_MAX_WZ);

    return true;
}

static void SendTelemetry(void)
{
    static uint32_t seq = 0U;
    seq++;

    const int16_t lx = (et08_ctrl != NULL) ? et08_ctrl->left.x : 0;
    const int16_t ly = (et08_ctrl != NULL) ? et08_ctrl->left.y : 0;
    const int16_t rx = (et08_ctrl != NULL) ? et08_ctrl->right.x : 0;
    const int16_t ry = (et08_ctrl != NULL) ? et08_ctrl->right.y : 0;

    TelemetryPrintf("[chs_et08] #%lu online=%u fs=%u lost=%u raw(lx=%d ly=%d rx=%d ry=%d)\\r\\n",
                    (unsigned long)seq,
                    (unsigned int)ET08_IsOnline(),
                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->failsafe : 0U),
                    (unsigned int)((et08_ctrl != NULL) ? et08_ctrl->frame_lost : 0U),
                    lx, ly, rx, ry);

    TelemetryPrintf("[chs_et08] wheel_ref(fr=%.1f fl=%.1f br=%.1f bl=%.1f) fdb(fr=%.1f)\\r\\n",
                    wheel_speed_ref[0], wheel_speed_ref[1], wheel_speed_ref[2], wheel_speed_ref[3],
                    (motor_fr != NULL) ? motor_fr->measure.speed_aps : 0.0f);
}

/* Main ----------------------------------------------------------------------*/
int main(void)
{
    HAL_Init();
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

    Debug_DisableWatchdogs();
    DWT_Init(168);
    BSPLogInit();
    InitTelemetryUsart();

    et08_ctrl = ET08_Init(&RC_UART);
    ChassisMotorsInit();

    HAL_Delay(MOTOR_STABILIZE_TIME_MS);
    TelemetrySendString("[chs_et08] control loop start\\r\\n");

    uint32_t last_control_tick = DWT_GetTimeline_ms();
    uint32_t last_telemetry_tick = last_control_tick;

    while (1) {
        DaemonTask();
        DJIMotorControl();

        uint32_t now = DWT_GetTimeline_ms();

        if ((now - last_control_tick) >= MAIN_LOOP_PERIOD_MS) {
            last_control_tick = now;

            ChassisCmd_t cmd = {0};
            bool online = BuildChassisCommandFromEt08(&cmd);

            if (!online) {
                ChassisStop();
                if (last_online_state != 0U) {
                    TelemetrySendString("[chs_et08] remote offline/failsafe -> stop\\r\\n");
                    last_online_state = 0U;
                }
            } else {
                ChassisApplyCommand(&cmd);
                if (last_online_state == 0U) {
                    TelemetrySendString("[chs_et08] remote online -> run\\r\\n");
                    last_online_state = 1U;
                }
            }
        }

        if ((now - last_telemetry_tick) >= TELEMETRY_PERIOD_MS) {
            last_telemetry_tick = now;
            SendTelemetry();
        }
    }
}

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

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        HAL_IncTick();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}
