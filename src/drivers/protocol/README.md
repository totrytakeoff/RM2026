# 串口通讯协议驱动

## 📁 目录结构

```
protocol/
├── serial_port.hpp          # 串口类头文件
├── serial_port.cpp          # 串口类实现
├── usb_port.hpp             # USB类头文件
├── usb_port.cpp             # USB类实现
├── can_comm.hpp             # CAN通讯类头文件
├── can_comm.cpp             # CAN通讯类实现
└── README.md                # 本文件
```

## 🚀 快速开始

### 最简单的例子（3行代码）

```cpp
#include "serial_port.hpp"

SerialPort uart1(SerialType::UART1);
uart1.init();                              // 初始化
uart1.sendString("Hello World!\r\n");      // 发送
```

### 接收数据（推荐方式）

```cpp
uart1.setRxCallback([](uint8_t* data, size_t len) {
    // 数据接收完成后自动调用
    uart1.send(data, len);  // 回显
});
```

## ✨ 核心特性

- ✅ **多串口支持**: UART1, UART6 (可扩展UART2/3)
- ✅ **4种工作模式**: 阻塞/中断/DMA/DMA+IDLE
- ✅ **IDLE空闲中断**: 自动处理不定长数据
- ✅ **环形缓冲区**: 2KB缓冲，防止数据丢失
- ✅ **回调机制**: 接收完成自动通知
- ✅ **零拷贝**: DMA直接搬运数据
- ✅ **低CPU占用**: < 1% (DMA模式)
- ✅ **易用接口**: 简洁的C++风格API

## 📖 文档

详细文档请查看：

- **使用指南**: `/docs/protocol/SERIAL_PORT_USAGE.md`
- **技术细节**: `/docs/protocol/SERIAL_PORT_TECHNICAL.md`
- **示例代码**: `/src/test/protocol/serial_demo.cpp`

## 🎯 工作模式选择

| 模式 | 适用场景 | CPU占用 | 推荐度 |
|------|----------|---------|--------|
| POLLING | 简单测试 | 100% | ⭐ |
| INTERRUPT | 中频通信 | 5-10% | ⭐⭐ |
| DMA | 固定长度 | < 1% | ⭐⭐⭐ |
| **DMA_IDLE** | **通用场景** | **< 1%** | **⭐⭐⭐⭐⭐** |

## 💡 使用示例

### 示例1: 简单回显
```cpp
SerialPort uart1(SerialType::UART1);
uart1.init();
uart1.setRxCallback([](uint8_t* data, size_t len) {
    uart1.send(data, len);  // 回显
});
```

### 示例2: 命令解析
```cpp
uint8_t buffer[256];

void loop() {
    if (uart1.available() > 0) {
        size_t len = uart1.read(buffer, sizeof(buffer));
        
        if (buffer[0] == 'A') {
            uart1.sendString("Command A\r\n");
        }
    }
}
```

### 示例3: 格式化输出
```cpp
char msg[128];
snprintf(msg, sizeof(msg), "Temp: %.1f°C\r\n", 25.6f);
uart1.sendString(msg);
```

### 示例4: 多串口
```cpp
SerialPort uart1(SerialType::UART1);  // 调试用
SerialPort uart6(SerialType::UART6);  // 裁判系统

uart1.init();  // 115200
uart6.init();  // 115200
```

## 🔧 配置选项

```cpp
SerialConfig config;
config.baudrate = 115200;                // 波特率
config.wordLength = UART_WORDLENGTH_8B;  // 8位数据
config.stopBits = UART_STOPBITS_1;       // 1位停止位
config.parity = UART_PARITY_NONE;        // 无校验
config.mode = SerialMode::DMA_IDLE;      // DMA+IDLE模式

uart1.init(config);
```

## 🔌 硬件连接

### UART1 (外壳丝印UART2)
- **TX**: PA9
- **RX**: PB7
- **接口**: 4-pin (RXD, TXD, GND, 5V)

### UART6 (外壳丝印UART1)
- **TX**: PG14
- **RX**: PG9
- **接口**: 3-pin (GND, TXD, RXD)

⚠️ **注意**: 
- 外壳丝印与实际UART编号不对应！
- UART6与裁判系统通信时需要交叉TX/RX

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 最大波特率 | 921600 bps |
| 最小延迟 | < 1ms |
| CPU占用 | < 1% (DMA模式) |
| 缓冲区大小 | 2KB |
| 内存占用 | ~3.3KB/实例 |

## 🐛 常见问题

### Q: 接收不到数据？
**A**: 检查波特率、TX/RX连接、电平匹配

### Q: 数据丢失？
**A**: 使用DMA_IDLE模式，及时读取缓冲区

### Q: 如何调试？
**A**: 使用USB转TTL + 串口助手，添加LED指示

### Q: 支持printf吗？
**A**: 可以，重定向`_write()`函数即可

## 🔄 USB-CDC支持

✅ **已实现！** USB虚拟串口类`USBPort`已完成：

```cpp
#include "usb_port.hpp"

USBPort usb;
usb.init();
usb.sendString("Hello USB!\r\n");
```

详细文档：`/docs/protocol/USB_PORT_USAGE.md`

## 📝 API速查

### 初始化
- `init(config)` - 初始化串口
- `deinit()` - 反初始化

### 发送
- `send(data, len)` - 发送字节数组
- `sendString(str)` - 发送字符串

### 接收
- `receive(data, len)` - 阻塞接收
- `read(data, maxLen)` - 非阻塞读取
- `available()` - 获取可用数据量
- `flush()` - 清空缓冲区

### 回调
- `setRxCallback(callback)` - 设置接收回调

### 控制
- `startReceive()` - 启动接收
- `stopReceive()` - 停止接收
- `isInitialized()` - 检查初始化状态
- `isBusy()` - 检查忙碌状态

## 🎓 学习路径

1. **入门**: 阅读 `SERIAL_PORT_USAGE.md`
2. **实践**: 运行 `serial_demo.cpp` 中的示例
3. **进阶**: 阅读 `SERIAL_PORT_TECHNICAL.md`
4. **应用**: 集成到自己的项目

## 🔗 相关资源

- [STM32F407 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020.pdf)
- [HAL库文档](https://www.st.com/resource/en/user_manual/dm00105879.pdf)
- [DMA原理](https://www.st.com/resource/en/application_note/dm00046011.pdf)

## 📅 版本历史

### v1.0.0 (2024-12-02)
- ✅ 初始版本发布
- ✅ 支持UART1和UART6
- ✅ 实现4种工作模式
- ✅ DMA + IDLE空闲中断
- ✅ 环形缓冲区
- ✅ 完整文档和示例

### 未来计划
- 🔜 USB-CDC虚拟串口
- 🔜 UART2/UART3支持
- 🔜 DMA双缓冲模式
- 🔜 自动波特率检测
- 🔜 数据协议解析器

## 👨‍💻 贡献

欢迎提交Issue和Pull Request！

## 📄 许可

本项目遵循项目根目录的许可协议。

---

**Happy Coding! 🚀**

*如有问题，请查阅详细文档或提交Issue*
