#include "vt_remote.h"

#include "bsp_log.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "memory.h"
#include "string.h"

#define VT_FRAME_HEADER_0 0xA9u
#define VT_FRAME_HEADER_1 0x53u

static VT_Ctrl_t vt_ctrl;
static uint8_t vt_init_flag = 0u;

static USARTInstance *vt_usart_instance;
static DaemonInstance *vt_daemon;

static uint16_t VT_Crc16CcittFalse(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    uint16_t i;
    for (i = 0; i < len; ++i)
    {
        uint8_t bit;
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0; bit < 8u; ++bit)
        {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc <<= 1;
        }
    }
    return crc;
}

static uint8_t VT_VerifyFrame(const uint8_t *buf)
{
    if (buf[0] != VT_FRAME_HEADER_0 || buf[1] != VT_FRAME_HEADER_1)
        return 0u;

    {
        uint16_t calc = VT_Crc16CcittFalse(buf, VT_FRAME_SIZE - 2u);
        uint16_t rx_le = (uint16_t)buf[VT_FRAME_SIZE - 2u] | ((uint16_t)buf[VT_FRAME_SIZE - 1u] << 8);
        uint16_t rx_be = (uint16_t)buf[VT_FRAME_SIZE - 1u] | ((uint16_t)buf[VT_FRAME_SIZE - 2u] << 8);
        if (calc != rx_le && calc != rx_be)
            return 0u;
    }

    return 1u;
}

static uint16_t VT_GetBitsU16(const uint8_t *buf, uint16_t start_bit, uint8_t bit_len)
{
    uint16_t out = 0u;
    uint8_t i;
    for (i = 0u; i < bit_len; ++i)
    {
        uint16_t bit_index = (uint16_t)(start_bit + i);
        uint16_t byte_index = bit_index >> 3;
        uint8_t bit_in_byte = (uint8_t)(bit_index & 0x07u);
        uint8_t bit = (uint8_t)((buf[byte_index] >> bit_in_byte) & 0x01u);
        out |= (uint16_t)bit << i;
    }
    return out;
}

static int16_t VT_ChannelCenter(uint16_t raw)
{
    return (int16_t)raw - VT_CH_CENTER;
}

static uint8_t VT_BoolFrom2Bits(uint16_t value)
{
    return (value == 1u) ? 1u : 0u;
}

static void VT_FillCtrl(const uint8_t *buf, VT_Ctrl_t *ctrl)
{
    uint16_t ch0 = VT_GetBitsU16(buf, 16u, 11u);
    uint16_t ch1 = VT_GetBitsU16(buf, 27u, 11u);
    uint16_t ch2 = VT_GetBitsU16(buf, 38u, 11u);
    uint16_t ch3 = VT_GetBitsU16(buf, 49u, 11u);
    uint16_t dial = VT_GetBitsU16(buf, 65u, 11u);

    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->ch0_right_x.raw = ch0;
    ctrl->ch1_right_y.raw = ch1;
    ctrl->ch2_left_y.raw = ch2;
    ctrl->ch3_left_x.raw = ch3;
    ctrl->dial.raw = dial;

    ctrl->ch0_right_x.centered = VT_ChannelCenter(ch0);
    ctrl->ch1_right_y.centered = VT_ChannelCenter(ch1);
    ctrl->ch2_left_y.centered = VT_ChannelCenter(ch2);
    ctrl->ch3_left_x.centered = VT_ChannelCenter(ch3);
    ctrl->dial.centered = VT_ChannelCenter(dial);

    ctrl->gear = (uint8_t)VT_GetBitsU16(buf, 60u, 2u);
    ctrl->pause_pressed = (uint8_t)VT_GetBitsU16(buf, 62u, 1u);
    ctrl->custom_left_pressed = (uint8_t)VT_GetBitsU16(buf, 63u, 1u);
    ctrl->custom_right_pressed = (uint8_t)VT_GetBitsU16(buf, 64u, 1u);
    ctrl->trigger_pressed = (uint8_t)VT_GetBitsU16(buf, 76u, 1u);

    ctrl->mouse_x = (int16_t)VT_GetBitsU16(buf, 80u, 16u);
    ctrl->mouse_y = (int16_t)VT_GetBitsU16(buf, 96u, 16u);
    ctrl->mouse_z = (int16_t)VT_GetBitsU16(buf, 112u, 16u);
    ctrl->mouse_left_pressed = VT_BoolFrom2Bits(VT_GetBitsU16(buf, 128u, 2u));
    ctrl->mouse_right_pressed = VT_BoolFrom2Bits(VT_GetBitsU16(buf, 130u, 2u));
    ctrl->mouse_middle_pressed = VT_BoolFrom2Bits(VT_GetBitsU16(buf, 132u, 2u));
    ctrl->keyboard_value = VT_GetBitsU16(buf, 136u, 16u);
}

static void VT_RxCallback(void)
{
    if (vt_usart_instance == NULL)
        return;

    {
        const uint8_t *buf = vt_usart_instance->recv_buff;
        if (!VT_VerifyFrame(buf))
        {
            vt_ctrl.bad_count++;
            vt_ctrl.crc_ok = 0u;
            return;
        }

        VT_Ctrl_t next = vt_ctrl;
        VT_FillCtrl(buf, &next);
        next.crc_ok = 1u;
        next.frame_count = vt_ctrl.frame_count + 1u;
        next.bad_count = vt_ctrl.bad_count;
        memcpy(&vt_ctrl, &next, sizeof(next));
    }

    DaemonReload(vt_daemon);
}

static void VT_LostCallback(void *id)
{
    (void)id;
    memset(&vt_ctrl, 0, sizeof(vt_ctrl));
    LOGWARNING("[vt] datalink remote control lost");

    if (vt_usart_instance != NULL)
        USARTServiceInit(vt_usart_instance);
}

static void VT_ReinitUart(UART_HandleTypeDef *uart_handle)
{
    if (uart_handle == NULL)
        return;

    (void)HAL_UART_DeInit(uart_handle);
    uart_handle->Init.BaudRate = 921600;
    uart_handle->Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle->Init.StopBits = UART_STOPBITS_1;
    uart_handle->Init.Parity = UART_PARITY_NONE;
    uart_handle->Init.Mode = UART_MODE_TX_RX;
    uart_handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle->Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(uart_handle) != HAL_OK)
        LOGWARNING("[vt] uart reinit failed");
}

VT_Ctrl_t *VT_Init(UART_HandleTypeDef *uart_handle)
{
    memset(&vt_ctrl, 0, sizeof(vt_ctrl));
    VT_ReinitUart(uart_handle);

    {
        USART_Init_Config_s conf;
        conf.module_callback = VT_RxCallback;
        conf.usart_handle = uart_handle;
        conf.recv_buff_size = VT_FRAME_SIZE;
        vt_usart_instance = USARTRegister(&conf);
    }

    {
        Daemon_Init_Config_s daemon_conf = {
            .reload_count = 20, // 200ms timeout, VT frame interval is around 14ms
            .callback = VT_LostCallback,
            .owner_id = NULL,
        };
        vt_daemon = DaemonRegister(&daemon_conf);
    }

    vt_init_flag = 1u;
    return &vt_ctrl;
}

uint8_t VT_IsOnline(void)
{
    if (!vt_init_flag)
        return 0u;
    return DaemonIsOnline(vt_daemon);
}

VT_Ctrl_t *VT_GetCtrl(void)
{
    return &vt_ctrl;
}
