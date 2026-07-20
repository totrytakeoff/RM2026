#include "infantry_tuning_telemetry.h"

#include <string.h>

#include "infantry_chassis.h"
#include "infantry_config.h"
#include "infantry_gimbal.h"
#include "usart.h"

enum {
    TUNING_CHANNEL_COUNT = 15U,
    TUNING_TAIL_SIZE = 4U,
    TUNING_FRAME_SIZE =
        (TUNING_CHANNEL_COUNT * (uint32_t)sizeof(float)) + TUNING_TAIL_SIZE,
};

static uint8_t tuning_tx_frame[TUNING_FRAME_SIZE]
    __attribute__((aligned(4)));
static bool tuning_initialized;
static uint32_t tuning_sent_count;
static uint32_t tuning_dropped_count;

/*
 * 这是正式固件自定义调参通道的唯一组帧入口。通道顺序同步记录在
 * docs/debug/infantry_tuning_telemetry.md，修改时必须同时更新文档。
 */
static void InfantryTuningTelemetry_FillChannels(
    uint32_t now_ms,
    float channels[TUNING_CHANNEL_COUNT])
{
    ChassisTuningSnapshot chassis = {0};
    GimbalYawTuningSnapshot yaw = {0};

    (void)Chassis_GetTuningSnapshot(&chassis);
    (void)Gimbal_GetYawTuningSnapshot(&yaw);

    channels[0] = (float)now_ms;
    channels[1] = (float)yaw.encoder_ecd;
    channels[2] = yaw.encoder_single_deg;
    channels[3] = yaw.target_total_deg;
    channels[4] = yaw.imu_total_deg;
    channels[5] = yaw.imu_single_deg;
    channels[6] = yaw.imu_gyro_z_rad_s;
    channels[7] = yaw.offset_raw_deg;
    channels[8] = yaw.offset_logic_deg;
    channels[9] = yaw.relative_speed_rad_s;
    channels[10] = chassis.follow_p_rad_s;
    channels[11] = chassis.follow_d_rad_s;
    channels[12] = chassis.follow_raw_wz_rad_s;
    channels[13] = chassis.follow_limited_wz_rad_s;
    channels[14] = chassis.command_wz_rad_s;
}

bool InfantryTuningTelemetry_Init(void)
{
#if INFANTRY_TUNING_TELEMETRY_ENABLE
    UART_HandleTypeDef *const uart = &INFANTRY_TUNING_UART_HANDLE;

    tuning_initialized = false;
    tuning_sent_count = 0U;
    tuning_dropped_count = 0U;
    memset(tuning_tx_frame, 0, sizeof(tuning_tx_frame));

    if (uart->Instance == NULL || uart->hdmatx == NULL) {
        return false;
    }

    uart->Init.BaudRate = INFANTRY_TUNING_UART_BAUDRATE;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = UART_STOPBITS_1;
    uart->Init.Parity = UART_PARITY_NONE;
    uart->Init.Mode = UART_MODE_TX_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(uart) != HAL_OK) {
        return false;
    }

    tuning_initialized = true;
    return true;
#else
    tuning_initialized = false;
    return true;
#endif
}

void InfantryTuningTelemetry_Publish(uint32_t now_ms)
{
#if INFANTRY_TUNING_TELEMETRY_ENABLE
    static const uint8_t just_float_tail[TUNING_TAIL_SIZE] = {
        0x00U, 0x00U, 0x80U, 0x7FU,
    };
    UART_HandleTypeDef *const uart = &INFANTRY_TUNING_UART_HANDLE;
    float channels[TUNING_CHANNEL_COUNT];

    if (!tuning_initialized) {
        return;
    }
    if (uart->gState != HAL_UART_STATE_READY) {
        tuning_dropped_count++;
        return;
    }

    InfantryTuningTelemetry_FillChannels(now_ms, channels);
    memcpy(tuning_tx_frame, channels, sizeof(channels));
    memcpy(tuning_tx_frame + sizeof(channels),
           just_float_tail,
           sizeof(just_float_tail));

    if (HAL_UART_Transmit_DMA(uart,
                              tuning_tx_frame,
                              (uint16_t)sizeof(tuning_tx_frame)) == HAL_OK) {
        tuning_sent_count++;
    } else {
        tuning_dropped_count++;
    }
#else
    (void)now_ms;
#endif
}

uint32_t InfantryTuningTelemetry_GetSentCount(void)
{
    return tuning_sent_count;
}

uint32_t InfantryTuningTelemetry_GetDroppedCount(void)
{
    return tuning_dropped_count;
}
