# USB LED控制演示程序详细说明

## 概述

这是一个完整的STM32 USB LED控制演示程序，展示了如何通过USB CDC（虚拟串口）控制开发板上的RGB LED。

## 文件结构

### 核心文件
```
src/
├── usb_device.c        # USB设备核心初始化
├── usbd_cdc_if.c       # USB CDC接口实现（包含命令处理逻辑）
├── usbd_desc.c         # USB描述符实现
├── usb_led_demo.c      # Demo主程序
└── usbd_cdc_if.c       # USB CDC接口实现

include/
├── usb_device.h        # USB设备头文件
├── usbd_cdc_if.h       # USB CDC接口头文件
├── usbd_desc.h         # USB描述符头文件
├── usbd_conf.h         # USB配置头文件
├── main.h              # 主程序头文件（包含GPIO定义）
└── gpio.h              # GPIO配置头文件
```

## 硬件要求

- **开发板**: STM32F4xx系列开发板
- **USB连接**: Micro-USB或USB-C连接线
- **LED**: RGB LED（红、绿、蓝三个LED）
  - 红色LED: PH12 (LED_R_Pin)
  - 绿色LED: PH11 (LED_G_Pin)
  - 蓝色LED: PH10 (LED_B_Pin)

## 软件架构

### USB通讯架构

```
PC端应用程序
    ↓ (USB虚拟串口)
CDC接口层 (usbd_cdc_if.c)
    ↓ (命令解析)
命令处理层 (ProcessCommand)
    ↓ (GPIO控制)
硬件LED (RGB LED)
```

### 关键组件说明

#### 1. USB设备初始化 (`usb_device.c`)
- **功能**: 初始化USB设备核心库
- **关键函数**: `MX_USB_DEVICE_Init()`
- **初始化步骤**:
  1. USB核心库初始化
  2. CDC类注册（虚拟串口）
  3. CDC接口回调函数注册
  4. USB设备启动

#### 2. CDC接口层 (`usbd_cdc_if.c`)
- **功能**: 处理USB数据收发和命令解析
- **关键函数**:
  - `CDC_Receive_FS()` - 接收数据回调
  - `CDC_Transmit_FS()` - 发送数据接口
  - `ProcessCommand()` - 命令处理核心

#### 3. 命令系统
支持以下命令格式：

##### 基础命令
- **`LED 0/1`** - 控制红色LED开关
  - `LED 1` - 点亮红色LED
  - `LED 0` - 关闭红色LED

##### RGB控制命令
- **`RGB R G B`** - 控制RGB三色LED
  - 参数范围: 0-255
  - `RGB 255 0 0` - 红色全亮，其他关闭
  - `RGB 0 255 0` - 绿色全亮，其他关闭
  - `RGB 0 0 255` - 蓝色全亮，其他关闭
  - `RGB 255 255 255` - 白色（全亮）
  - `RGB 0 0 0` - 全部关闭

##### 辅助命令
- **`HELP`** - 显示帮助信息
- **`STATUS`** - 查询当前LED状态

## 使用方法

### 1. 编译和下载
```bash
# 使用STM32CubeIDE或其他ARM编译工具
# 编译项目并下载到开发板
```

### 2. 连接USB
1. 使用USB线连接开发板到PC
2. PC识别为虚拟串口设备
3. 记录COM端口号（Windows）或设备路径（Linux/Mac）

### 3. 串口终端设置
- **波特率**: 115200 (任意，USB虚拟串口无实际波特率限制)
- **数据位**: 8
- **停止位**: 1
- **校验位**: None
- **流控制**: None

### 4. 控制LED
发送以下命令测试功能：

```bash
# 基础测试
LED 1        # 点亮红色LED
LED 0        # 关闭红色LED

# RGB测试
RGB 255 0 0  # 红色
RGB 0 255 0  # 绿色
RGB 0 0 255  # 蓝色
RGB 255 255 255  # 白色
RGB 0 0 0    # 全部关闭

# 辅助功能
HELP         # 显示帮助
STATUS       # 查看状态
```

## 技术细节

### USB CDC协议
- **协议类型**: CDC (Communication Device Class)
- **接口类型**: 虚拟串口 (Virtual COM Port)
- **传输方式**: 中断驱动
- **缓冲区大小**: 1024字节（收发各1KB）

### 命令处理机制
```c
// 命令处理流程
CDC_Receive_FS() → ProcessCommand() → 具体命令函数 → GPIO控制 → 响应发送
```

### 错误处理
- **命令过长**: 返回错误提示
- **参数无效**: 返回格式说明
- **未知命令**: 提示使用HELP命令

## 调试和故障排除

### 常见问题

#### 1. USB设备未识别
- **检查**: USB线缆、开发板供电
- **解决**: 更换USB线或重新插拔

#### 2. 命令无响应
- **检查**: 串口终端设置、换行符格式
- **解决**: 确保发送CR或LF作为命令结束符

#### 3. LED不亮
- **检查**: GPIO配置、硬件连接
- **解决**: 验证LED引脚定义和硬件连接

### 调试技巧

1. **使用断点调试**: 在`ProcessCommand`函数设置断点
2. **串口监视**: 监视接收到的原始数据
3. **LED状态检查**: 使用`STATUS`命令查看当前状态

## 扩展功能建议

### 1. PWM亮度控制
```c
// 可以添加PWM控制实现LED亮度调节
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, brightness);
```

### 2. 更多LED效果
```c
// 呼吸灯效果
void BreathingLED(uint8_t led, uint16_t period);

// 彩虹循环效果
void RainbowCycle(uint16_t period);
```

### 3. 状态保存
```c
// 使用EEPROM保存LED状态
void SaveLEDState(void);
void LoadLEDState(void);
```

## 性能特点

- **响应时间**: <1ms（USB中断处理）
- **命令解析**: 实时处理，无延迟
- **功耗**: 低功耗模式，__WFI()节能
- **稳定性**: 支持长时间连续运行

## 总结

这个演示程序完整展示了STM32 USB CDC的应用，包括：
- USB虚拟串口通信
- 命令解析系统
- GPIO控制
- 错误处理机制
- 用户友好的交互界面

通过学习这个代码，您可以掌握：
1. STM32 USB CDC开发
2. 串口命令解析
3. GPIO外设控制
4. 嵌入式系统架构设计

这为进一步开发复杂的USB应用设备提供了良好的基础。