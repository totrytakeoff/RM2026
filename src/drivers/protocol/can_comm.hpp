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

/**
 * @brief 嵌入式友好的 CAN 封装类（不使用 STL、无动态分配）
 *
 * 基于 HAL 的 `CAN_HandleTypeDef` 提供简洁的发送与接收轮询接口，
 * 并支持向上层注册接收回调（回调只在主动 poll 时触发）。
 */
class CanBus {
public:
    /**
     * @brief 接收回调函数类型
     *
     * @param header 接收帧头（标准/扩展 ID、DLC 等）
     * @param data   指向 8 字节数据缓冲区（仅在回调内有效）
     * @param user   用户自定义上下文指针
     */
    typedef void (*RxCallback)(const CAN_RxHeaderTypeDef* header, const uint8_t* data, void* user);

    /**
     * @brief 构造函数
     * @param handle 底层 HAL CAN 句柄（例如 &hcan1）
     */
    explicit CanBus(CAN_HandleTypeDef* handle);

    /**
     * @brief 绑定接收回调（可选）
     * @param cb 回调函数指针
     * @param user 用户上下文指针，原样传入回调
     */
    void attachRxCallback(RxCallback cb, void* user);

    /**
     * @brief 发送标准帧（非阻塞，将数据入硬件邮箱）
     * @param std_id 11 位标准 ID
     * @param data 指向数据缓冲区（最多 8 字节）
     * @param len 数据长度 [0,8]
     * @return HAL_OK 表示成功入队，否则返回 HAL_ERROR 等
     */
    HAL_StatusTypeDef sendStd(uint16_t std_id, const uint8_t* data, uint8_t len);

    /**
     * @brief 发送扩展帧（非阻塞）
     * @param ext_id 29 位扩展 ID
     * @param data 指向数据缓冲区（最多 8 字节）
     * @param len 数据长度 [0,8]
     * @return HAL_OK 表示成功入队
     */
    HAL_StatusTypeDef sendExt(uint32_t ext_id, const uint8_t* data, uint8_t len);

    /**
     * @brief 主动轮询一次接收 FIFO0，如果有帧则取出并触发回调（若已绑定）
     * @return true 本次轮询取到一帧并处理；false FIFO 空或取帧失败
     */
    bool pollOnce(void);

    /**
     * @brief 获取底层 HAL CAN 句柄（用于直接调用 HAL API）
     */
    CAN_HandleTypeDef* handle();

private:
    CAN_HandleTypeDef* h_;   /**< HAL CAN 句柄（外部初始化与启动） */
    RxCallback         rx_cb_;   /**< 接收回调指针 */
    void*              rx_user_; /**< 用户上下文指针 */
};

#endif // CAN_COMM_HPP
