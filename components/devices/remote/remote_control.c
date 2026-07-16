#include "remote_control.h"
#include "string.h"
#include "bsp_usart.h"
#include "memory.h"
#include "stdlib.h"
#include "daemon.h"
#include "bsp_log.h"

#define REMOTE_CONTROL_FRAME_SIZE 18u // 遥控器接收的buffer大小

// 遥控器数据(输出),始终是当前生效的数据
static RC_ctrl_t rc_ctrl_out[2]; //[0]:当前数据TEMP,[1]:上一次的数据LAST.用于按键持续按下和切换的判断

// DT7/DR16 数据
static RC_ctrl_t rc_ctrl_dt7[2];
static uint8_t dt7_init_flag = 0;

// 虚拟DBUS 数据
static RC_ctrl_t rc_ctrl_virtual[2];
static uint8_t virtual_init_flag = 0;

// 串口实例
static USARTInstance *rc_usart_instance_dt7;
static USARTInstance *rc_usart_instance_virtual;
static DaemonInstance *rc_daemon_dt7;
static DaemonInstance *rc_daemon_virtual;

static RC_Source_t active_source = RC_SOURCE_NONE;
static uint8_t virtual_enabled = 0;

/**
 * @brief 矫正遥控器摇杆的值,超过660或者小于-660的值都认为是无效值,置0
 *
 */
static void RectifyRCjoystick(RC_ctrl_t *ctrl)
{
    for (uint8_t i = 0; i < 5; ++i)
        if (abs(*(&ctrl[TEMP].rc.rocker_l_ + i)) > 660)
            *(&ctrl[TEMP].rc.rocker_l_ + i) = 0;
}

/**
 * @brief 遥控器数据解析
 *
 * @param sbus_buf 接收buffer
 */
static void sbus_to_rc(const uint8_t *sbus_buf, RC_ctrl_t *ctrl)
{
    // 摇杆,直接解算时减去偏置
    ctrl[TEMP].rc.rocker_r_ = ((sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff) - RC_CH_VALUE_OFFSET;                              //!< Channel 0
    ctrl[TEMP].rc.rocker_r1 = (((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff) - RC_CH_VALUE_OFFSET;                       //!< Channel 1
    ctrl[TEMP].rc.rocker_l_ = (((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | (sbus_buf[4] << 10)) & 0x07ff) - RC_CH_VALUE_OFFSET; //!< Channel 2
    ctrl[TEMP].rc.rocker_l1 = (((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff) - RC_CH_VALUE_OFFSET;                       //!< Channel 3
    ctrl[TEMP].rc.dial = ((sbus_buf[16] | (sbus_buf[17] << 8)) & 0x07FF) - RC_CH_VALUE_OFFSET;                                 // 左侧拨轮
    RectifyRCjoystick(ctrl);
    // 开关,0左1右
    ctrl[TEMP].rc.switch_right = ((sbus_buf[5] >> 4) & 0x0003);     //!< Switch right
    ctrl[TEMP].rc.switch_left = ((sbus_buf[5] >> 4) & 0x000C) >> 2; //!< Switch left

    // 鼠标解析
    ctrl[TEMP].mouse.x = (sbus_buf[6] | (sbus_buf[7] << 8)); //!< Mouse X axis
    ctrl[TEMP].mouse.y = (sbus_buf[8] | (sbus_buf[9] << 8)); //!< Mouse Y axis
    ctrl[TEMP].mouse.press_l = sbus_buf[12];                 //!< Mouse Left Is Press ?
    ctrl[TEMP].mouse.press_r = sbus_buf[13];                 //!< Mouse Right Is Press ?

    //  位域的按键值解算,直接memcpy即可,注意小端低字节在前,即lsb在第一位,msb在最后
    *(uint16_t *)&ctrl[TEMP].key[KEY_PRESS] = (uint16_t)(sbus_buf[14] | (sbus_buf[15] << 8));
    if (ctrl[TEMP].key[KEY_PRESS].ctrl) // ctrl键按下
        ctrl[TEMP].key[KEY_PRESS_WITH_CTRL] = ctrl[TEMP].key[KEY_PRESS];
    else
        memset(&ctrl[TEMP].key[KEY_PRESS_WITH_CTRL], 0, sizeof(Key_t));
    if (ctrl[TEMP].key[KEY_PRESS].shift) // shift键按下
        ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT] = ctrl[TEMP].key[KEY_PRESS];
    else
        memset(&ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT], 0, sizeof(Key_t));

    uint16_t key_now = ctrl[TEMP].key[KEY_PRESS].keys,                   // 当前按键是否按下
        key_last = ctrl[LAST].key[KEY_PRESS].keys,                       // 上一次按键是否按下
        key_with_ctrl = ctrl[TEMP].key[KEY_PRESS_WITH_CTRL].keys,        // 当前ctrl组合键是否按下
        key_with_shift = ctrl[TEMP].key[KEY_PRESS_WITH_SHIFT].keys,      //  当前shift组合键是否按下
        key_last_with_ctrl = ctrl[LAST].key[KEY_PRESS_WITH_CTRL].keys,   // 上一次ctrl组合键是否按下
        key_last_with_shift = ctrl[LAST].key[KEY_PRESS_WITH_SHIFT].keys; // 上一次shift组合键是否按下

    for (uint16_t i = 0, j = 0x1; i < 16; j <<= 1, i++)
    {
        if (i == 4 || i == 5) // 4,5位为ctrl和shift,直接跳过
            continue;
        // 如果当前按键按下,上一次按键没有按下,且ctrl和shift组合键没有按下,则按键按下计数加1(检测到上升沿)
        if ((key_now & j) && !(key_last & j) && !(key_with_ctrl & j) && !(key_with_shift & j))
            ctrl[TEMP].key_count[KEY_PRESS][i]++;
        // 当前ctrl组合键按下,上一次ctrl组合键没有按下,则ctrl组合键按下计数加1(检测到上升沿)
        if ((key_with_ctrl & j) && !(key_last_with_ctrl & j))
            ctrl[TEMP].key_count[KEY_PRESS_WITH_CTRL][i]++;
        // 当前shift组合键按下,上一次shift组合键没有按下,则shift组合键按下计数加1(检测到上升沿)
        if ((key_with_shift & j) && !(key_last_with_shift & j))
            ctrl[TEMP].key_count[KEY_PRESS_WITH_SHIFT][i]++;
    }

    memcpy(&ctrl[LAST], &ctrl[TEMP], sizeof(RC_ctrl_t)); // 保存上一次的数据,用于按键持续按下和切换的判断
}

static uint8_t Dt7IsOnlineInternal()
{
    if (dt7_init_flag)
        return DaemonIsOnline(rc_daemon_dt7);
    return 0;
}

static uint8_t VirtualIsOnlineInternal()
{
    if (virtual_init_flag)
        return DaemonIsOnline(rc_daemon_virtual);
    return 0;
}

static void UpdateOutputFromDt7()
{
    memcpy(rc_ctrl_out, rc_ctrl_dt7, sizeof(rc_ctrl_dt7));
    active_source = RC_SOURCE_DT7;
}

static void UpdateOutputFromVirtual()
{
    memcpy(rc_ctrl_out, rc_ctrl_virtual, sizeof(rc_ctrl_virtual));
    active_source = RC_SOURCE_VIRTUAL;
}

static void ReinitUartForDbus(UART_HandleTypeDef *uart_handle)
{
    if (uart_handle == NULL)
        return;

    (void)HAL_UART_DeInit(uart_handle);
    uart_handle->Init.BaudRate = 100000;
    uart_handle->Init.WordLength = UART_WORDLENGTH_9B;
    uart_handle->Init.StopBits = UART_STOPBITS_1;
    uart_handle->Init.Parity = UART_PARITY_EVEN;
    uart_handle->Init.Mode = UART_MODE_TX_RX;
    uart_handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle->Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(uart_handle) != HAL_OK)
    {
        LOGWARNING("[rc] dbus uart reinit failed");
    }
}

/**
 * @brief 对sbus_to_rc的简单封装,用于注册到bsp_usart的回调函数中
 *
 */
static void RemoteControlRxCallback()
{
    if (rc_usart_instance_dt7 == NULL ||
        rc_usart_instance_dt7->recv_len != REMOTE_CONTROL_FRAME_SIZE)
        return;

    DaemonReload(rc_daemon_dt7);                                    // 先喂狗
    sbus_to_rc(rc_usart_instance_dt7->recv_buff, rc_ctrl_dt7);      // 进行协议解析
    UpdateOutputFromDt7();                                          // DT7优先级最高
}

/**
 * @brief 遥控器离线的回调函数,注册到守护进程中,串口掉线时调用
 *
 */
static void RCLostCallback(void *id)
{
    if (VirtualIsOnlineInternal())
    {
        UpdateOutputFromVirtual();
    }
    else
    {
        memset(rc_ctrl_out, 0, sizeof(rc_ctrl_out)); // 清空输出数据
        active_source = RC_SOURCE_NONE;
    }

    if (rc_usart_instance_dt7)
        USARTServiceInit(rc_usart_instance_dt7); // 尝试重新启动接收
    LOGWARNING("[rc] dt7 remote control lost");
}

RC_ctrl_t *RemoteControlInit(UART_HandleTypeDef *rc_usart_handle)
{
    memset(rc_ctrl_dt7, 0, sizeof(rc_ctrl_dt7));
    if (!virtual_init_flag)
        memset(rc_ctrl_out, 0, sizeof(rc_ctrl_out));

    USART_Init_Config_s conf;
    conf.module_callback = RemoteControlRxCallback;
    conf.usart_handle = rc_usart_handle;
    conf.recv_buff_size = REMOTE_CONTROL_FRAME_SIZE;
    rc_usart_instance_dt7 = USARTRegister(&conf);

    // 进行守护进程的注册,用于定时检查遥控器是否正常工作
    DaemonConfig daemon_conf = {
        .timeout_ms = 100U, // 100ms未收到数据视为离线,遥控器的接收频率实际上是1000/14Hz(大约70Hz)
        .callback = RCLostCallback,
        .owner = NULL, // 只有1个遥控器,不需要owner_id
    };
    rc_daemon_dt7 = DaemonRegister(&daemon_conf);

    dt7_init_flag = 1;
    if (active_source == RC_SOURCE_NONE)
        active_source = RC_SOURCE_DT7;
    return rc_ctrl_out;
}

static void VirtualRxCallback()
{
    if (!virtual_enabled || rc_usart_instance_virtual == NULL ||
        rc_usart_instance_virtual->recv_len != REMOTE_CONTROL_FRAME_SIZE)
        return;

    DaemonReload(rc_daemon_virtual);
    sbus_to_rc(rc_usart_instance_virtual->recv_buff, rc_ctrl_virtual);

    if (!Dt7IsOnlineInternal())
        UpdateOutputFromVirtual();
}

static void VirtualLostCallback(void *id)
{
    if (!virtual_enabled)
        return;

    if (!Dt7IsOnlineInternal())
    {
        memset(rc_ctrl_out, 0, sizeof(rc_ctrl_out));
        active_source = RC_SOURCE_NONE;
    }

    if (rc_usart_instance_virtual)
        USARTServiceInit(rc_usart_instance_virtual);
    LOGWARNING("[rc] virtual dbus lost");
}

RC_ctrl_t *RemoteControlInitVirtual(UART_HandleTypeDef *virtual_usart_handle)
{
    memset(rc_ctrl_virtual, 0, sizeof(rc_ctrl_virtual));
    if (!dt7_init_flag)
        memset(rc_ctrl_out, 0, sizeof(rc_ctrl_out));

    ReinitUartForDbus(virtual_usart_handle);

    USART_Init_Config_s conf;
    conf.module_callback = VirtualRxCallback;
    conf.usart_handle = virtual_usart_handle;
    conf.recv_buff_size = REMOTE_CONTROL_FRAME_SIZE;
    rc_usart_instance_virtual = USARTRegister(&conf);

    DaemonConfig daemon_conf = {
        .timeout_ms = 100U,
        .callback = VirtualLostCallback,
        .owner = NULL,
    };
    rc_daemon_virtual = DaemonRegister(&daemon_conf);

    virtual_init_flag = 1;
    virtual_enabled = 1;
    if (active_source == RC_SOURCE_NONE)
        active_source = RC_SOURCE_VIRTUAL;
    return rc_ctrl_out;
}

uint8_t RemoteControlIsOnline()
{
    if (Dt7IsOnlineInternal())
        return 1;
    if (VirtualIsOnlineInternal())
        return 1;
    return 0;
}

uint8_t RemoteControlIsDt7Online()
{
    return Dt7IsOnlineInternal();
}

uint8_t RemoteControlIsVirtualOnline()
{
    return VirtualIsOnlineInternal();
}

RC_Source_t RemoteControlGetActiveSource()
{
    return active_source;
}

void RemoteControlSetVirtualEnabled(uint8_t enabled)
{
    virtual_enabled = enabled ? 1 : 0;

    if (virtual_enabled && virtual_init_flag && rc_usart_instance_virtual)
    {
        ReinitUartForDbus(rc_usart_instance_virtual->usart_handle);
        USARTServiceInit(rc_usart_instance_virtual);
        return;
    }

    if (!virtual_enabled && active_source == RC_SOURCE_VIRTUAL)
    {
        if (Dt7IsOnlineInternal())
            UpdateOutputFromDt7();
        else
        {
            memset(rc_ctrl_out, 0, sizeof(rc_ctrl_out));
            active_source = RC_SOURCE_NONE;
        }
    }
}

uint8_t RemoteControlIsVirtualEnabled()
{
    return virtual_enabled;
}
