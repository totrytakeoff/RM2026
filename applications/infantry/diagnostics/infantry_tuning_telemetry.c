#include "infantry_tuning_telemetry.h"

#include <string.h>

#include "infantry_chassis.h"
#include "infantry_config.h"
#include "infantry_gimbal.h"
#include "usart.h"

enum {
    TUNING_CHANNEL_COUNT = 31U,
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
    GimbalTuningSnapshot gimbal = {0};
    const GimbalAxisTuningSnapshot *axis;

    (void)Chassis_GetTuningSnapshot(&chassis);
    (void)Gimbal_GetTuningSnapshot(&gimbal);

#if INFANTRY_TUNING_GIMBAL_AXIS == INFANTRY_TUNING_GIMBAL_AXIS_PITCH
    axis = &gimbal.pitch;
#else
    axis = &gimbal.yaw;
#endif

    channels[0] = (float)now_ms;
    channels[1] = (float)axis->control_mode;
    channels[2] = axis->operator_speed_command_deg_s;
    channels[3] = axis->hold_target_deg;
    channels[4] = axis->imu_angle_deg;
    channels[5] = axis->angle_error_deg;
    channels[6] = axis->imu_gyro_deg_s;
    channels[7] = axis->angle_p_deg_s;
    channels[8] = axis->angle_i_deg_s;
    channels[9] = axis->angle_d_deg_s;
    channels[10] = axis->angle_output_deg_s;
    channels[11] = axis->angle_output_limit_ratio;
    channels[12] = axis->speed_reference_deg_s;
    channels[13] = axis->speed_feedback_deg_s;
    channels[14] = axis->speed_error_deg_s;
    channels[15] = axis->speed_p_current;
    channels[16] = axis->speed_i_current;
    channels[17] = axis->speed_d_current;
    channels[18] = axis->speed_output_current;
    channels[19] = axis->speed_output_limit_ratio;
    channels[20] = (float)axis->motor_current_feedback;
    channels[21] = (float)axis->encoder_ecd;
    channels[22] = axis->current_feedforward;
    channels[23] = gimbal.yaw_offset_logic_deg;
    channels[24] = chassis.command_wz_rad_s;
    channels[25] = gimbal.yaw_base_rate_estimate_rad_s;
    channels[26] = chassis.input_x_intent;
    channels[27] = chassis.input_y_intent;
    channels[28] = chassis.command_vx_m_s;
    channels[29] = chassis.command_vy_m_s;
    channels[30] = chassis.spin_translation_scale;
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
