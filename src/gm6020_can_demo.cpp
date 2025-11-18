//
// GM6020 CAN 驱动示例 (基于 CanBus 封装)
// 说明:
// - 本示例演示如何通过 0x1FF 组播帧向 1..4 号电机(0x205..0x208)发送电流指令
// - 仅使用基础 C++ 特性，无 STL/动态分配，适用于嵌入式
// - 提供 C 接口函数，便于从 C 的 main.c 调用
//

extern "C" {
#include "stm32f4xx_hal.h"
#include "can.h"            // 提供 hcan1 / hcan2
}

#include "can_comm.hpp"     // 通用 CAN 封装类

// 选择要使用的 CAN 口: 你可以改为 &hcan2
// 注意: 你当前工程中 GIMBAL_CAN 宏已切到 hcan1
static CanBus g_can(&hcan1);

// 将 4 路 int16_t 电流值打包为 8 字节负载
static inline void pack_currents(int16_t i1, int16_t i2, int16_t i3, int16_t i4, uint8_t out[8])
{
    out[0] = (uint8_t)(i1 >> 8); out[1] = (uint8_t)(i1);
    out[2] = (uint8_t)(i2 >> 8); out[3] = (uint8_t)(i2);
    out[4] = (uint8_t)(i3 >> 8); out[5] = (uint8_t)(i3);
    out[6] = (uint8_t)(i4 >> 8); out[7] = (uint8_t)(i4);
}

// 可选: 接收回调示例（解析 0x205..0x208 反馈）
static void on_can_rx(const CAN_RxHeaderTypeDef* h, const uint8_t* d, void* user)
{
    (void)user;
    // 这里可以根据 h->StdId 判断是否为 6020 的反馈帧，然后解析角度/速度/电流/温度
    // 本示例不做实际处理，用户可根据需要自行扩展
}

extern "C" void GM6020_Demo_Init(void)
{
    // 绑定接收回调（可选）
    g_can.attachRxCallback(&on_can_rx, 0);
}

// 在主循环中周期调用该函数（例如每 5ms）
// 示例: 让 1/4 号电机转动，2/3 号关闭
extern "C" void GM6020_Demo_Loop(void)
{
    // 发送前尽可能拉取接收，降低延迟
    while (g_can.pollOnce()) {}

    uint8_t payload[8];
    pack_currents(10000, 0, 0, 10000, payload);
    // 0x1FF: 云台/发射总线的组播控制帧
    g_can.sendStd(0x1FF, payload, 8);
}
