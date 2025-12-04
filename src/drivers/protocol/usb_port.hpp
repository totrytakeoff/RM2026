/**
 * @file usb_port.hpp
 * @brief USB CDC虚拟串口通讯类
 * @version 1.0
 * @date 2025-12-04
 * 
 * @details
 * 提供简洁易用的USB CDC虚拟串口接口，支持：
 * - USB CDC虚拟串口通讯
 * - 环形缓冲区管理（防止数据丢失）
 * - 异步接收回调机制
 * - 连接状态管理
 * - 发送状态检查（避免冲突）
 * - 格式化输出（printf风格）
 */

#ifndef USB_PORT_HPP
#define USB_PORT_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif

/**
 * @brief USB状态枚举
 */
enum class USBStatus {
    OK = 0,         ///< 操作成功
    ERROR,          ///< 一般错误
    BUSY,           ///< USB忙（上次发送未完成）
    TIMEOUT,        ///< 超时
    NOT_CONNECTED,  ///< USB未连接
    NOT_INIT,       ///< 未初始化
    BUFFER_FULL     ///< 缓冲区满
};

/**
 * @brief USB连接状态枚举
 */
enum class USBConnectionState {
    DISCONNECTED,   ///< 未连接
    CONNECTING,     ///< 连接中
    CONNECTED,      ///< 已连接
    SUSPENDED       ///< 挂起
};

/**
 * @brief 接收回调函数类型
 * @param data 接收到的数据指针
 * @param length 数据长度
 */
typedef void (*USBRxCallback)(uint8_t* data, size_t length);

/**
 * @brief 连接状态回调函数类型
 * @param state 连接状态
 */
typedef void (*USBConnectCallback)(USBConnectionState state);

// C包装函数声明（用于C代码调用C++类）
#ifdef __cplusplus
extern "C" {
#endif

struct USBPort;
USBPort* getUSBPortInstance();
void USBPort_rxCallback(USBPort* port, uint8_t* data, uint32_t len);
void USBPort_connectCallback(USBPort* port);
void USBPort_disconnectCallback(USBPort* port);

#ifdef __cplusplus
}
#endif

/**
 * @brief USB CDC虚拟串口通讯类
 * 
 * @example 基本使用示例
 * @code
 * USBPort usb;
 * usb.init();
 * 
 * // 设置接收回调
 * usb.setRxCallback([](uint8_t* data, size_t len) {
 *     usb.send(data, len);  // 回显
 * });
 * 
 * // 发送数据
 * usb.sendString("Hello USB!\r\n");
 * 
 * // 格式化输出
 * usb.printf("Counter: %d\r\n", counter);
 * @endcode
 */
class USBPort {
public:
    /**
     * @brief 构造函数
     */
    USBPort();
    
    /**
     * @brief 析构函数
     */
    ~USBPort();
    
    /**
     * @brief 初始化USB设备
     * @return USBStatus 初始化状态
     */
    USBStatus init();
    
    /**
     * @brief 反初始化USB设备
     * @return USBStatus 反初始化状态
     */
    USBStatus deinit();
    
    /**
     * @brief 发送数据
     * @param data 数据指针
     * @param length 数据长度
     * @param timeout 超时时间(ms)，默认1000ms
     * @return USBStatus 发送状态
     */
    USBStatus send(const uint8_t* data, size_t length, uint32_t timeout = 1000);
    
    /**
     * @brief 发送字符串
     * @param str 字符串指针
     * @param timeout 超时时间(ms)
     * @return USBStatus 发送状态
     */
    USBStatus sendString(const char* str, uint32_t timeout = 1000);
    
    /**
     * @brief 格式化输出（printf风格）
     * @param format 格式化字符串
     * @param ... 可变参数
     * @return USBStatus 发送状态
     */
    USBStatus printf(const char* format, ...);
    
    /**
     * @brief 从接收缓冲区读取数据（非阻塞）
     * @param data 数据缓冲区
     * @param maxLength 最大读取长度
     * @return size_t 实际读取的字节数
     */
    size_t read(uint8_t* data, size_t maxLength);
    
    /**
     * @brief 获取接收缓冲区中可用的数据量
     * @return size_t 可用字节数
     */
    size_t available() const;
    
    /**
     * @brief 清空接收缓冲区
     */
    void flush();
    
    /**
     * @brief 设置接收回调函数
     * @param callback 回调函数指针
     */
    void setRxCallback(USBRxCallback callback);
    
    /**
     * @brief 设置连接状态回调函数
     * @param callback 回调函数指针
     */
    void setConnectCallback(USBConnectCallback callback);
    
    /**
     * @brief 等待USB连接
     * @param timeout 超时时间(ms)，0表示无限等待
     * @return bool true=连接成功，false=超时
     */
    bool waitForConnection(uint32_t timeout = 5000);
    
    /**
     * @brief 检查USB是否已初始化
     * @return bool true=已初始化
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 检查USB是否已连接
     * @return bool true=已连接
     */
    bool isConnected() const { return connected_; }
    
    /**
     * @brief 检查USB是否忙碌
     * @return bool true=忙碌（正在发送）
     */
    bool isBusy() const;
    
    /**
     * @brief 获取连接状态
     * @return USBConnectionState 当前连接状态
     */
    USBConnectionState getConnectionState() const { return connectionState_; }
    
    /**
     * @brief USB接收回调处理（由CDC接口调用）
     * @param data 接收到的数据
     * @param length 数据长度
     */
    void onReceive(uint8_t* data, uint32_t length);

    /**
     * @brief 在主循环中处理接收的数据并调用用户回调（必须由主循环调用）
     * 
     * 说明：USB底层回调可能在中断或USB软中断上下文被调用，
     * 为避免在中断上下文执行用户回调（例如调用发送函数）导致阻塞或不可重入，
     * onReceive() 只将数据写入环形缓冲并标记为待处理。调用 `poll()` 会在主线程
     * 上把环形缓冲的数据取出并安全地调用 `rxCallback_`。
     */
    void poll();
    
    /**
     * @brief USB连接回调处理（由HAL库调用）
     */
    void onConnect();
    
    /**
     * @brief USB断开回调处理（由HAL库调用）
     */
    void onDisconnect();
    
    // 友元函数声明 - 允许C包装函数访问私有成员
    friend USBPort* getUSBPortInstance();
    friend void USBPort_rxCallback(USBPort* port, uint8_t* data, uint32_t len);
    friend void USBPort_connectCallback(USBPort* port);
    friend void USBPort_disconnectCallback(USBPort* port);

private:
    bool initialized_;                      ///< 初始化标志
    volatile bool connected_;               ///< 连接状态标志
    USBConnectionState connectionState_;    ///< 连接状态

    // 接收缓冲区（环形缓冲区）
    static constexpr size_t RING_BUFFER_SIZE = 2048;
    uint8_t ringBuffer_[RING_BUFFER_SIZE];  ///< 环形缓冲区
    volatile size_t rxHead_;                ///< 环形缓冲区头指针（写入位置）
    volatile size_t rxTail_;                ///< 环形缓冲区尾指针（读取位置）

    // USB接收缓冲区（用于底层驱动）
    static constexpr size_t USB_RX_BUFFER_SIZE = 64;
    uint8_t usbRxBuffer_[USB_RX_BUFFER_SIZE]; ///< USB接收缓冲区

    // 发送缓冲区（用于printf等格式化输出）
    static constexpr size_t TX_BUFFER_SIZE = 512;
    uint8_t txBuffer_[TX_BUFFER_SIZE];      ///< 发送缓冲区

    // 回调函数
    USBRxCallback rxCallback_;              ///< 接收回调函数
    USBConnectCallback connectCallback_;    ///< 连接状态回调函数
    volatile bool dataPending_;             ///< 主循环待处理接收标志
    
    /* 发送队列（避免丢包/吞消息） */
    static constexpr size_t TX_SLOT_SIZE = TX_BUFFER_SIZE; // 每个队列槽的最大字节数
    static constexpr size_t TX_QUEUE_SIZE = 8;            // 队列槽数量
    struct TxSlot {
        uint16_t len;
        uint8_t data[TX_SLOT_SIZE];
    };

    TxSlot txQueue_[TX_QUEUE_SIZE];           ///< 发送队列环形缓冲槽
    volatile size_t txHead_;                  ///< 发送队列头（写入位置）
    volatile size_t txTail_;                  ///< 发送队列尾（读取位置）
    volatile size_t txCount_;                 ///< 发送队列中当前元素个数

    /**
     * @brief 处理发送队列（在主循环/上下文中调用）
     */
    void processTxQueue();
    bool enqueueTx(const uint8_t* data, size_t len);
    bool dequeueTx(uint8_t* outBuf, size_t* outLen);

    // 统计数据
    volatile uint32_t stats_tx_enqueued;      ///< 成功入队次数
    volatile uint32_t stats_tx_dequeued;      ///< 出队次数
    volatile uint32_t stats_tx_queue_full;    ///< 入队失败（队列满）次数
    volatile uint32_t stats_tx_busy;          ///< 发送返回 BUSY 次数
    volatile uint32_t stats_tx_ok;            ///< 发送成功次数
    volatile uint32_t stats_rx_dropped;       ///< 接收写入环形缓冲被丢弃次数
    

    /**
     * @brief 启动USB接收
     */
    void startReceive();
    
    /**
     * @brief 将数据写入环形缓冲区
     * @param data 数据指针
     * @param length 数据长度
     * @return size_t 实际写入的字节数
     */
    size_t writeToRingBuffer(const uint8_t* data, size_t length);
    
    /**
     * @brief 从环形缓冲区读取数据
     * @param data 数据缓冲区
     * @param maxLength 最大读取长度
     * @return size_t 实际读取的字节数
     */
    size_t readFromRingBuffer(uint8_t* data, size_t maxLength);
    
    /**
     * @brief 获取环形缓冲区可用空间
     * @return size_t 可用字节数
     */
    size_t getRingBufferFreeSpace() const;
    
    /**
     * @brief 获取环形缓冲区已用空间
     * @return size_t 已用字节数
     */
    size_t getRingBufferUsedSpace() const;
};

#endif // USB_PORT_HPP
