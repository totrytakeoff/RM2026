//
//  文件: can_comm.hpp
//  说明: 面向嵌入式的通用 CAN 收发轻量封装类（仅使用基础 C++，不使用 STL/动态分配）
//
//  设计目标:
//  - 以 HAL 的 CAN_HandleTypeDef 为依托，提供简洁稳定的发送与接收轮询接口
//  - 无动态内存申请、无异常、无 STL，适用于资源受限的 MCU 环境
//  - 可选回调，在主动轮询时对每帧数据进行处理（避免在中断中做重活）
//
#ifndef CAN_COMM_HPP
#define CAN_COMM_HPP

extern "C" {
#include "stm32f4xx_hal.h"
}

// 嵌入式友好的 CAN 封装类（不使用 STL）
class CanBus {
public:
    // 接收回调函数原型：当 pollOnce() 拉取到一帧数据时被调用
    // 参数:
    //  - header: CAN 接收头（标准/扩展 ID、数据长度等）
    //  - data  : 8 字节数据缓冲区（仅在回调内有效，如需长期保存请自行拷贝）
    //  - user  : 用户自定义指针，便于在回调中访问外部对象
    typedef void (*RxCallback)(const CAN_RxHeaderTypeDef* header, const uint8_t* data, void* user);

    // 构造函数
    // 参数: handle 为底层 HAL 的 CAN 句柄（例如 &hcan1 / &hcan2）
    explicit CanBus(CAN_HandleTypeDef* handle)
        : h_(handle), rx_cb_(0), rx_user_(0) {}

    // 绑定接收回调（可选）。仅在主动调用 pollOnce() 拉取时会触发回调
    // 注意: 回调中不要执行耗时操作，避免阻塞主循环
    void attachRxCallback(RxCallback cb, void* user) {
        rx_cb_ = cb;
        rx_user_ = user;
    }

    // 发送标准帧（非阻塞）
    // 参数:
    //  - std_id: 11 位标准 ID（例如 0x1FF）
    //  - data  : 指向 8 字节以内的数据缓冲区
    //  - len   : 数据长度 [0,8]
    // 返回: HAL_OK 表示成功入队（最终发送由硬件邮箱完成）
    HAL_StatusTypeDef sendStd(uint16_t std_id, const uint8_t* data, uint8_t len) {
        if (!h_) return HAL_ERROR;
        CAN_TxHeaderTypeDef tx = {0};
        uint32_t mailbox = 0;
        tx.StdId = std_id;
        tx.ExtId = 0;
        tx.IDE = CAN_ID_STD;
        tx.RTR = CAN_RTR_DATA;
        tx.DLC = len & 0x0F;
        return HAL_CAN_AddTxMessage(h_, &tx, const_cast<uint8_t*>(data), &mailbox);
    }

    // 发送扩展帧（非阻塞）
    // 参数:
    //  - ext_id: 29 位扩展 ID
    //  - data  : 指向 8 字节以内的数据缓冲区
    //  - len   : 数据长度 [0,8]
    // 返回: HAL_OK 表示成功入队
    HAL_StatusTypeDef sendExt(uint32_t ext_id, const uint8_t* data, uint8_t len) {
        if (!h_) return HAL_ERROR;
        CAN_TxHeaderTypeDef tx = {0};
        uint32_t mailbox = 0;
        tx.StdId = 0;
        tx.ExtId = ext_id;
        tx.IDE = CAN_ID_EXT;
        tx.RTR = CAN_RTR_DATA;
        tx.DLC = len & 0x0F;
        return HAL_CAN_AddTxMessage(h_, &tx, const_cast<uint8_t*>(data), &mailbox);
    }

    // 轮询拉取一帧接收数据（FIFO0）。如有数据并成功取出，则调用回调（若已绑定）
    // 返回: true 表示本次轮询确实取到了一帧；false 表示 FIFO0 为空或取帧失败
    bool pollOnce(void) {
        if (!h_) return false;
        if (HAL_CAN_GetRxFifoFillLevel(h_, CAN_RX_FIFO0) == 0) return false;
        CAN_RxHeaderTypeDef rxh;
        uint8_t buf[8];
        if (HAL_CAN_GetRxMessage(h_, CAN_RX_FIFO0, &rxh, buf) != HAL_OK) return false;
        if (rx_cb_) rx_cb_(&rxh, buf, rx_user_);
        return true;
    }

    // 获取底层 HAL 句柄（在需要直接调用 HAL API 时使用）
    CAN_HandleTypeDef* handle() { return h_; }

private:
    CAN_HandleTypeDef* h_;   // HAL CAN 句柄（外部初始化与启动）
    RxCallback         rx_cb_;   // 接收回调指针
    void*              rx_user_; // 用户上下文指针
};

#endif // CAN_COMM_HPP
