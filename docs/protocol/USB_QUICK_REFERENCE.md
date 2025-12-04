# USB通讯快速参考

## 🚀 3行代码开始

```cpp
USBPort usb;
usb.init();
usb.sendString("Hello!\r\n");
```

## 📤 发送

```cpp
// 字符串
usb.sendString("Hello\r\n");

// 字节数组
uint8_t data[] = {0x01, 0x02, 0x03};
usb.send(data, 3);

// 格式化
usb.printf("Value: %d\r\n", 123);
```

## 📥 接收

```cpp
// 回调方式（推荐）
usb.setRxCallback([](uint8_t* data, size_t len) {
    usb.send(data, len);  // 回显
});

// 轮询方式
if (usb.available() > 0) {
    uint8_t buf[128];
    size_t len = usb.read(buf, sizeof(buf));
}
```

## 🔌 连接管理

```cpp
// 检查连接
if (usb.isConnected()) { }

// 等待连接
usb.waitForConnection(5000);

// 连接回调
usb.setConnectCallback([](USBConnectionState state) {
    if (state == USBConnectionState::CONNECTED) {
        // 连接成功
    }
});
```

## 🔍 状态查询

```cpp
usb.isInitialized();  // 是否已初始化
usb.isConnected();     // 是否已连接
usb.isBusy();          // 是否正在发送
usb.available();       // 可读数据量
```

## ⚙️ 系统集成

### 时钟配置（关键！）

```cpp
RCC_OscInitStruct.PLL.PLLM = 6;   // 12MHz/6 = 2MHz
RCC_OscInitStruct.PLL.PLLN = 168; // 2MHz*168 = 336MHz
RCC_OscInitStruct.PLL.PLLQ = 7;   // 336MHz/7 = 48MHz ✓
```

### 模式切换

```c
// usbd_cdc_if.c
#define USE_USB_PORT_CLASS 1  // 1=USBPort类，0=原始命令
```

## 🐛 常见问题

| 问题 | 解决方法 |
|------|----------|
| PC无法识别 | 检查USB时钟=48MHz |
| 发送失败 | 检查`isConnected()` |
| 接收不到 | 设置`setRxCallback()` |

## 📁 文件位置

```
src/drivers/protocol/usb_port.hpp    # 头文件
src/drivers/protocol/usb_port.cpp    # 实现
src/test/usb_demo.cpp                # 示例
docs/protocol/USB_PORT_USAGE.md      # 详细文档
```

## 💡 完整示例

```cpp
#include "usb_port.hpp"

USBPort usb;

void onReceive(uint8_t* data, size_t len) {
    usb.send(data, len);  // 回显
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    usb.init();
    usb.setRxCallback(onReceive);
    
    while(1) {
        HAL_Delay(10);
    }
}
```

---

**详细文档**: `/docs/protocol/USB_PORT_USAGE.md`
