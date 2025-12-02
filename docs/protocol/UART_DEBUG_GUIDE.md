# UART 乱码问题调试指南

## 🐛 问题现象

串口监视器显示乱码：
```
␞f���`␆��fx␀����f␆~������~␆�␀�␘␆�~������␀����␆���␀��␆~␀␘�f����
```

## 🔍 问题原因分析

### 1. 波特率不匹配（最常见）

**症状**: 收到乱码，但有规律的字符出现

**原因**: 
- 发送端波特率 ≠ 接收端波特率
- 时钟配置错误导致实际波特率不对

**检查方法**:
```cpp
// 确认初始化时的波特率
SerialConfig config;
config.baudrate = 115200;  // 必须与串口监视器一致
uart6.init(config);
```

### 2. 时钟配置错误

**UART6 挂载在 APB2 总线上**，需要确保APB2时钟正确：

```cpp
// 正确的时钟配置
RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  // APB2 = 84MHz
```

**计算公式**:
```
实际波特率 = APB2_Clock / (16 * USARTDIV)
115200 = 84000000 / (16 * USARTDIV)
USARTDIV ≈ 45.57
```

### 3. GPIO配置错误

**UART6 引脚**:
- TX: PG14 (复用功能 AF8)
- RX: PG9 (复用功能 AF8)

检查 `serial_port.cpp` 中的配置：
```cpp
GPIO_InitStruct.Alternate = GPIO_AF8_USART6;  // 必须是AF8
```

### 4. 中断服务函数冲突

检查是否有重复定义：
- `stm32f4xx_it.c` 中不应该有 `USART6_IRQHandler`
- `serial_port.cpp` 中已经定义了

## 🔧 调试步骤

### 步骤1: 验证硬件连接

```
USB转TTL          C板UART6 (3针接口)
┌──────────┐     ┌──────────┐
│   GND    │────>│   GND    │ (Pin 1)
│   TXD    │────>│   RXD    │ (Pin 3, PG9)
│   RXD    │<────│   TXD    │ (Pin 2, PG14)
└──────────┘     └──────────┘
```

⚠️ **注意**: TX和RX要交叉连接！

### 步骤2: 使用简化测试程序

我已经创建了 `uart6_simple_test.cpp`，使用这个程序测试：

```bash
# 修改 platformio.ini
build_src_filter =
    +<*>
    -<main.cpp>
    -<test/*>
    +<test/protocol/uart6_simple_test.cpp>

# 编译上传
pio run -t upload

# 监视串口
pio device monitor -b 115200
```

### 步骤3: 逐步排查

#### 3.1 测试发送功能

如果您能看到启动消息（即使是乱码），说明：
- ✅ UART初始化成功
- ✅ DMA工作正常
- ❌ 波特率不对

**解决**: 尝试不同的波特率
```bash
# 尝试9600
pio device monitor -b 9600

# 尝试57600
pio device monitor -b 57600

# 尝试230400
pio device monitor -b 230400
```

#### 3.2 测试接收功能

发送任意字符，观察：
- LED是否闪烁？（说明接收中断触发）
- 是否有回显？（说明回调函数执行）

### 步骤4: 添加调试输出

修改 `serial_port.cpp`，添加调试信息：

```cpp
SerialStatus SerialPort::initUART(const SerialConfig& config) {
    // ... 原有代码 ...
    
    // 添加调试：打印实际配置
    // 注意：这需要另一个串口或LED指示
    
    return SerialStatus::OK;
}
```

## 🎯 常见问题和解决方案

### 问题1: 启动时收到一串乱码，然后就没反应了

**原因**: 
- 启动消息发送成功（虽然乱码）
- 但接收中断没有正常工作

**解决**:
1. 检查中断服务函数是否被调用
2. 检查DMA配置是否正确
3. 确认IDLE中断是否使能

### 问题2: 完全没有输出

**原因**:
- UART初始化失败
- GPIO配置错误
- 硬件连接问题

**解决**:
```cpp
// 添加错误指示
SerialStatus status = uart6.init(config);
if (status != SerialStatus::OK) {
    // LED快闪表示错误
    while(1) {
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_Delay(100);
    }
}
```

### 问题3: 只能发送，不能接收

**原因**:
- RX引脚配置错误
- DMA RX配置错误
- 中断优先级问题

**检查**:
```cpp
// 在回调中添加LED指示
void onUart6Receive(uint8_t* data, size_t length) {
    HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);  // 收到数据时闪烁
    uart6.send(data, length);
}
```

## 🔬 高级调试技巧

### 使用逻辑分析仪

如果有逻辑分析仪，可以直接测量波特率：

1. 连接到TX引脚（PG14）
2. 发送已知数据（如 "A"）
3. 测量一个字符的时间
4. 计算实际波特率 = 1 / (bit_time * 10)

### 使用示波器

观察TX引脚波形：
- 空闲时应该是高电平
- 起始位是低电平
- 数据位按LSB顺序
- 停止位是高电平

### 计算实际波特率

```cpp
// 在代码中计算
uint32_t apb2_clock = HAL_RCC_GetPCLK2Freq();
uint32_t usartdiv = huart6.Instance->BRR;
uint32_t actual_baudrate = apb2_clock / (16 * usartdiv);

// 通过另一个串口或LED闪烁次数输出
```

## 📝 检查清单

在提问前，请确认：

- [ ] 硬件连接正确（TX/RX交叉）
- [ ] GND已连接
- [ ] 波特率设置一致（115200）
- [ ] 时钟配置正确（APB2 = 84MHz）
- [ ] GPIO复用功能正确（AF8）
- [ ] 没有中断服务函数冲突
- [ ] LED指示灯工作正常
- [ ] 使用了简化测试程序

## 🆘 如果还是不行

### 最简单的测试：阻塞模式

修改为阻塞模式测试基本功能：

```cpp
SerialConfig config;
config.mode = SerialMode::POLLING;  // 阻塞模式
uart6.init(config);

while(1) {
    uart6.sendString("Test\r\n");
    HAL_Delay(1000);
}
```

如果阻塞模式能正常发送，说明：
- ✅ GPIO配置正确
- ✅ 时钟配置正确
- ✅ 波特率正确
- ❌ DMA或中断配置有问题

### 回退到HAL库原生API测试

```cpp
UART_HandleTypeDef huart6;

void test_hal_uart() {
    // 手动初始化
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();
    
    // GPIO配置
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    
    // UART配置
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart6);
    
    // 测试发送
    while(1) {
        HAL_UART_Transmit(&huart6, (uint8_t*)"Test\r\n", 6, 100);
        HAL_Delay(1000);
    }
}
```

如果这个能工作，说明SerialPort类有问题。
如果这个也不工作，说明是硬件或时钟问题。

## 💡 我的建议

基于您的情况（收到乱码），我强烈建议：

1. **先用简化测试程序** (`uart6_simple_test.cpp`)
2. **尝试不同波特率**（9600, 57600, 115200）
3. **检查硬件连接**（用万用表测通断）
4. **观察LED指示**（确认程序在运行）

---

**最后更新**: 2025-12-03  
**问题反馈**: 如果按照上述步骤还是不行，请提供：
1. 使用的USB转TTL型号
2. LED指示灯状态
3. 尝试过的波特率
4. 逻辑分析仪/示波器截图（如果有）
