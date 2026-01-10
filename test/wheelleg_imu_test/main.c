/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Wheel-leg IMU test (DM-IMU CAN2 request mode)
 ******************************************************************************
 */
/* USER CODE END Header */

#include "main.h"

#include "can.h"
#include "dma.h"
#include "gpio.h"
#include "tim.h"
#include "usart.h"

#include <string.h>

#include "bsp_init.h"
#include "bsp_usart.h"
#include "dm_imu.h"
#include "utils.h"

#define IMU_REQUEST_INTERVAL_MS 20U
#define IMU_PRINT_INTERVAL_MS 50U
#define IMU_HEARTBEAT_INTERVAL_MS 1000U

#define IMU_ID 0x01
#define IMU_MASTER_ID 0x00
#define IMU_RX_ID ((IMU_MASTER_ID << 4) | (IMU_ID & 0x0F))

static USARTInstance *usart6_ = NULL;
static dm_imu_t dm_imu;

void SystemClock_Config(void);
void Error_Handler(void);

static void Usart6Init(void)
{
    USART_Init_Config_s config = {
        .module_callback = NULL,
        .recv_buff_size = 1,
        .usart_handle = &huart6,
    };
    usart6_ = USARTRegister(&config);
}

static void TelemetrySend(const char *msg)
{
    if (!msg)
        return;

    uint16_t len = (uint16_t)strlen(msg);
    if (usart6_)
    {
        USARTSend(usart6_, (uint8_t *)msg, len, USART_TRANSFER_BLOCKING);
    }
    else
    {
        HAL_UART_Transmit(&huart6, (uint8_t *)msg, len, 1000);
    }
}

static void ImuInit(void)
{
    dm_imu_can_config_t can_cfg = {
        .can_handle = &hcan2,
        .can_id = IMU_ID,
        .mst_id = IMU_RX_ID,
    };
    dm_imu_init_can(&dm_imu, &can_cfg);
    dm_imu_can_set_active(&dm_imu, false);
}

static void ImuRequestOnce(void)
{
    dm_imu_can_request_accel(&dm_imu);
    dm_imu_can_request_gyro(&dm_imu);
    dm_imu_can_request_euler(&dm_imu);
    dm_imu_can_request_quat(&dm_imu);
}

static void ImuPrintIfReady(void)
{
    dm_imu_data_t data;
    if (!dm_imu_get_data(&dm_imu, &data))
        return;

    char buffer[180];
    safe_snprintf(buffer, sizeof(buffer),
                  "IMU A[%.3f %.3f %.3f] G[%.3f %.3f %.3f] E[%.2f %.2f %.2f] Q[%.3f %.3f %.3f %.3f] mask=0x%02X\r\n",
                  data.accel[0], data.accel[1], data.accel[2],
                  data.gyro[0], data.gyro[1], data.gyro[2],
                  data.euler[0], data.euler[1], data.euler[2],
                  data.quat[0], data.quat[1], data.quat[2], data.quat[3],
                  (unsigned)data.valid_mask);
    TelemetrySend(buffer);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CAN2_Init();
    MX_USART6_UART_Init();

    BSPInit();

    Usart6Init();
    ImuInit();

    TelemetrySend("wheelleg_imu: start\r\n");

    uint32_t last_request_tick = 0;
    uint32_t last_print_tick = 0;
    uint32_t last_heartbeat_tick = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();
        if (now - last_request_tick >= IMU_REQUEST_INTERVAL_MS)
        {
            last_request_tick = now;
            ImuRequestOnce();
        }

        if (now - last_print_tick >= IMU_PRINT_INTERVAL_MS)
        {
            last_print_tick = now;
            ImuPrintIfReady();
        }

        if (now - last_heartbeat_tick >= IMU_HEARTBEAT_INTERVAL_MS)
        {
            last_heartbeat_tick = now;
            uint8_t rid = 0;
            uint8_t ack = 0;
            char buffer[96];
            dm_imu_get_last_ack(&dm_imu, &rid, &ack);
            safe_snprintf(buffer, sizeof(buffer),
                          "wheelleg_imu: waiting rx=0x%02X ack[rid=0x%02X code=0x%02X] alive=%u\r\n",
                          (unsigned)IMU_RX_ID, (unsigned)rid, (unsigned)ack,
                          (unsigned)dm_imu_is_alive(&dm_imu, 500));
            TelemetrySend(buffer);
        }

        HAL_Delay(2);
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

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14)
    {
        HAL_IncTick();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
