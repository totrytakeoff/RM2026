# SerialPort 快速入门指南

## 🎯 5分钟上手

### 步骤1: 包含头文件
```cpp
#include "serial_port.hpp"
```

### 步骤2: 创建串口对象
```cpp
SerialPort uart1(SerialType::UART1);
```

### 步骤3: 初始化
```cpp
uart1.init();  // 使用默认配置：115200, 8N1, DMA+IDLE
```

### 步骤4: 发送数据
```cpp
uart1.sendString("Hello World!\r\n");
```

### 步骤5: 接收数据
```cpp
uart1.setRxCallback([](uint8_t* data, size_t len) {
    // 数据接收完成后自动调用
    uart1.send(data, len);  // 回显
});
```

**完成！** 🎉

---

## 📝 完整示例程序

### 最简单的回显程序

```cpp
#include "main.h"
#include "serial_port.hpp"

SerialPort uart1(SerialType::UART1);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    // 初始化串口
    uart1.init();
    
    // 设置回调
    uart1.setRxCallback([](uint8_t* data, size_t len) {
        uart1.send(data, len);  // 回显
    });
    
    // 发送欢迎消息
    uart1.sendString("Echo Server Ready!\r\n");
    
    while (1) {
        HAL_Delay(100);
    }
}
```

---

## 🔧 常用配置

### 1. 修改波特率

```cpp
SerialConfig config;
config.baudrate = 9600;  // 改为9600
uart1.init(config);
```

### 2. 使用不同的串口

```cpp
SerialPort uart6(SerialType::UART6);  // 使用UART6
uart6.init();
```

### 3. 同时使用多个串口

```cpp
SerialPort uart1(SerialType::UART1);
SerialPort uart6(SerialType::UART6);

uart1.init();
uart6.init();

uart1.sendString("UART1\r\n");
uart6.sendString("UART6\r\n");
```

---

## 💡 实用代码片段

### 发送格式化字符串

```cpp
char buffer[128];
float temperature = 25.6f;
int speed = 1234;

snprintf(buffer, sizeof(buffer), 
         "Temp: %.1f°C, Speed: %d RPM\r\n", 
         temperature, speed);

uart1.sendString(buffer);
```

### 发送二进制数据

```cpp
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
uart1.send(data, sizeof(data));
```

### 从缓冲区读取数据

```cpp
uint8_t buffer[256];

if (uart1.available() > 0) {
    size_t len = uart1.read(buffer, sizeof(buffer));
    // 处理数据
}
```

### 命令解析

```cpp
uart1.setRxCallback([](uint8_t* data, size_t len) {
    if (len > 0) {
        switch (data[0]) {
            case 'A':
                uart1.sendString("Command A\r\n");
                break;
            case 'B':
                uart1.sendString("Command B\r\n");
                break;
            default:
                uart1.sendString("Unknown command\r\n");
                break;
        }
    }
});
```

---

## 🔌 硬件连接

### 使用USB转TTL模块

```
USB转TTL模块          C板UART1
┌──────────┐         ┌──────────┐
│   VCC    │         │   5V     │ (可选，供电)
│   GND    │────────>│   GND    │
│   TXD    │────────>│   RXD    │ (PB7)
│   RXD    │<────────│   TXD    │ (PA9)
└──────────┘         └──────────┘
```

⚠️ **注意**: TX和RX要交叉连接！

### 使用串口助手

推荐工具：
- **Windows**: SSCOM, 串口调试助手
- **macOS**: CoolTerm, Serial
- **Linux**: minicom, screen

配置：
- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验位: 无
- 流控: 无

---

## 🐛 故障排查

### 问题1: 接收不到数据

**检查清单**:
- [ ] 波特率是否匹配？
- [ ] TX/RX是否交叉连接？
- [ ] GND是否连接？
- [ ] 电平是否匹配（3.3V/5V）？
- [ ] 是否调用了`init()`？
- [ ] 是否设置了回调或调用`read()`？

**调试步骤**:
```cpp
// 1. 先测试发送
uart1.sendString("Test\r\n");

// 2. 检查初始化状态
if (uart1.isInitialized()) {
    uart1.sendString("Init OK\r\n");
}

// 3. 添加LED指示
uart1.setRxCallback([](uint8_t* data, size_t len) {
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
    uart1.send(data, len);
});
```

### 问题2: 数据乱码

**原因**: 波特率不匹配

**解决**:
```cpp
// 确保两端波特率一致
SerialConfig config;
config.baudrate = 115200;  // 与串口助手一致
uart1.init(config);
```

### 问题3: 数据丢失

**原因**: 缓冲区溢出或处理不及时

**解决**:
```cpp
// 1. 及时读取数据
if (uart1.available() > 0) {
    uint8_t buffer[256];
    uart1.read(buffer, sizeof(buffer));
}

// 2. 在回调中快速处理
uart1.setRxCallback([](uint8_t* data, size_t len) {
    // 快速复制数据，不要做耗时操作
    memcpy(userBuffer, data, len);
    dataReady = true;
});
```

---

## 📚 进阶学习

### 1. 理解工作模式

| 模式 | 何时使用 |
|------|----------|
| POLLING | 简单测试、调试 |
| INTERRUPT | 低频通信 |
| DMA | 高频固定长度 |
| **DMA_IDLE** | **推荐！通用场景** |

### 2. 实现数据协议

```cpp
// 协议格式: [0xAA] [长度] [数据...] [校验和]
void parseProtocol(uint8_t* data, size_t len) {
    if (len < 3 || data[0] != 0xAA) return;
    
    uint8_t dataLen = data[1];
    uint8_t checksum = 0;
    
    for (int i = 0; i < dataLen + 2; i++) {
        checksum += data[i];
    }
    
    if (checksum == data[dataLen + 2]) {
        // 数据有效
        processData(&data[2], dataLen);
    }
}

uart1.setRxCallback(parseProtocol);
```

### 3. 实现printf重定向

```cpp
// 在syscalls.c或main.cpp中
extern SerialPort uart1;

extern "C" int _write(int file, char *ptr, int len) {
    uart1.send((uint8_t*)ptr, len);
    return len;
}

// 然后就可以使用printf了
printf("Hello World! Value = %d\r\n", 123);
```

---

## 🎓 学习路径

### 新手路线
1. ✅ 运行简单回显程序
2. ✅ 尝试修改波特率
3. ✅ 实现命令解析
4. ✅ 添加数据协议

### 进阶路线
1. 📖 阅读技术文档了解DMA原理
2. 🔧 实现自定义协议解析器
3. 🚀 优化性能和内存使用
4. 🔗 集成到实际项目

---

## 📖 相关文档

- **详细使用指南**: `SERIAL_PORT_USAGE.md`
- **技术实现细节**: `SERIAL_PORT_TECHNICAL.md`
- **完整示例代码**: `../src/test/protocol/serial_demo.cpp`
- **简单测试程序**: `../src/test/protocol/simple_uart_test.cpp`

---

## ❓ 常见问题

**Q: 可以同时使用多个串口吗？**  
A: 可以！每个串口独立工作。

**Q: 支持USB虚拟串口吗？**  
A: 接口已预留，后续版本实现。

**Q: 如何提高传输速度？**  
A: 使用DMA_IDLE模式，提高波特率（最高921600）。

**Q: 数据最大长度是多少？**  
A: 单次发送无限制，接收缓冲区2KB。

**Q: 如何调试串口问题？**  
A: 使用USB转TTL + 串口助手，添加LED指示。

---

## 🆘 获取帮助

如果遇到问题：
1. 查看详细文档
2. 运行测试程序
3. 检查硬件连接
4. 提交Issue

---

**祝你使用愉快！** 🎉

*最后更新: 2024-12-02*
