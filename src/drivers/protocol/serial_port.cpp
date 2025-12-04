/**
 * @file serial_port.cpp
 * @brief 通用串口通讯类实现
 */

#include "serial_port.hpp"
#include "pin_map.h"

// 静态UART句柄实例
static UART_HandleTypeDef huart1_instance;
static UART_HandleTypeDef huart2_instance;
static UART_HandleTypeDef huart3_instance;
static UART_HandleTypeDef huart6_instance;

// 静态DMA句柄实例
static DMA_HandleTypeDef hdma_usart1_tx;
static DMA_HandleTypeDef hdma_usart1_rx;
static DMA_HandleTypeDef hdma_usart6_tx;
static DMA_HandleTypeDef hdma_usart6_rx;

// 全局SerialPort实例指针（用于中断回调）
static SerialPort* g_serialPorts[4] = {nullptr, nullptr, nullptr, nullptr};

/**
 * @brief 构造函数
 */
SerialPort::SerialPort(SerialType type)
    : type_(type)
    , mode_(SerialMode::DMA_IDLE)
    , initialized_(false)
    , huart_(nullptr)
    , hdma_tx_(nullptr)
    , hdma_rx_(nullptr)
    , rxHead_(0)
    , rxTail_(0)
    , rxCallback_(nullptr)
{
    // 根据类型分配UART句柄
    switch (type_) {
        case SerialType::UART1:
            huart_ = &huart1_instance;
            hdma_tx_ = &hdma_usart1_tx;
            hdma_rx_ = &hdma_usart1_rx;
            break;
        case SerialType::UART2:
            huart_ = &huart2_instance;
            g_serialPorts[1] = this;
            break;
        case SerialType::UART3:
            huart_ = &huart3_instance;
            g_serialPorts[2] = this;
            break;
        case SerialType::UART6:
            huart_ = &huart6_instance;
            hdma_tx_ = &hdma_usart6_tx;
            hdma_rx_ = &hdma_usart6_rx;
            break;
        case SerialType::USB_CDC:
            // USB CDC将在后续实现
            break;
    }
    
    memset(rxBuffer_, 0, RX_BUFFER_SIZE);
    memset(rxRingBuffer_, 0, RX_BUFFER_SIZE * 2);
}

/**
 * @brief 析构函数
 */
SerialPort::~SerialPort() {
    deinit();
}

/**
 * @brief 初始化串口
 */
SerialStatus SerialPort::init(const SerialConfig& config) {
    if (initialized_) {
        return SerialStatus::OK;
    }
    
    mode_ = config.mode;
    
    // 1. 初始化GPIO
    if (initGPIO() != SerialStatus::OK) {
        return SerialStatus::ERROR;
    }
    
    // 2. 初始化UART外设
    if (initUART(config) != SerialStatus::OK) {
        return SerialStatus::ERROR;
    }
    
    // 3. 如果使用DMA模式，初始化DMA
    if (mode_ == SerialMode::DMA || mode_ == SerialMode::DMA_IDLE) {
        if (initDMA() != SerialStatus::OK) {
            return SerialStatus::ERROR;
        }
    }
    
    // 4. 初始化NVIC中断
    if (initNVIC() != SerialStatus::OK) {
        return SerialStatus::ERROR;
    }
    
    // 5. 如果使用IDLE中断模式，使能IDLE中断
    if (mode_ == SerialMode::DMA_IDLE) {
        enableIdleInterrupt();
    }
    
    initialized_ = true;
    // 注册到全局实例表，供中断处理器查找（延后到 init 以避免静态初始化顺序问题）
    switch (type_) {
        case SerialType::UART1:
            g_serialPorts[0] = this;
            break;
        case SerialType::UART2:
            g_serialPorts[1] = this;
            break;
        case SerialType::UART3:
            g_serialPorts[2] = this;
            break;
        case SerialType::UART6:
            g_serialPorts[3] = this;
            break;
        default:
            break;
    }

    // 6. 自动启动接收
    startReceive();
    
    return SerialStatus::OK;
}

/**
 * @brief 反初始化串口
 */
SerialStatus SerialPort::deinit() {
    if (!initialized_) {
        return SerialStatus::OK;
    }
    
    stopReceive();
    
    if (huart_) {
        HAL_UART_DeInit(huart_);
    }
    
    initialized_ = false;
    // 注销全局实例表
    switch (type_) {
        case SerialType::UART1:
            if (g_serialPorts[0] == this) g_serialPorts[0] = nullptr;
            break;
        case SerialType::UART2:
            if (g_serialPorts[1] == this) g_serialPorts[1] = nullptr;
            break;
        case SerialType::UART3:
            if (g_serialPorts[2] == this) g_serialPorts[2] = nullptr;
            break;
        case SerialType::UART6:
            if (g_serialPorts[3] == this) g_serialPorts[3] = nullptr;
            break;
        default:
            break;
    }
    return SerialStatus::OK;
}

/**
 * @brief 初始化GPIO引脚
 */
SerialStatus SerialPort::initGPIO() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    switch (type_) {
        case SerialType::UART1: {
            // 使能时钟
            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            __HAL_RCC_USART1_CLK_ENABLE();
            
            // TX: PA9
            GPIO_InitStruct.Pin = UART1_TX_PIN;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
            HAL_GPIO_Init(UART1_TX_GPIO_PORT, &GPIO_InitStruct);
            
            // RX: PB7
            GPIO_InitStruct.Pin = UART1_RX_PIN;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
            HAL_GPIO_Init(UART1_RX_GPIO_PORT, &GPIO_InitStruct);
            break;
        }
        
        case SerialType::UART6: {
            // 使能时钟
            __HAL_RCC_GPIOG_CLK_ENABLE();
            __HAL_RCC_USART6_CLK_ENABLE();
            
            // TX: PG14
            GPIO_InitStruct.Pin = UART6_TX_PIN;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
            HAL_GPIO_Init(UART6_TX_GPIO_PORT, &GPIO_InitStruct);
            
            // RX: PG9
            GPIO_InitStruct.Pin = UART6_RX_PIN;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
            HAL_GPIO_Init(UART6_RX_GPIO_PORT, &GPIO_InitStruct);
            break;
        }
        
        default:
            return SerialStatus::ERROR;
    }
    
    return SerialStatus::OK;
}

/**
 * @brief 初始化UART外设
 */
SerialStatus SerialPort::initUART(const SerialConfig& config) {
    if (!huart_) {
        return SerialStatus::ERROR;
    }
    
    // 设置UART实例
    switch (type_) {
        case SerialType::UART1:
            huart_->Instance = USART1;
            break;
        case SerialType::UART2:
            huart_->Instance = USART2;
            break;
        case SerialType::UART3:
            huart_->Instance = USART3;
            break;
        case SerialType::UART6:
            huart_->Instance = USART6;
            break;
        default:
            return SerialStatus::ERROR;
    }
    
    // 配置UART参数
    huart_->Init.BaudRate = config.baudrate;
    huart_->Init.WordLength = config.wordLength;
    huart_->Init.StopBits = config.stopBits;
    huart_->Init.Parity = config.parity;
    huart_->Init.Mode = UART_MODE_TX_RX;
    huart_->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_->Init.OverSampling = UART_OVERSAMPLING_16;
    
    // 初始化UART
    if (HAL_UART_Init(huart_) != HAL_OK) {
        return SerialStatus::ERROR;
    }
    
    return SerialStatus::OK;
}

/**
 * @brief 初始化DMA
 */
SerialStatus SerialPort::initDMA() {
    if (!hdma_tx_ || !hdma_rx_) {
        return SerialStatus::ERROR;
    }
    
    // 使能DMA时钟
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    switch (type_) {
        case SerialType::UART1: {
            // DMA TX: DMA2 Stream 7 Channel 4
            hdma_tx_->Instance = DMA2_Stream7;
            hdma_tx_->Init.Channel = DMA_CHANNEL_4;
            hdma_tx_->Init.Direction = DMA_MEMORY_TO_PERIPH;
            hdma_tx_->Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_tx_->Init.MemInc = DMA_MINC_ENABLE;
            hdma_tx_->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_tx_->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_tx_->Init.Mode = DMA_NORMAL;
            hdma_tx_->Init.Priority = DMA_PRIORITY_LOW;
            hdma_tx_->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            
            if (HAL_DMA_Init(hdma_tx_) != HAL_OK) {
                return SerialStatus::ERROR;
            }
            __HAL_LINKDMA(huart_, hdmatx, *hdma_tx_);
            
            // DMA RX: DMA2 Stream 2 Channel 4
            hdma_rx_->Instance = DMA2_Stream2;
            hdma_rx_->Init.Channel = DMA_CHANNEL_4;
            hdma_rx_->Init.Direction = DMA_PERIPH_TO_MEMORY;
            hdma_rx_->Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_rx_->Init.MemInc = DMA_MINC_ENABLE;
            hdma_rx_->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_rx_->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_rx_->Init.Mode = DMA_CIRCULAR;  // 循环模式
            hdma_rx_->Init.Priority = DMA_PRIORITY_HIGH;
            hdma_rx_->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            
            if (HAL_DMA_Init(hdma_rx_) != HAL_OK) {
                return SerialStatus::ERROR;
            }
            __HAL_LINKDMA(huart_, hdmarx, *hdma_rx_);
            
            // DMA中断优先级
            HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 1);
            HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
            HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
            HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
            break;
        }
        
        case SerialType::UART6: {
            // DMA TX: DMA2 Stream 6 Channel 5
            hdma_tx_->Instance = DMA2_Stream6;
            hdma_tx_->Init.Channel = DMA_CHANNEL_5;
            hdma_tx_->Init.Direction = DMA_MEMORY_TO_PERIPH;
            hdma_tx_->Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_tx_->Init.MemInc = DMA_MINC_ENABLE;
            hdma_tx_->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_tx_->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_tx_->Init.Mode = DMA_NORMAL;
            hdma_tx_->Init.Priority = DMA_PRIORITY_LOW;
            hdma_tx_->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            
            if (HAL_DMA_Init(hdma_tx_) != HAL_OK) {
                return SerialStatus::ERROR;
            }
            __HAL_LINKDMA(huart_, hdmatx, *hdma_tx_);
            
            // DMA RX: DMA2 Stream 1 Channel 5
            hdma_rx_->Instance = DMA2_Stream1;
            hdma_rx_->Init.Channel = DMA_CHANNEL_5;
            hdma_rx_->Init.Direction = DMA_PERIPH_TO_MEMORY;
            hdma_rx_->Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_rx_->Init.MemInc = DMA_MINC_ENABLE;
            hdma_rx_->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_rx_->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_rx_->Init.Mode = DMA_CIRCULAR;
            hdma_rx_->Init.Priority = DMA_PRIORITY_HIGH;
            hdma_rx_->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            
            if (HAL_DMA_Init(hdma_rx_) != HAL_OK) {
                return SerialStatus::ERROR;
            }
            __HAL_LINKDMA(huart_, hdmarx, *hdma_rx_);
            
            // DMA中断优先级
            HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 5, 1);
            HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
            HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
            HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
            break;
        }
        
        default:
            return SerialStatus::ERROR;
    }
    
    return SerialStatus::OK;
}

/**
 * @brief 初始化NVIC中断
 */
SerialStatus SerialPort::initNVIC() {
    IRQn_Type irqn;
    
    switch (type_) {
        case SerialType::UART1:
            irqn = USART1_IRQn;
            break;
        case SerialType::UART2:
            irqn = USART2_IRQn;
            break;
        case SerialType::UART3:
            irqn = USART3_IRQn;
            break;
        case SerialType::UART6:
            irqn = USART6_IRQn;
            break;
        default:
            return SerialStatus::ERROR;
    }
    
    // 设置UART中断优先级
    HAL_NVIC_SetPriority(irqn, 5, 0);
    HAL_NVIC_EnableIRQ(irqn);
    
    return SerialStatus::OK;
}

/**
 * @brief 使能IDLE空闲中断
 */
void SerialPort::enableIdleInterrupt() {
    if (huart_) {
        __HAL_UART_ENABLE_IT(huart_, UART_IT_IDLE);
    }
}

/**
 * @brief 发送数据
 */
SerialStatus SerialPort::send(const uint8_t* data, size_t length, uint32_t timeout) {
    if (!initialized_ || !huart_ || !data || length == 0) {
        return SerialStatus::ERROR;
    }
    
    HAL_StatusTypeDef status;
    
    switch (mode_) {
        case SerialMode::POLLING:
            status = HAL_UART_Transmit(huart_, (uint8_t*)data, length, timeout);
            break;
            
        case SerialMode::INTERRUPT:
            status = HAL_UART_Transmit_IT(huart_, (uint8_t*)data, length);
            break;
            
        case SerialMode::DMA:
        case SerialMode::DMA_IDLE:
            status = HAL_UART_Transmit_DMA(huart_, (uint8_t*)data, length);
            break;
            
        default:
            return SerialStatus::ERROR;
    }
    
    if (status == HAL_OK) {
        return SerialStatus::OK;
    } else if (status == HAL_BUSY) {
        return SerialStatus::BUSY;
    } else if (status == HAL_TIMEOUT) {
        return SerialStatus::TIMEOUT;
    } else {
        return SerialStatus::ERROR;
    }
}

/**
 * @brief 发送字符串
 */
SerialStatus SerialPort::sendString(const char* str, uint32_t timeout) {
    if (!str) {
        return SerialStatus::ERROR;
    }
    return send((const uint8_t*)str, strlen(str), timeout);
}

/**
 * @brief 接收数据（阻塞模式）
 */
SerialStatus SerialPort::receive(uint8_t* data, size_t length, uint32_t timeout) {
    if (!initialized_ || !huart_ || !data || length == 0) {
        return SerialStatus::ERROR;
    }
    if (mode_ != SerialMode::POLLING) {
        return SerialStatus::ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_UART_Receive(huart_, data, length, timeout);
    
    if (status == HAL_OK) {
        return SerialStatus::OK;
    } else if (status == HAL_TIMEOUT) {
        return SerialStatus::TIMEOUT;
    } else {
        return SerialStatus::ERROR;
    }
}

/**
 * @brief 从环形缓冲区读取数据
 */
size_t SerialPort::read(uint8_t* data, size_t maxLength) {
    return readFromRingBuffer(data, maxLength);
}

/**
 * @brief 获取可用数据量
 */
size_t SerialPort::available() const {
    size_t head;
    size_t tail;
    __disable_irq();
    head = rxHead_;
    tail = rxTail_;
    __enable_irq();
    if (head >= tail) {
        return head - tail;
    } else {
        return (RX_BUFFER_SIZE * 2) - tail + head;
    }
}

/**
 * @brief 清空接收缓冲区
 */
void SerialPort::flush() {
    __disable_irq();
    rxHead_ = 0;
    rxTail_ = 0;
    __enable_irq();
    memset(rxRingBuffer_, 0, RX_BUFFER_SIZE * 2);
}

/**
 * @brief 设置接收回调函数
 */
void SerialPort::setRxCallback(SerialRxCallback callback) {
    rxCallback_ = callback;
}

/* HAL Library Callbacks -------------------------------------------------*/
SerialStatus SerialPort::startReceive() {
    if (!initialized_ || !huart_) {
        return SerialStatus::ERROR;
    }
    
    HAL_StatusTypeDef status;
    
    switch (mode_) {
        case SerialMode::INTERRUPT:
            status = HAL_UART_Receive_IT(huart_, rxBuffer_, 1);
            break;
            
        case SerialMode::DMA:
        case SerialMode::DMA_IDLE:
            status = HAL_UART_Receive_DMA(huart_, rxBuffer_, RX_BUFFER_SIZE);
            break;
            
        default:
            return SerialStatus::OK;  // POLLING模式不需要启动
    }
    
    return (status == HAL_OK) ? SerialStatus::OK : SerialStatus::ERROR;
}

/**
 * @brief 停止接收
 */
SerialStatus SerialPort::stopReceive() {
    if (!initialized_ || !huart_) {
        return SerialStatus::ERROR;
    }
    
    if (mode_ == SerialMode::DMA || mode_ == SerialMode::DMA_IDLE) {
        HAL_UART_DMAStop(huart_);
    } else if (mode_ == SerialMode::INTERRUPT) {
        HAL_UART_AbortReceive_IT(huart_);
    }
    
    return SerialStatus::OK;
}

/**
 * @brief 检查是否忙碌
 */
bool SerialPort::isBusy() const {
    if (!huart_) {
        return false;
    }
    return (huart_->gState != HAL_UART_STATE_READY);
}

/**
 * @brief 处理UART中断
 */
void SerialPort::handleIRQ() {
    if (!huart_) {
        return;
    }
    
    // 检查IDLE中断
    if (__HAL_UART_GET_FLAG(huart_, UART_FLAG_IDLE)) {
        handleIdleInterrupt();
    }
    
    // 调用HAL库的中断处理
    HAL_UART_IRQHandler(huart_);
}

/**
 * @brief 处理IDLE空闲中断
 */
void SerialPort::handleIdleInterrupt() {
    if (!huart_ || !hdma_rx_) {
        return;
    }
    
    // 清除IDLE标志
    __HAL_UART_CLEAR_IDLEFLAG(huart_);
    
    // 停止DMA传输
    HAL_UART_DMAStop(huart_);
    
    // 计算接收到的数据长度
    uint32_t dataLength = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hdma_rx_);
    
    if (dataLength > 0) {
        // 写入环形缓冲区
        writeToRingBuffer(rxBuffer_, dataLength);
        
        // 调用回调函数
        if (rxCallback_) {
            rxCallback_(rxBuffer_, dataLength);
        }
    }
    
    // 重新启动DMA接收
    HAL_UART_Receive_DMA(huart_, rxBuffer_, RX_BUFFER_SIZE);
}

/**
 * @brief 接收完成回调
 */
void SerialPort::rxCompleteCallback() {
    if (mode_ == SerialMode::INTERRUPT) {
        writeToRingBuffer(rxBuffer_, 1);
        if (rxCallback_) {
            rxCallback_(rxBuffer_, 1);
        }
        HAL_UART_Receive_IT(huart_, rxBuffer_, 1);
    } else if (mode_ == SerialMode::DMA) {
        const size_t half = RX_BUFFER_SIZE / 2;
        writeToRingBuffer(rxBuffer_ + half, half);
        if (rxCallback_) {
            rxCallback_(rxBuffer_ + half, half);
        }
    }
}

void SerialPort::rxHalfCompleteCallback() {
    if (mode_ == SerialMode::DMA) {
        const size_t half = RX_BUFFER_SIZE / 2;
        writeToRingBuffer(rxBuffer_, half);
        if (rxCallback_) {
            rxCallback_(rxBuffer_, half);
        }
    }
}

/**
 * @brief 发送完成回调
 */
void SerialPort::txCompleteCallback() {
    // 可以在这里添加发送完成的处理逻辑
}

/**
 * @brief 错误回调
 */
void SerialPort::errorCallback() {
    // 错误处理：重新启动接收
    if (mode_ == SerialMode::DMA || mode_ == SerialMode::DMA_IDLE) {
        HAL_UART_Receive_DMA(huart_, rxBuffer_, RX_BUFFER_SIZE);
    } else if (mode_ == SerialMode::INTERRUPT) {
        HAL_UART_Receive_IT(huart_, rxBuffer_, 1);
    }
}

/**
 * @brief 写入环形缓冲区
 */
size_t SerialPort::writeToRingBuffer(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return 0;
    }
    
    size_t freeSpace = getRingBufferFreeSpace();
    if (length > freeSpace) {
        length = freeSpace;  // 限制写入长度
    }
    
    for (size_t i = 0; i < length; i++) {
        rxRingBuffer_[rxHead_] = data[i];
        rxHead_ = (rxHead_ + 1) % (RX_BUFFER_SIZE * 2);
    }
    
    return length;
}

/**
 * @brief 从环形缓冲区读取数据
 */
size_t SerialPort::readFromRingBuffer(uint8_t* data, size_t maxLength) {
    if (!data || maxLength == 0) {
        return 0;
    }
    
    __disable_irq();
    size_t head = rxHead_;
    size_t tail = rxTail_;
    size_t availableData;
    if (head >= tail) {
        availableData = head - tail;
    } else {
        availableData = (RX_BUFFER_SIZE * 2) - tail + head;
    }
    __enable_irq();
    if (maxLength > availableData) {
        maxLength = availableData;
    }
    
    __disable_irq();
    for (size_t i = 0; i < maxLength; i++) {
        data[i] = rxRingBuffer_[rxTail_];
        rxTail_ = (rxTail_ + 1) % (RX_BUFFER_SIZE * 2);
    }
    __enable_irq();
    
    return maxLength;
}

/**
 * @brief 获取环形缓冲区可用空间
 */
size_t SerialPort::getRingBufferFreeSpace() const {
    return (RX_BUFFER_SIZE * 2) - available() - 1;
}

// ============================================================================
// 全局访问函数（供中断服务函数使用）
// ============================================================================

extern "C" {

/**
 * @brief 获取全局SerialPort实例
 */
SerialPort* getSerialPort(int index) {
    if (index >= 0 && index < 4) {
        return g_serialPorts[index];
    }
    return nullptr;
}

/**
 * @brief C风格包装函数 - 调用handleIRQ
 */
void SerialPort_handleIRQ(SerialPort* port) {
    if (port) {
        port->handleIRQ();
    }
}

/**
 * @brief C风格包装函数 - 调用rxCompleteCallback
 */
void SerialPort_rxCompleteCallback(SerialPort* port) {
    if (port) {
        port->rxCompleteCallback();
    }
}

/**
 * @brief C风格包装函数 - 调用rxHalfCompleteCallback
 */
void SerialPort_rxHalfCpltCallback(SerialPort* port) {
    if (port) {
        port->rxHalfCompleteCallback();
    }
}

/**
 * @brief C风格包装函数 - 调用txCompleteCallback
 */
void SerialPort_txCompleteCallback(SerialPort* port) {
    if (port) {
        port->txCompleteCallback();
    }
}

/**
 * @brief C风格包装函数 - 调用errorCallback
 */
void SerialPort_errorCallback(SerialPort* port) {
    if (port) {
        port->errorCallback();
    }
}

/**
 * @brief C风格包装函数 - 获取UART句柄
 */
UART_HandleTypeDef* SerialPort_getUartHandle(SerialPort* port) {
    if (port) {
        return port->huart_;
    }
    return nullptr;
}

/**
 * @brief C风格包装函数 - 获取DMA TX句柄
 */
DMA_HandleTypeDef* SerialPort_getDmaTxHandle(SerialPort* port) {
    if (port) {
        return port->hdma_tx_;
    }
    return nullptr;
}

/**
 * @brief C风格包装函数 - 获取DMA RX句柄
 */
DMA_HandleTypeDef* SerialPort_getDmaRxHandle(SerialPort* port) {
    if (port) {
        return port->hdma_rx_;
    }
    return nullptr;
}

} // extern "C"
