/**
 * @file serial_port.hpp
 * @brief 通用串口通讯类 - 支持UART和USB-CDC
 * @version 1.0
 * @date 2024-12-02
 * 
 * @details
 * 提供统一的串口通讯接口，支持：
 * - 硬件UART (USART1, USART6等)
 * - USB虚拟串口 (CDC)
 * - 多种工作模式：阻塞/中断/DMA
 * - IDLE空闲中断处理不定长数据
 * - 环形缓冲区管理
 * - 错误处理和超时机制
 */

#ifndef SERIAL_PORT_HPP
#define SERIAL_PORT_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

/**
 * @brief 串口类型枚举
 */
enum class SerialType {
    UART1,      ///< 硬件串口1 (USART1)
    UART2,      ///< 硬件串口2 (USART2)
    UART3,      ///< 硬件串口3 (USART3)
    UART6,      ///< 硬件串口6 (USART6)
    USB_CDC     ///< USB虚拟串口
};

/**
 * @brief 串口工作模式枚举
 */
enum class SerialMode {
    POLLING,    ///< 阻塞轮询模式
    INTERRUPT,  ///< 中断模式
    DMA,        ///< DMA模式
    DMA_IDLE    ///< DMA + IDLE空闲中断模式（推荐）
};

/**
 * @brief 串口状态枚举
 */
enum class SerialStatus {
    OK = 0,         ///< 操作成功
    ERROR,          ///< 一般错误
    BUSY,           ///< 串口忙
    TIMEOUT,        ///< 超时
    BUFFER_FULL,    ///< 缓冲区满
    NOT_INIT        ///< 未初始化
};

/**
 * @brief 串口配置结构体
 */
struct SerialConfig {
    uint32_t baudrate;      ///< 波特率 (9600, 115200等)
    uint32_t wordLength;    ///< 数据位 (8位或9位)
    uint32_t stopBits;      ///< 停止位 (1位或2位)
    uint32_t parity;        ///< 校验位 (无/奇/偶)
    SerialMode mode;        ///< 工作模式
    
    // 默认配置：115200, 8N1, DMA+IDLE模式
    SerialConfig() 
        : baudrate(115200)
        , wordLength(UART_WORDLENGTH_8B)
        , stopBits(UART_STOPBITS_1)
        , parity(UART_PARITY_NONE)
        , mode(SerialMode::DMA_IDLE)
    {}
};

/**
 * @brief 接收回调函数类型
 * @param data 接收到的数据指针
 * @param length 数据长度
 */
typedef void (*SerialRxCallback)(uint8_t* data, size_t length);

// 为了让 C 中断处理函数能够调用这些包装函数，并且
// 保持链接一致性，在此提供 C 链接（extern "C"）的函数原型。
#ifdef __cplusplus
extern "C" {
#endif

// 注意：这些函数在 C 文件中以不完整类型引用（typedef struct SerialPort），
// 在 C++ 中它们接收 `SerialPort*` 指针。声明在此处以确保链接类型一致。
struct SerialPort;
SerialPort* getSerialPort(int index);
void SerialPort_handleIRQ(SerialPort* port);
void SerialPort_rxCompleteCallback(SerialPort* port);
void SerialPort_txCompleteCallback(SerialPort* port);
void SerialPort_errorCallback(SerialPort* port);
UART_HandleTypeDef* SerialPort_getUartHandle(SerialPort* port);
DMA_HandleTypeDef* SerialPort_getDmaTxHandle(SerialPort* port);
DMA_HandleTypeDef* SerialPort_getDmaRxHandle(SerialPort* port);

#ifdef __cplusplus
}
#endif

/**
 * @brief 通用串口通讯类
 * 
 * @example 基本使用示例
 * @code
 * SerialPort uart1(SerialType::UART1);
 * SerialConfig config;
 * config.baudrate = 115200;
 * config.mode = SerialMode::DMA_IDLE;
 * 
 * uart1.init(config);
 * uart1.setRxCallback([](uint8_t* data, size_t len) {
 *     // 处理接收到的数据
 * });
 * 
 * uint8_t txData[] = "Hello";
 * uart1.send(txData, 5);
 * @endcode
 */
class SerialPort {
public:
    /**
     * @brief 构造函数
     * @param type 串口类型
     */
    explicit SerialPort(SerialType type);
    
    /**
     * @brief 析构函数
     */
    ~SerialPort();
    
    /**
     * @brief 初始化串口
     * @param config 串口配置
     * @return SerialStatus 初始化状态
     */
    SerialStatus init(const SerialConfig& config = SerialConfig());
    
    /**
     * @brief 反初始化串口
     * @return SerialStatus 反初始化状态
     */
    SerialStatus deinit();
    
    /**
     * @brief 发送数据
     * @param data 数据指针
     * @param length 数据长度
     * @param timeout 超时时间(ms)，默认1000ms
     * @return SerialStatus 发送状态
     */
    SerialStatus send(const uint8_t* data, size_t length, uint32_t timeout = 1000);
    
    /**
     * @brief 发送字符串
     * @param str 字符串指针
     * @param timeout 超时时间(ms)
     * @return SerialStatus 发送状态
     */
    SerialStatus sendString(const char* str, uint32_t timeout = 1000);
    
    /**
     * @brief 接收数据（阻塞模式）
     * @param data 数据缓冲区
     * @param length 期望接收的长度
     * @param timeout 超时时间(ms)
     * @return SerialStatus 接收状态
     */
    SerialStatus receive(uint8_t* data, size_t length, uint32_t timeout = 1000);
    
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
    void setRxCallback(SerialRxCallback callback);
    
    /**
     * @brief 启动接收（中断或DMA模式）
     * @return SerialStatus 启动状态
     */
    SerialStatus startReceive();
    
    /**
     * @brief 停止接收
     * @return SerialStatus 停止状态
     */
    SerialStatus stopReceive();
    
    /**
     * @brief 检查串口是否已初始化
     * @return bool true=已初始化
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 检查串口是否忙碌
     * @return bool true=忙碌
     */
    bool isBusy() const;
    
    /**
     * @brief 获取串口句柄（用于底层操作）
     * @return UART_HandleTypeDef* UART句柄指针
     */
    UART_HandleTypeDef* getHandle() { return huart_; }
    
    /**
     * @brief 获取当前工作模式
     * @return SerialMode 当前工作模式
     */
    SerialMode getMode() const { return mode_; }
    
    /**
     * @brief 获取串口类型
     * @return SerialType 串口类型
     */
    SerialType getType() const { return type_; }
    
    /**
     * @brief UART中断回调处理（由IRQ Handler调用）
     */
    void handleIRQ();
    
    /**
     * @brief UART接收完成回调（由HAL库调用）
     */
    void rxCompleteCallback();
    
    /**
     * @brief UART发送完成回调（由HAL库调用）
     */
    void txCompleteCallback();
    
    /**
     * @brief UART错误回调（由HAL库调用）
     */
    void errorCallback();
    
    // 友元函数声明 - 允许C包装函数访问私有成员
    friend void SerialPort_handleIRQ(SerialPort* port);
    friend void SerialPort_rxCompleteCallback(SerialPort* port);
    friend void SerialPort_txCompleteCallback(SerialPort* port);
    friend void SerialPort_errorCallback(SerialPort* port);
    friend UART_HandleTypeDef* SerialPort_getUartHandle(SerialPort* port);
    friend DMA_HandleTypeDef* SerialPort_getDmaTxHandle(SerialPort* port);
    friend DMA_HandleTypeDef* SerialPort_getDmaRxHandle(SerialPort* port);

private:
    SerialType type_;                   ///< 串口类型
    SerialMode mode_;                   ///< 工作模式
    bool initialized_;                  ///< 初始化标志
    
    UART_HandleTypeDef* huart_;         ///< UART句柄
    DMA_HandleTypeDef* hdma_tx_;        ///< DMA发送句柄
    DMA_HandleTypeDef* hdma_rx_;        ///< DMA接收句柄
    
    // 接收缓冲区
    static constexpr size_t RX_BUFFER_SIZE = 1024;
    uint8_t rxBuffer_[RX_BUFFER_SIZE];  ///< DMA接收缓冲区
    uint8_t rxRingBuffer_[RX_BUFFER_SIZE * 2]; ///< 环形缓冲区
    volatile size_t rxHead_;            ///< 环形缓冲区头指针
    volatile size_t rxTail_;            ///< 环形缓冲区尾指针
    
    SerialRxCallback rxCallback_;       ///< 接收回调函数
    
    /**
     * @brief 初始化GPIO引脚
     * @return SerialStatus 初始化状态
     */
    SerialStatus initGPIO();
    
    /**
     * @brief 初始化UART外设
     * @param config 配置参数
     * @return SerialStatus 初始化状态
     */
    SerialStatus initUART(const SerialConfig& config);
    
    /**
     * @brief 初始化DMA
     * @return SerialStatus 初始化状态
     */
    SerialStatus initDMA();
    
    /**
     * @brief 初始化NVIC中断
     * @return SerialStatus 初始化状态
     */
    SerialStatus initNVIC();
    
    /**
     * @brief 使能IDLE空闲中断
     */
    void enableIdleInterrupt();
    
    /**
     * @brief 处理IDLE空闲中断
     */
    void handleIdleInterrupt();
    
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
};

#endif // SERIAL_PORT_HPP
