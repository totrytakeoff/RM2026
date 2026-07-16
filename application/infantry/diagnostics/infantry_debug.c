/**
 * @file minimal_debug.c
 * @brief 最小框架统一调试输出层实现
 */

#include "infantry_debug.h"

#include <stdarg.h>
#include <string.h>

#include "usart.h"
#include "infantry_types.h"
#include "infantry_input.h"
#include "infantry_chassis.h"
#include "infantry_gimbal.h"
#include "infantry_shoot.h"
#include "rm_time.h"
#include "utils.h"

static uint32_t last_text_tick = 0U;
static uint32_t last_vofa_tick = 0U;
static uint32_t vofa_tx_fail_cnt = 0U;
static uint32_t text_tx_fail_cnt = 0U;

static uint32_t MinimalDebug_GetTxTimeoutMs(uint16_t len)
{
    uint32_t baud = (MINIMAL_DEBUG_UART_BAUDRATE > 0U) ? MINIMAL_DEBUG_UART_BAUDRATE : 115200U;
    /* UART 8N1约10bit/byte，留一点冗余 */
    uint32_t need_ms = ((uint32_t)len * 10U * 1000U + baud - 1U) / baud + 2U;
    if (need_ms < MINIMAL_DEBUG_UART_TIMEOUT_MS) {
        need_ms = MINIMAL_DEBUG_UART_TIMEOUT_MS;
    }
    return need_ms;
}

static void __attribute__((unused)) MinimalDebug_TxRaw(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0U) {
        return;
    }
    if (HAL_UART_Transmit(&MINIMAL_DEBUG_UART_HANDLE, (uint8_t *)buf, len, MinimalDebug_GetTxTimeoutMs(len)) != HAL_OK) {
        text_tx_fail_cnt++;
    }
}

static void MinimalDebug_LogEventByTag(const char *tag, const char *fmt, va_list args)
{
#if MINIMAL_DEBUG_TEXT_STREAM_ACTIVE
    char msg_buf[160];
    char line_buf[192];
    int msg_n;
    int line_n;
    size_t msg_len;

    msg_n = RmFormat_Vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    if (msg_n < 0) {
        return;
    }
    msg_len = strnlen(msg_buf, sizeof(msg_buf));
    while (msg_len > 0U && (msg_buf[msg_len - 1U] == '\r' || msg_buf[msg_len - 1U] == '\n')) {
        msg_buf[msg_len - 1U] = '\0';
        msg_len--;
    }
    line_n = RmFormat_Snprintf(line_buf,
                              sizeof(line_buf),
                              "[DBG][%s] %s\r\n",
                              tag,
                              msg_buf);
    if (line_n > 0) {
        uint16_t tx_len;
        if (line_n < (int)sizeof(line_buf)) {
            tx_len = (uint16_t)line_n;
        } else {
            /* 行被截断时强制补齐 CRLF，避免日志黏连 */
            size_t cap = sizeof(line_buf);
            line_buf[cap - 3U] = '\r';
            line_buf[cap - 2U] = '\n';
            line_buf[cap - 1U] = '\0';
            tx_len = (uint16_t)(cap - 1U);
        }
        MinimalDebug_TxRaw((const uint8_t *)line_buf, tx_len);
    }
#else
    (void)tag;
    (void)fmt;
    (void)args;
#endif
}

static void __attribute__((unused)) MinimalDebug_TextPeriodic(void)
{
#if MINIMAL_DEBUG_TEXT_STREAM_ACTIVE
#if MINIMAL_DEBUG_MOD_SYSTEM
    MinimalDebug_LogEventSystem("in=%u online=%u estop=%u init=%u",
                                (unsigned)g_robot.input.active_input,
                                (unsigned)g_robot.input.online,
                                (unsigned)g_robot.input.emergency_stop,
                                (unsigned)g_robot.initialized);
#endif

#if MINIMAL_DEBUG_MOD_INPUT
    MinimalDebug_LogEventInput(
        "src=%u online=%u vt_allowed=%u gear=%u estop=%u mode=%u kb=0x%04X mouse=(%d,%d,%d) ch=(%d,%d,%d,%d) cmd(vx=%ld vy=%ld wz=%ld) yaw=%ld pitch=%ld",
        (unsigned)g_robot.input.active_input,
        (unsigned)g_robot.input.online,
        (unsigned)Input_IsVTAllowed(),
        (unsigned)g_robot.input.gear,
        (unsigned)g_robot.input.emergency_stop,
        (unsigned)g_robot.input.gimbal_mode,
        (unsigned)g_robot.input.vt_raw.keyboard,
        (int)g_robot.input.vt_raw.mouse_x,
        (int)g_robot.input.vt_raw.mouse_y,
        (int)g_robot.input.vt_raw.mouse_z,
        (int)g_robot.input.vt_raw.ch0_c,
        (int)g_robot.input.vt_raw.ch1_c,
        (int)g_robot.input.vt_raw.ch2_c,
        (int)g_robot.input.vt_raw.ch3_c,
        (long)(g_robot.input.vx * 1000.0f),
        (long)(g_robot.input.vy * 1000.0f),
        (long)(g_robot.input.wz * 1000.0f),
        (long)(g_robot.input.yaw_speed * 10.0f),
        (long)(g_robot.input.pitch_speed * 10.0f));
#endif

#if MINIMAL_DEBUG_MOD_CHASSIS
    MinimalDebug_LogEventChassis("vx_mms=%ld vy_mms=%ld wz_mrads=%ld pwr_x1e3=%ld fr_ref=%ld fr_fdb=%ld",
                                 (long)(g_robot.chassis.vx * 1000.0f),
                                 (long)(g_robot.chassis.vy * 1000.0f),
                                 (long)(g_robot.chassis.wz * 1000.0f),
                                 (long)(Chassis_GetPowerScale() * 1000.0f),
                                 (long)(Chassis_GetFRSpeedRef()),
                                 (long)(Chassis_GetFRSpeedFdb()));
#endif

#if MINIMAL_DEBUG_MOD_GIMBAL
    MinimalDebug_LogEventGimbal("mode=%u pitch_loop=%u yaw_ref=%ld yaw_fdb=%ld yaw_off=%ld pitch_ref=%ld pitch_fdb=%ld pitch_tgt=%ld enc(y=%ld p=%ld) imu(y=%ld p=%ld)",
                                (unsigned)Gimbal_GetMode(),
                                (unsigned)Gimbal_GetPitchCtrlMode(),
                                (long)(Gimbal_GetYawSpeedRef()),
                                (long)(Gimbal_GetYawSpeedFdb()),
                                (long)(Gimbal_GetYawOffsetLogicDeg()),
                                (long)(Gimbal_GetPitchSpeedRef()),
                                (long)(Gimbal_GetPitchSpeedFdb()),
                                (long)(Gimbal_GetPitchTargetAngle()),
                                (long)(Gimbal_GetYawEncoderAngle()),
                                (long)(Gimbal_GetPitchEncoderAngle()),
                                (long)(Gimbal_GetYawIMUAngle()),
                                (long)(Gimbal_GetPitchIMUAngle()));
#endif

#if MINIMAL_DEBUG_MOD_SHOOT
    MinimalDebug_LogEventShoot("state=%u loader_ref=%ld loader_fdb=%ld pend=%u act=%u",
                               (unsigned)Shoot_GetState(),
                               (long)(Shoot_GetLoaderRef()),
                               (long)(Shoot_GetLoaderFeedback()),
                               (unsigned)Shoot_GetPendingShots(),
                               (unsigned)Shoot_IsSingleActive());
#endif
#endif
}

void MinimalDebug_Init(void)
{
#if MINIMAL_DEBUG_ENABLE
#if (MINIMAL_DEBUG_UART_PORT == 1U) || (MINIMAL_DEBUG_UART_PORT == 6U)
    MINIMAL_DEBUG_UART_HANDLE.Init.BaudRate = MINIMAL_DEBUG_UART_BAUDRATE;
    MINIMAL_DEBUG_UART_HANDLE.Init.WordLength = UART_WORDLENGTH_8B;
    MINIMAL_DEBUG_UART_HANDLE.Init.StopBits = UART_STOPBITS_1;
    MINIMAL_DEBUG_UART_HANDLE.Init.Parity = UART_PARITY_NONE;
    MINIMAL_DEBUG_UART_HANDLE.Init.Mode = UART_MODE_TX_RX;
    MINIMAL_DEBUG_UART_HANDLE.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    MINIMAL_DEBUG_UART_HANDLE.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&MINIMAL_DEBUG_UART_HANDLE);
#endif

#if MINIMAL_DEBUG_TEXT_STREAM_ACTIVE && MINIMAL_DEBUG_MOD_SYSTEM
    MinimalDebug_LogEventSystem("debug mode=0x%02x uart=%u baud=%lu",
                                MINIMAL_DEBUG_MODE,
                                MINIMAL_DEBUG_UART_PORT,
                                (unsigned long)MINIMAL_DEBUG_UART_BAUDRATE);
#endif
#endif
}

void MinimalDebug_UpdatePeriodic(uint32_t now_ms)
{
#if MINIMAL_DEBUG_ENABLE
#if MINIMAL_DEBUG_TEXT_STREAM_ACTIVE
    if ((now_ms - last_text_tick) >= MINIMAL_DEBUG_TEXT_PERIOD_MS) {
        last_text_tick = now_ms;
        MinimalDebug_TextPeriodic();
    }
#endif

#if MINIMAL_DEBUG_VOFA_STREAM_ACTIVE
    if ((now_ms - last_vofa_tick) >= MINIMAL_DEBUG_VOFA_PERIOD_MS) {
        last_vofa_tick = now_ms;
        MinimalDebug_PublishVofaFrame();
    }
#endif
#else
    (void)now_ms;
#endif
}

void MinimalDebug_LogEventSystem(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MinimalDebug_LogEventByTag("SYS", fmt, args);
    va_end(args);
}

void MinimalDebug_LogEventInput(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MinimalDebug_LogEventByTag("IN", fmt, args);
    va_end(args);
}

void MinimalDebug_LogEventChassis(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MinimalDebug_LogEventByTag("CHS", fmt, args);
    va_end(args);
}

void MinimalDebug_LogEventGimbal(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MinimalDebug_LogEventByTag("GMB", fmt, args);
    va_end(args);
}

void MinimalDebug_LogEventShoot(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    MinimalDebug_LogEventByTag("SHT", fmt, args);
    va_end(args);
}

void MinimalDebug_PublishVofaFrame(void)
{
#if MINIMAL_DEBUG_VOFA_STREAM_ACTIVE
    float ch[16];
    uint8_t txbuf[(sizeof(ch)) + 4U];
    static const uint8_t tail[4] = {0x00U, 0x00U, 0x80U, 0x7FU};
    HAL_StatusTypeDef ret;

    ch[0] = (float)RmTime_NowMs();
    ch[1] = (float)g_robot.input.active_input;
    ch[2] = (float)g_robot.input.online;
    ch[3] = g_robot.chassis.vx;
    ch[4] = g_robot.chassis.vy;
    ch[5] = g_robot.chassis.wz;
    ch[6] = Chassis_GetFRSpeedRef();
    ch[7] = Chassis_GetFRSpeedFdb();
    ch[8] = Gimbal_GetYawSpeedRef();
    ch[9] = Gimbal_GetYawSpeedFdb();
    ch[10] = Gimbal_GetPitchSpeedRef();
    ch[11] = Gimbal_GetPitchSpeedFdb();
    ch[12] = Gimbal_GetYawOffsetLogicDeg();
    ch[13] = Gimbal_GetPitchTargetAngle();
    ch[14] = Shoot_GetLoaderRef();
    ch[15] = Shoot_GetLoaderFeedback();

    memcpy(txbuf, ch, sizeof(ch));
    memcpy(txbuf + sizeof(ch), tail, sizeof(tail));

    ret = HAL_UART_Transmit(&MINIMAL_DEBUG_UART_HANDLE,
                            txbuf,
                            (uint16_t)sizeof(txbuf),
                            MinimalDebug_GetTxTimeoutMs((uint16_t)sizeof(txbuf)));
    if (ret != HAL_OK) {
        vofa_tx_fail_cnt++;
#if MINIMAL_DEBUG_TEXT_STREAM_ACTIVE && MINIMAL_DEBUG_MOD_SYSTEM
        if ((vofa_tx_fail_cnt % 50U) == 1U) {
            MinimalDebug_LogEventSystem("vofa_tx_fail=%lu text_tx_fail=%lu",
                                        (unsigned long)vofa_tx_fail_cnt,
                                        (unsigned long)text_tx_fail_cnt);
        }
#endif
    }
#endif
}
