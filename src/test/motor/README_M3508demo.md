# M3508demo 使用说明

## 概述

这是一个简单的 M3508 电机测试程序，提供了速度环和位置环的测试函数。

## 功能函数

### 1. 初始化函数

```cpp
void init_motors()
```
- 初始化 4 个电机
- 注册 CAN 回调
- 配置默认 PID 参数

### 2. 速度环测试

#### 单电机速度环
```cpp
void test_speed_single(float target_speed)
```
- 参数：`target_speed` - 目标速度 (rpm)
- 示例：`test_speed_single(1000.0f);  // 1000 rpm`

#### 四电机批量速度环
```cpp
void test_speed_group(float s1, float s2, float s3, float s4)
```
- 参数：`s1-s4` - 各电机目标速度 (rpm)
- 示例：`test_speed_group(500.0f, -500.0f, 800.0f, -800.0f);`

### 3. 位置环测试

#### 单电机位置环
```cpp
void test_position_single(float target_position)
```
- 参数：`target_position` - 目标位置（度）
- 示例：`test_position_single(360.0f);  // 转到 360 度`

#### 四电机批量位置环
```cpp
void test_position_group(float p1, float p2, float p3, float p4)
```
- 参数：`p1-p4` - 各电机目标位置（度）
- 示例：`test_position_group(0.0f, 360.0f, 720.0f, 180.0f);`

### 4. 停止函数

```cpp
void stop_all_motors()
```
- 停止所有电机（发送 0 电流）

### 5. 直接控制函数（无需创建实例）

#### 直接速度环控制
```cpp
void test_speed_direct(float target1, float target2, float target3, float target4, float kp = 15.0f)
```
- 参数：`target1-4` - 各电机目标速度 (rpm)，`kp` - PID 参数
- 示例：`test_speed_direct(1000.0f, -1000.0f, 500.0f, -500.0f);`
- **特点**：直接传值，无需通过电机实例，适合快速测试

#### 直接位置环控制
```cpp
void test_position_direct(float target1, float target2, float target3, float target4, 
                         float pos_kp = 0.8f, float spd_kp = 15.0f)
```
- 参数：`target1-4` - 各电机目标位置（度），`pos_kp/spd_kp` - PID 参数
- 示例：`test_position_direct(360.0f, 720.0f, 180.0f, 540.0f);`
- **特点**：直接传值，级联控制，无需通过电机实例

## 测试模式

在 `main()` 函数中提供了 7 种测试模式，取消注释对应的代码即可：

### 测试 1: 单电机恒速测试
```cpp
while (1) {
    test_speed_single(1000.0f);  // 1000 rpm
    HAL_Delay(1);
}
```

### 测试 2: 四电机速度环批量测试
```cpp
while (1) {
    test_speed_group(500.0f, -500.0f, 800.0f, -800.0f);
    HAL_Delay(1);
}
```

### 测试 3: 单电机位置环往复运动
```cpp
while (1) {
    test_position_single(360.0f);   // 转到 360 度
    HAL_Delay(2000);
    test_position_single(0.0f);     // 转回 0 度
    HAL_Delay(2000);
}
```

### 测试 4: 四电机位置环批量测试
```cpp
while (1) {
    test_position_group(0.0f, 0.0f, 0.0f, 0.0f);
    HAL_Delay(2000);
    test_position_group(360.0f, 720.0f, 180.0f, 540.0f);
    HAL_Delay(2000);
}
```

### 测试 5: 直接速度环测试（无需实例）
```cpp
while (1) {
    test_speed_direct(1000.0f, -1000.0f, 500.0f, -500.0f);
    HAL_Delay(1);
}
```

### 测试 6: 直接位置环测试（无需实例）
```cpp
while (1) {
    test_position_direct(0.0f, 0.0f, 0.0f, 0.0f);
    HAL_Delay(2000);
    test_position_direct(360.0f, 720.0f, 180.0f, 540.0f);
    HAL_Delay(2000);
}
```

### 测试 7: 速度环变速测试（默认启用）
```cpp
while (1) {
    // 加速 0 → 1000 rpm
    for (int i = 0; i <= 1000; i += 100) {
        test_speed_single((float)i);
        HAL_Delay(500);
    }
    // 减速 1000 → 0 rpm
    for (int i = 1000; i >= 0; i -= 100) {
        test_speed_single((float)i);
        HAL_Delay(500);
    }
    // 反向 0 → -1000 rpm
    for (int i = 0; i >= -1000; i -= 100) {
        test_speed_single((float)i);
        HAL_Delay(500);
    }
    // 回零 -1000 → 0 rpm
    for (int i = -1000; i <= 0; i += 100) {
        test_speed_single((float)i);
        HAL_Delay(500);
    }
}
```

## PID 参数

### 默认速度环 PID
- Kp = 15.0
- Ki = 0.8
- Kd = 0.0

### 默认位置环 PID
- Kp = 0.8
- Ki = 0.0
- Kd = 0.2

**注意**：这些参数需要根据实际电机和负载情况调整。

## 使用步骤

1. **选择测试模式**：在 `main()` 函数中取消注释想要的测试
2. **编译上传**：使用 PlatformIO 编译并上传到开发板
3. **观察 LED**：
   - GPIO_PIN_11 亮 → 电机初始化完成
   - GPIO_PIN_10 亮 → 系统就绪
4. **调整参数**：根据实际效果调整 PID 参数或目标值

## 注意事项

1. **安全第一**：首次测试时使用较小的速度/位置值
2. **控制频率**：测试函数内部已包含 `update()` 调用，主循环只需 1ms 延迟
3. **CAN 中断**：程序使用 CAN 中断模式，自动接收反馈数据
4. **电机 ID**：确保电机 ID 设置为 1-4

## 调试技巧

1. 如需打印调试信息，取消注释测试函数中的 `printf` 语句
2. 可以通过 LED 闪烁频率判断程序运行状态
3. 建议先测试单电机，确认正常后再测试多电机

## 常见问题

**Q: 电机不转？**
- 检查 CAN 线连接
- 检查电机 ID 是否正确
- 检查电源是否正常

**Q: 电机抖动？**
- 降低 PID 参数（特别是 Kp）
- 检查机械负载是否过大
- 增加控制频率

**Q: 位置不准？**
- 检查编码器反馈是否正常
- 调整位置环 PID 参数
- 确认累计角度计算正确

---

**作者**: RM2026 Team  
**日期**: 2025-12-02
