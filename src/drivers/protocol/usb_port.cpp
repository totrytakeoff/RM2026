/**
 * @file usb_port.cpp
 * @brief USB CDC虚拟串口通讯类实现
 * @version 1.0
 * @date 2025-12-04
 */

#include "usb_port.hpp"
#include "../hal/usb_device.h"
#include "main.h"
#include "../hal/usbd_cdc_if.h"

#ifdef __cplusplus
extern "C" {
#endif
extern USBD_HandleTypeDef hUsbDeviceFS;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

// 全局USB实例指针（用于C回调访问）
static USBPort* g_usbPortInstance = nullptr;

// ============================================================================
// C包装函数实现（用于C代码调用C++类）
// ============================================================================

extern "C" {

/**
 * @brief 获取全局USB实例
 */
USBPort* getUSBPortInstance() {
    return g_usbPortInstance;
}

/**
 * @brief USB接收回调包装函数
 */
void USBPort_rxCallback(USBPort* port, uint8_t* data, uint32_t len) {
    if (port) {
        port->onReceive(data, len);
    }
}

/**
 * @brief USB连接回调包装函数
 */
void USBPort_connectCallback(USBPort* port) {
    if (port) {
        port->onConnect();
    }
}

/**
 * @brief USB断开回调包装函数
 */
void USBPort_disconnectCallback(USBPort* port) {
    if (port) {
        port->onDisconnect();
    }
}

} // extern "C"

// ============================================================================
// USBPort类实现
// ============================================================================

/**
 * @brief 构造函数
 */
USBPort::USBPort()
    : initialized_(false)
    , connected_(false)
    , connectionState_(USBConnectionState::DISCONNECTED)
    , rxHead_(0)
    , rxTail_(0)
    , rxCallback_(nullptr)
    , connectCallback_(nullptr)
    , dataPending_(false)
    , txHead_(0)
    , txTail_(0)
    , txCount_(0)
    , stats_tx_enqueued(0)
    , stats_tx_dequeued(0)
    , stats_tx_queue_full(0)
    , stats_tx_busy(0)
    , stats_tx_ok(0)
    , stats_rx_dropped(0)
{
    // 设置全局实例指针
    g_usbPortInstance = this;

    // 清空缓冲区
    memset(ringBuffer_, 0, sizeof(ringBuffer_));
    memset(txBuffer_, 0, sizeof(txBuffer_));
    memset(usbRxBuffer_, 0, sizeof(usbRxBuffer_));
}

/**
 * @brief 析构函数
 */
USBPort::~USBPort()
{
    deinit();
    
    // 清除全局实例指针
    if (g_usbPortInstance == this) {
        g_usbPortInstance = nullptr;
    }
}

/**
 * @brief 初始化USB设备
 * @note USB硬件应该在系统启动时通过MX_USB_DEVICE_Init()初始化
 *       这个函数只是标记USBPort类已准备好使用
 */
USBStatus USBPort::init()
{
    if (initialized_) {
        return USBStatus::OK;
    }
    
    // 确保USB硬件已初始化
    // 注意：MX_USB_DEVICE_Init()应该在main()中的BSP初始化后调用
    // 如果还未初始化，这里进行初始化
    extern USBD_HandleTypeDef hUsbDeviceFS;
    if (hUsbDeviceFS.pData == nullptr) {
        // USB设备未初始化，进行初始化
        MX_USB_DEVICE_Init();
        HAL_Delay(100);  // 等待USB枚举完成
    }
    
    // 设置初始化标志
    initialized_ = true;


    return USBStatus::OK;
}

/**
 * @brief 反初始化USB设备
 */
USBStatus USBPort::deinit()
{
    if (!initialized_) {
        return USBStatus::NOT_INIT;
    }
    
    // 清空缓冲区
    flush();
    
    // 清除回调
    rxCallback_ = nullptr;
    connectCallback_ = nullptr;
    
    // 清除标志
    initialized_ = false;
    connected_ = false;
    connectionState_ = USBConnectionState::DISCONNECTED;
    
    return USBStatus::OK;
}

/**
 * @brief 发送数据
 */
USBStatus USBPort::send(const uint8_t* data, size_t length, uint32_t timeout)
{
    if (!initialized_) {
        return USBStatus::NOT_INIT;
    }
    
    if (!data || length == 0) {
        return USBStatus::ERROR;
    }
    
    // Ensure CDC class is configured
    extern USBD_HandleTypeDef hUsbDeviceFS;
    if (hUsbDeviceFS.pClassData == nullptr) {
        return USBStatus::NOT_CONNECTED;
    }
    
    // 非阻塞入队策略：在ISR或非ISR中尽量入队，主循环会处理发送
    bool inISR = (__get_IPSR() != 0);

    // 如果队列空且USB空闲，优先直接发送以减少延迟（仅在非ISR上下文）
    if (!inISR) {
        if (txCount_ == 0 && !isBusy()) {
            uint8_t res = CDC_Transmit_FS((uint8_t*)data, (uint16_t)length);
            if (res == USBD_OK) {
                return USBStatus::OK;
            }
            // 如果返回 BUSY 或 FAIL，我们会尝试将数据入队
        }
    }
    // 非阻塞：尝试一次入队，不等待（避免在主循环阻塞其它逻辑）
    if (enqueueTx(data, length)) {
        return USBStatus::OK;
    }

    // 入队失败（队列满）: 立即返回 BUSY，以便调用方决定后续策略
    return USBStatus::BUSY;
}

/**
 * @brief 发送字符串
 */
USBStatus USBPort::sendString(const char* str, uint32_t timeout)
{
    if (!str) {
        return USBStatus::ERROR;
    }
    
    return send((const uint8_t*)str, strlen(str), timeout);
}

/**
 * @brief 格式化输出（printf风格）
 */
USBStatus USBPort::printf(const char* format, ...)
{
    if (!format) {
        return USBStatus::ERROR;
    }
    
    va_list args;
    va_start(args, format);
    
    // 格式化到缓冲区
    int len = vsnprintf((char*)txBuffer_, TX_BUFFER_SIZE, format, args);
    
    va_end(args);
    
    if (len < 0) {
        return USBStatus::ERROR;
    }
    
    // 限制长度
    if (len >= (int)TX_BUFFER_SIZE) {
        len = TX_BUFFER_SIZE - 1;
    }
    
    // 发送数据
    return send(txBuffer_, len);
}

/**
 * @brief 从接收缓冲区读取数据
 */
size_t USBPort::read(uint8_t* data, size_t maxLength)
{
    if (!data || maxLength == 0) {
        return 0;
    }
    
    return readFromRingBuffer(data, maxLength);
}

/**
 * @brief 获取接收缓冲区中可用的数据量
 */
size_t USBPort::available() const
{
    return getRingBufferUsedSpace();
}

/**
 * @brief 清空接收缓冲区
 */
void USBPort::flush()
{
    // 禁用中断，保证原子操作
    __disable_irq();
    
    rxHead_ = 0;
    rxTail_ = 0;
    
    __enable_irq();
}

/**
 * @brief 设置接收回调函数
 */
void USBPort::setRxCallback(USBRxCallback callback)
{
    rxCallback_ = callback;
}

/**
 * @brief 设置连接状态回调函数
 */
void USBPort::setConnectCallback(USBConnectCallback callback)
{
    connectCallback_ = callback;
}

/**
 * @brief 等待USB连接
 */
bool USBPort::waitForConnection(uint32_t timeout)
{
    if (!initialized_) {
        return false;
    }
    
    if (connected_) {
        return true;
    }
    
    uint32_t startTick = HAL_GetTick();
    
    while (!connected_) {
        if (timeout > 0 && (HAL_GetTick() - startTick) >= timeout) {
            return false;
        }
        HAL_Delay(10);
    }
    
    return true;
}

/**
 * @brief 检查USB是否忙碌
 */
bool USBPort::isBusy() const
{
    extern USBD_HandleTypeDef hUsbDeviceFS;
    
    if (!initialized_) {
        return false;
    }
    
    // 获取CDC类句柄
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    
    if (hcdc == nullptr) {
        return false;
    }
    
    // 检查发送状态
    return (hcdc->TxState != 0);
}

/**
 * @brief USB接收回调处理
 */
void USBPort::onReceive(uint8_t* data, uint32_t length)
{
    if (!data || length == 0) {
        // 即使没有数据，也重新启动接收
        startReceive();
        return;
    }

    // 调试输出
    // printf("[USB] Received %u bytes\n", length);

    // 将数据写入环形缓冲区
    size_t written = writeToRingBuffer(data, length);
    (void)written;  // 抑制未使用变量警告
    
    // 标记主线程有数据待处理（不要在中断上下文执行用户回调）
    if (length > 0) {
        dataPending_ = true;
    }

}

/**
 * @brief 在主循环中调用，处理环形缓冲区中的数据并调用用户回调
 */
void USBPort::poll()
{
    // 如果有接收数据待处理，先处理接收数据
    if (dataPending_) {
        // 在主上下文读取所有可用数据并一次性交给回调（分块处理）
        const size_t chunkSize = 256;
        uint8_t tmp[chunkSize];

        while (getRingBufferUsedSpace() > 0) {
            size_t toRead = getRingBufferUsedSpace();
            if (toRead > chunkSize) toRead = chunkSize;
            size_t actually = readFromRingBuffer(tmp, toRead);
            if (actually == 0) break;

            if (rxCallback_) {
                rxCallback_(tmp, actually);
            }
        }

        // 已处理完所有数据，清除标志
        dataPending_ = false;
    }

    // 无论是否有接收数据，都处理发送队列，确保队列能被及时发送
    processTxQueue();

    // 周期性输出统计（每5秒）以便调试
    static uint32_t lastStat = 0;
    uint32_t now = HAL_GetTick();
    if (lastStat == 0) lastStat = now;
    if ((now - lastStat) >= 5000) {
        lastStat = now;
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "TX enq:%lu deq:%lu full:%lu busy:%lu ok:%lu RXdrop:%lu\r\n",
                         (unsigned long)stats_tx_enqueued,
                         (unsigned long)stats_tx_dequeued,
                         (unsigned long)stats_tx_queue_full,
                         (unsigned long)stats_tx_busy,
                         (unsigned long)stats_tx_ok,
                         (unsigned long)stats_rx_dropped);
        if (n > 0) {
            // 使用 sendString（会入队）打印统计
            sendString(buf);
        }
    }
}

bool USBPort::enqueueTx(const uint8_t* data, size_t len)
{
    if (len == 0 || len > TX_SLOT_SIZE) return false;
    __disable_irq();
    if (txCount_ >= TX_QUEUE_SIZE) {
        stats_tx_queue_full++;
        __enable_irq();
        return false; // full
    }
    size_t idx = txHead_;
    txQueue_[idx].len = (uint16_t)len;
    memcpy(txQueue_[idx].data, data, len);
    txHead_ = (txHead_ + 1) % TX_QUEUE_SIZE;
    txCount_++;
    stats_tx_enqueued++;
    __enable_irq();
    return true;
}

bool USBPort::dequeueTx(uint8_t* outBuf, size_t* outLen)
{
    __disable_irq();
    if (txCount_ == 0) {
        __enable_irq();
        return false;
    }
    size_t idx = txTail_;
    uint16_t len = txQueue_[idx].len;
    if (len > 0 && outBuf && outLen) {
        memcpy(outBuf, txQueue_[idx].data, len);
        *outLen = len;
    }
    txTail_ = (txTail_ + 1) % TX_QUEUE_SIZE;
    txCount_--;
    stats_tx_dequeued++;
    __enable_irq();
    return true;
}

/**
 * @brief 在主循环中处理发送队列：当USB空闲时发送队列中的下一条
 */
void USBPort::processTxQueue()
{
    // 如果当前正在发送，则等待完成
    // 尽可能多地发送队列中的数据，直到USB忙或队列空
    uint8_t tmp[TX_BUFFER_SIZE];
    size_t len = 0;
    while (!isBusy()) {
        // 如果队列为空，退出
        if (txCount_ == 0) break;

        if (!dequeueTx(tmp, &len)) break;

        uint8_t res = CDC_Transmit_FS(tmp, (uint16_t)len);
        if (res == USBD_OK) {
            stats_tx_ok++;
            // 继续发送下一个（如果还有）
            continue;
        } else if (res == USBD_BUSY) {
            // USB变为忙，统计并把当前数据回退到队首
            stats_tx_busy++;
            __disable_irq();
            size_t newHead = (txHead_ + TX_QUEUE_SIZE - 1) % TX_QUEUE_SIZE;
            txHead_ = newHead;
            txQueue_[newHead].len = (uint16_t)len;
            memcpy(txQueue_[newHead].data, tmp, len);
            txCount_++;
            __enable_irq();
            break;
        } else {
            // 发送失败：统计并继续处理下一条
            continue;
        }
    }
}

/**
 * @brief USB连接回调处理
 */
void USBPort::onConnect()
{
    // If CDC class is not yet configured, report CONNECTING
    extern USBD_HandleTypeDef hUsbDeviceFS;
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    if (hcdc == nullptr) {
        connected_ = false;
        connectionState_ = USBConnectionState::CONNECTING;
        if (connectCallback_) {
            connectCallback_(USBConnectionState::CONNECTING);
        }
        return;
    }
    
    connected_ = true;
    connectionState_ = USBConnectionState::CONNECTED;
    if (connectCallback_) {
        connectCallback_(USBConnectionState::CONNECTED);
    }
}

/**
 * @brief USB断开回调处理
 */
void USBPort::onDisconnect()
{
    connected_ = false;
    connectionState_ = USBConnectionState::DISCONNECTED;

    // 触发连接回调
    if (connectCallback_) {
        connectCallback_(USBConnectionState::DISCONNECTED);
    }
}

/**
 * @brief 启动USB接收
 */
void USBPort::startReceive()
{
    // 确保已经初始化
    if (!initialized_) {
        return;
    }

    // 启动接收
    // 将此类的接收缓冲区设置给USB栈并准备接收下一个包
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, usbRxBuffer_);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

// ============================================================================
// 环形缓冲区私有方法
// ============================================================================

/**
 * @brief 将数据写入环形缓冲区
 */
size_t USBPort::writeToRingBuffer(const uint8_t* data, size_t length)
{
    if (!data || length == 0) {
        return 0;
    }
    
    // 禁用中断，保证原子操作
    __disable_irq();
    
    size_t freeSpace = getRingBufferFreeSpace();
    size_t writeLen = (length < freeSpace) ? length : freeSpace;
    
    for (size_t i = 0; i < writeLen; i++) {
        ringBuffer_[rxHead_] = data[i];
        rxHead_ = (rxHead_ + 1) % RING_BUFFER_SIZE;
    }
    
    __enable_irq();
    if (writeLen < length) {
        stats_rx_dropped++;
    }

    return writeLen;
}

/**
 * @brief 从环形缓冲区读取数据
 */
size_t USBPort::readFromRingBuffer(uint8_t* data, size_t maxLength)
{
    if (!data || maxLength == 0) {
        return 0;
    }
    
    // 禁用中断，保证原子操作
    __disable_irq();
    
    size_t usedSpace = getRingBufferUsedSpace();
    size_t readLen = (maxLength < usedSpace) ? maxLength : usedSpace;
    
    for (size_t i = 0; i < readLen; i++) {
        data[i] = ringBuffer_[rxTail_];
        rxTail_ = (rxTail_ + 1) % RING_BUFFER_SIZE;
    }
    
    __enable_irq();
    
    return readLen;
}

/**
 * @brief 获取环形缓冲区可用空间
 */
size_t USBPort::getRingBufferFreeSpace() const
{
    size_t used = getRingBufferUsedSpace();
    return RING_BUFFER_SIZE - used - 1;  // 保留1字节区分满和空
}

/**
 * @brief 获取环形缓冲区已用空间
 */
size_t USBPort::getRingBufferUsedSpace() const
{
    if (rxHead_ >= rxTail_) {
        return rxHead_ - rxTail_;
    } else {
        return RING_BUFFER_SIZE - rxTail_ + rxHead_;
    }
}
