#include "bsp_log.h"

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>


void BSPLogInit()
{
#if !BSP_LOG_USE_UART
    SEGGER_RTT_Init();
#endif
}

int PrintLog(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#if BSP_LOG_USE_UART
    char buffer[256];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (n > 0) {
        size_t len = (n < (int)sizeof(buffer)) ? (size_t)n : (sizeof(buffer) - 1U);
        HAL_UART_Transmit(&huart6, (uint8_t *)buffer, (uint16_t)len, 100);
    }
#else
    int n = SEGGER_RTT_vprintf(BUFFER_INDEX, fmt, &args); // 一次可以开启多个buffer(多个终端),我们只用一个
#endif
    va_end(args);
    return n;
}

void Float2Str(char *str, float va)
{
    int flag = va < 0;
    int head = (int)va;
    int point = (int)((va - head) * 1000);
    head = abs(head);
    point = abs(point);
    if (flag)
        sprintf(str, "-%d.%d", head, point);
    else
        sprintf(str, "%d.%d", head, point);
}

