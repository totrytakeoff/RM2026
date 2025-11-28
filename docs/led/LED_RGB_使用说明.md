# LED和RGB类使用说明

## 概述

本项目实现了通用的LED和RGB LED封装类，支持GPIO和PWM两种控制模式，并提供了丰富的LED效果功能。

## 类结构

### LED类
- **功能**: 单个LED控制
- **支持模式**: GPIO模式、PWM模式
- **主要功能**: 开关、亮度调节、PWM控制、呼吸灯、闪烁

### RGBLED类
- **功能**: RGB LED组合控制
- **支持模式**: 基于三个LED对象组合
- **主要功能**: 颜色设置、亮度控制、呼吸灯、多种颜色格式

## 硬件连接

### LED硬件定义 (main.h)
```c
#define LED_R_Pin GPIO_PIN_12
#define LED_R_GPIO_Port GPIOH
#define LED_G_Pin GPIO_PIN_11
#define LED_G_GPIO_Port GPIOH
#define LED_B_Pin GPIO_PIN_10
#define LED_B_GPIO_Port GPIOH

#define KEY_Pin GPIO_PIN_0
#define KEY_GPIO_Port GPIOA
#define BUTTON_TRIG_Pin GPIO_PIN_1
#define BUTTON_TRIG_GPIO_Port GPIOA
```

### PWM配置
- **定时器**: TIM5
- **红色LED**: TIM5_CH3 (PH12)
- **绿色LED**: TIM5_CH2 (PH11)
- **蓝色LED**: TIM5_CH1 (PH10)

## 基本使用方法

### 1. 创建LED对象

```cpp
// GPIO模式LED
LED status_led(GPIOA, GPIO_PIN_0);

// PWM模式LED
LED red_led(LED_R_GPIO_Port, LED_R_Pin, &htim5, TIM_CHANNEL_3);
LED green_led(LED_G_GPIO_Port, LED_G_Pin, &htim5, TIM_CHANNEL_2);
LED blue_led(LED_B_GPIO_Port, LED_B_Pin, &htim5, TIM_CHANNEL_1);

// 创建RGB LED对象
RGBLED rgb_led(red_led, green_led, blue_led);
```

### 2. 基本控制

```cpp
// LED基本操作
led.on();                    // 开启
led.off();                   // 关闭
led.toggle(3, 200);         // 闪烁3次，间隔200ms

// PWM模式亮度控制
led.setBrightness(128);       // 设置亮度(0-255)
led.setPWM(32768);          // 直接设置PWM值(0-65535)

// 呼吸灯效果
led.BreathingLight(2, 50);  // 2秒周期，50ms间隔
```

### 3. RGB LED颜色控制

```cpp
// 预定义颜色
rgb_led.setColor(RED);
rgb_led.setColor(GREEN);
rgb_led.setColor(BLUE);

// RGB颜色
rgb_led.setColorRGB(255, 0, 0);    // 红色
rgb_led.setColorRGB(0, 255, 0);    // 绿色
rgb_led.setColorRGB(0, 0, 255);    // 蓝色

// ARGB颜色(带透明度)
rgb_led.setColorARGB(128, 255, 0, 0);  // 半透明红色

// HSV颜色
rgb_led.setColorHSV(120, 255, 255);     // 绿色
rgb_led.setColorHSV(240, 255, 255);     // 蓝色

// 亮度控制
rgb_led.setBrightness(128);  // 50%亮度

// RGB呼吸灯
rgb_led.BreathingLight(3, 30);  // 3秒周期，30ms间隔
```

## 按钮控制

### 创建按钮对象

```cpp
Button mode_button(KEY_GPIO_Port, KEY_Pin, ButtonMode::PULL_UP, 50);
Button func_button(BUTTON_TRIG_GPIO_Port, BUTTON_TRIG_Pin, ButtonMode::PULL_UP, 50);
```

### 按钮事件检测

```cpp
// 按下事件
if (button.isPressed()) {
    // 按钮被按下时执行
}

// 释放事件
if (button.isReleased()) {
    // 按钮释放时执行
}

// 长按事件
if (button.isLongPressed(2000)) {
    // 长按2秒时执行
}

// 获取按下持续时间
uint32_t duration = button.getPressedDuration();
```

## 测试模式

项目提供了完整的测试程序 `led_demo_main()`，包含10种测试模式：

### 模式列表
1. **MODE_OFF**: 关闭所有LED
2. **MODE_BASIC_LED**: 基本LED测试
3. **MODE_PWM_BRIGHTNESS**: PWM亮度渐变测试
4. **MODE_RGB_COLORS**: RGB预定义颜色测试
5. **MODE_RGB_CUSTOM**: RGB自定义颜色测试
6. **MODE_BREATHING**: 呼吸灯效果测试
7. **MODE_RAINBOW**: 彩虹循环效果
8. **MODE_STROBE**: 频闪效果
9. **MODE_POLICE**: 警灯效果
10. **MODE_HEARTBEAT**: 心跳效果

### 操作说明
- **KEY按钮**: 切换测试模式
- **TRIG按钮**: 在当前模式下执行特定功能
- **模式指示**: 切换模式时，状态LED闪烁次数表示当前模式编号

## 预定义颜色

```cpp
enum Color_Types {
    RED = 0xFFFF0000,
    ORANGE = 0xFFFFA500,
    YELLOW = 0xFFFFFF00,
    GREEN = 0xFF00FF00,
    CYAN = 0xFF00FFFF,
    BLUE = 0xFF0000FF,
    PURPLE = 0xFF800080,
    WHITE = 0xFFFFFFFF,
};
```

## 注意事项

1. **PWM模式**: 只有配置了定时器和通道的LED才能使用亮度调节和呼吸灯功能
2. **GPIO模式**: 仅支持开关和闪烁功能
3. **按钮防抖**: 默认50ms防抖时间，可根据需要调整
4. **呼吸灯**: 使用静态变量实现，同时只能有一个呼吸灯效果
5. **内存使用**: RGBLED类使用引用，避免内存拷贝

## 扩展功能

### 添加新的颜色效果
```cpp
void custom_effect() {
    static uint16_t step = 0;
    uint32_t current_time = HAL_GetTick();
    
    if (current_time - last_update_time >= 50) {
        // 自定义效果逻辑
        step++;
    }
}
```

### 添加新的测试模式
1. 在`TestMode`枚举中添加新模式
2. 实现对应的模式函数
3. 在主循环的switch语句中添加case
4. 在`update_test_mode()`中添加按钮响应逻辑

## 编译和运行

```bash
# 编译项目
pio run -e cmsis-dap-native

# 上传固件
pio run -e cmsis-dap-native -t upload

# 监控串口
pio device monitor
```

## 故障排除

1. **编译错误**: 检查头文件包含路径和硬件定义
2. **LED不亮**: 检查GPIO配置和PWM初始化
3. **按钮无响应**: 检查按钮硬件连接和上拉/下拉配置
4. **呼吸灯不工作**: 确保LED配置为PWM模式且定时器已启动

## 性能优化

- 使用`HAL_GetTick()`获取系统时间，避免阻塞延时
- 按钮检测使用状态机，减少CPU占用
- LED效果使用时间差分，实现平滑过渡
