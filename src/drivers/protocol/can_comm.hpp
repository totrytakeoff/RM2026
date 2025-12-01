//
//  文件: can_comm.hpp
//  说明: 面向嵌入式的通用 CAN 收发轻量封装类（仅使用基础 C++，不使用 STL/动态分配）
//
//  设计目标:
//  - 以 HAL 的 CAN_HandleTypeDef 为依托，提供简洁稳定的发送与接收轮询接口
//  - 无动态内存申请、无异常、无 STL，适用于资源受限的 MCU 环境
//  - 支持多个回调注册，在主动轮询时对每帧数据进行处理（避免在中断中做重活）
//  - 支持基于 CAN ID 的回调过滤，提高处理效率
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
 * 并支持向上层注册多个接收回调（回调只在主动 poll 时触发）。
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
     * @brief 回调节点结构体（链表节点）
     */
    struct CallbackNode {
        RxCallback callback;     /**< 回调函数指针 */
        void* user_data;         /**< 用户上下文数据 */
        uint32_t filter_id;      /**< 过滤 ID（0 表示接收所有）*/
        bool use_filter;         /**< 是否启用 ID 过滤 */
        bool is_ext_id;          /**< 是否为扩展 ID */
        CallbackNode* next;      /**< 下一个节点 */
    };

    /**
     * @brief 最大回调数量限制（静态数组大小）
     */
    static constexpr uint8_t MAX_CALLBACKS = 16;

    /**
     * @brief 构造函数
     * @param handle 底层 HAL CAN 句柄（例如 &hcan1）
     */
    explicit CanBus(CAN_HandleTypeDef* handle);

    /**
     * @brief 注册接收回调（支持多个回调）
     * @param cb 回调函数指针
     * @param user 用户上下文指针，原样传入回调
     * @return true=注册成功，false=回调列表已满
     */
    bool registerRxCallback(RxCallback cb, void* user = nullptr);

    /**
     * @brief 注册带 ID 过滤的接收回调
     * @param cb 回调函数指针
     * @param filter_id 过滤的 CAN ID（只有匹配的帧才会触发此回调）
     * @param is_ext_id 是否为扩展 ID
     * @param user 用户上下文指针
     * @return true=注册成功，false=回调列表已满
     */
    bool registerRxCallback(RxCallback cb, uint32_t filter_id, bool is_ext_id, void* user = nullptr);

    /**
     * @brief 注销指定的回调函数
     * @param cb 要注销的回调函数指针
     * @return true=注销成功，false=未找到该回调
     */
    bool unregisterRxCallback(RxCallback cb);

    /**
     * @brief 清除所有回调
     */
    void clearAllCallbacks();

    /**
     * @brief 获取当前注册的回调数量
     * @return 回调数量
     */
    uint8_t getCallbackCount() const;

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
    /**
     * @brief 触发所有匹配的回调
     * @param header 接收帧头
     * @param data 接收数据
     */
    void invokeCallbacks(const CAN_RxHeaderTypeDef* header, const uint8_t* data);

private:
    CAN_HandleTypeDef* h_;           /**< HAL CAN 句柄（外部初始化与启动） */
    CallbackNode callback_pool_[MAX_CALLBACKS]; /**< 回调节点池（静态分配）*/
    CallbackNode* callback_list_;    /**< 回调链表头指针 */
    uint8_t callback_count_;         /**< 当前回调数量 */
};

#endif // CAN_COMM_HPP
