# BSP LED 封装方案文档

## 概述

本文档描述了RM C型板RGB LED的BSP封装方案，该方案参考了官方demo的实现方式，提供了丰富的LED控制功能。

## 功能特性

### 基础功能
- ✅ 基本颜色显示（红、绿、蓝、白等预定义颜色）
- ✅ 自定义RGB色彩显示（0-255分量格式）
- ✅ aRGB格式支持（带透明度）
- ✅ 亮度调节（0-255）

### 高级效果
- ✅ 呼吸灯效果（可调节周期和颜色）
- ✅ RGB流水灯效果（支持自定义颜色序列）
- ✅ 渐变显示（颜色平滑过渡）
- ✅ 闪烁效果（可调节周期和占空比）
- ✅ 彩虹循环效果
- ✅ 淡入淡出效果
- ✅ 颜色混合功能

## 硬件配置

### 引脚定义
- **红色LED**: PH12 (TIM5_CH3)
- **绿色LED**: PH11 (TIM5_CH2)  
- **蓝色LED**: PH10 (TIM5_CH1)

### PWM配置
- **定时器**: TIM5
- **PWM周期**: 65535
- **预分频器**: 0
- **时钟源**: APB1 (42MHz)

## API接口

### 初始化函数
```c
BSP_StatusTypeDef BSP_LED_Init(void);
BSP_StatusTypeDef BSP_LED_DeInit(void);
```

### 基础颜色设置
```c
// aRGB格式设置 (0xaaRRGGBB)
void BSP_LED_SetColor(uint32_t aRGB);

// RGB分量设置
void BSP_LED_SetRGB(uint8_t red, uint8_t green, uint8_t blue);

// aRGB分量设置
void BSP_LED_SetaRGB(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);

// 预定义颜色设置
void BSP_LED_SetPresetColor(uint32_t color);
```

### 预定义颜色常量
```c
#define BSP_LED_COLOR_RED         0xFFFF0000  // 红色
#define BSP_LED_COLOR_GREEN       0xFF00FF00  // 绿色
#define BSP_LED_COLOR_BLUE        0xFF0000FF  // 蓝色
#define BSP_LED_COLOR_YELLOW      0xFFFFFF00  // 黄色
#define BSP_LED_COLOR_CYAN        0xFF00FFFF  // 青色
#define BSP_LED_COLOR_MAGENTA     0xFFFF00FF  // 洋红色
#define BSP_LED_COLOR_WHITE       0xFFFFFFFF  // 白色
#define BSP_LED_COLOR_BLACK       0xFF000000  // 黑色(关闭)
#define BSP_LED_COLOR_ORANGE      0xFFFFA500  // 橙色
#define BSP_LED_COLOR_PURPLE      0xFF800080  // 紫色
```

### 效果控制函数
```c
// 呼吸灯效果
void BSP_LED_SetBreathing(uint32_t color, uint16_t period);

// 流水灯效果
void BSP_LED_SetFlow(const uint32_t *colors, uint8_t count, uint16_t period);

// 渐变效果
void BSP_LED_SetGradient(uint32_t start_color, uint32_t end_color, uint16_t period);

// 闪烁效果
void BSP_LED_SetBlink(uint32_t color, uint16_t period, uint8_t duty_cycle);

// 停止所有效果
void BSP_LED_StopEffect();
```

### 高级功能函数
```c
// 彩虹循环
void BSP_LED_Rainbow(uint16_t period);

// 淡入淡出
void BSP_LED_FadeIn(uint32_t color, uint16_t duration);
void BSP_LED_FadeOut(uint16_t duration);

// 颜色混合
uint32_t BSP_LED_MixColor(uint32_t color1, uint32_t color2, uint8_t ratio);

// 亮度控制
void BSP_LED_SetBrightness(uint8_t brightness);
uint8_t BSP_LED_GetBrightness(void);
```

### 系统函数
```c
// LED更新函数（需要在主循环中调用）
void BSP_LED_Update(void);

// 开关控制
void BSP_LED_On(void);
void BSP_LED_Off(void);

// 获取配置
BSP_LED_ConfigTypeDef* BSP_LED_GetConfig(void);
```

## 使用示例

### 基础使用
```c
// 初始化LED
BSP_LED_Init();

// 设置红色
BSP_LED_SetPresetColor(BSP_LED_COLOR_RED);

// 设置自定义颜色 (橙色)
BSP_LED_SetRGB(255, 165, 0);

// 设置亮度为50%
BSP_LED_SetBrightness(128);
```

### 效果演示
```c
// 呼吸灯效果 - 红色，2秒周期
BSP_LED_SetBreathing(BSP_LED_COLOR_RED, 2000);

// 流水灯效果 - 彩虹色，每色1秒
uint32_t colors[] = {BSP_LED_COLOR_RED, BSP_LED_COLOR_GREEN, BSP_LED_COLOR_BLUE};
BSP_LED_SetFlow(colors, 3, 1000);

// 渐变效果 - 从红到蓝，3秒
BSP_LED_SetGradient(BSP_LED_COLOR_RED, BSP_LED_COLOR_BLUE, 3000);

// 闪烁效果 - 白色，500ms周期，50%占空比
BSP_LED_SetBlink(BSP_LED_COLOR_WHITE, 500, 50);

// 彩虹循环 - 6秒完整周期
BSP_LED_Rainbow(6000);
```

### 主循环集成
```c
int main(void) {
    // BSP初始化
    BSP_InitTypeDef config = BSP_INIT_DEFAULT;
    BSP_Init(&config);
    
    // LED初始化
    BSP_LED_Init();
    
    // 设置效果
    BSP_LED_SetBreathing(BSP_LED_COLOR_BLUE, 2000);
    
    while (1) {
        // 更新LED效果
        BSP_LED_Update();
        
        // 其他应用代码
        // ...
        
        HAL_Delay(10);
    }
}
```

## 实现原理

### PWM控制
- 使用TIM5的三个PWM通道分别控制RGB三个颜色分量
- PWM占空比决定LED亮度
- 16位PWM精度提供平滑的亮度调节

### 效果实现
- **呼吸灯**: 使用正弦函数计算亮度变化
- **流水灯**: 按时间间隔切换预定义颜色
- **渐变**: 线性插值计算颜色过渡
- **闪烁**: 方波控制开关状态

### 时间管理
- 使用HAL_GetTick()获取系统时间
- 10ms更新周期保证效果流畅性
- 支持毫秒级时间控制

## 参考实现

本封装方案参考了以下官方demo：

1. **PWM_light demo**: 
   - PWM配置和控制方式
   - 颜色分量到PWM值的转换

2. **homework_flow_led demo**:
   - 流水灯效果实现
   - 颜色渐变算法
   - aRGB颜色格式处理

## 文件结构

```
src/bsp/
├── bsp_led.h          # LED BSP头文件
└── bsp_led.c          # LED BSP实现文件

src/test/
└── bsp_led_demo.cpp   # LED功能演示程序
```

## 注意事项

1. **初始化顺序**: 必须先调用BSP_Init()再调用BSP_LED_Init()
2. **更新函数**: 使用效果时必须在主循环中调用BSP_LED_Update()
3. **时间精度**: 效果更新频率为10ms，避免过于频繁的调用
4. **内存管理**: 流水灯最多支持8个颜色
5. **线程安全**: 当前实现不支持多线程，请在单线程环境中使用

## 编译配置

确保在platformio.ini中包含以下配置：

```ini
[env:stm32f407ig]
build_flags = 
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
```

## 测试验证

运行`src/test/bsp_led_demo.cpp`可以验证所有功能：

1. 基本颜色显示测试
2. 自定义RGB色彩测试
3. 呼吸灯效果测试
4. 流水灯效果测试
5. 渐变效果测试
6. 闪烁效果测试
7. 彩虹循环测试
8. 淡入淡出测试
9. 亮度调节测试
10. 颜色混合测试

## 版本信息

- **版本**: 1.0
- **作者**: RM2026 Team
- **日期**: 2025-11-26
- **兼容性**: STM32F407IGT6, RM C型开发板
