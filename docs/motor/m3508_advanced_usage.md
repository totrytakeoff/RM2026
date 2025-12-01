# M3508 电机高级控制使用指南

## 概述

本文档介绍优化后的 M3508 电机控制类的使用方法，包括：
- 多回调 CAN 通信机制
- 速度环闭环控制
- 位置环闭环控制
- 多电机协同控制

## 更新内容

### 1. CAN 通信优化

#### 1.1 多回调支持

原来的 `CanBus` 类只支持单个回调函数，现在支持注册多个回调：

```cpp
// 创建 CAN 总线对象
CanBus can1(&hcan1);

// 注册多个回调函数
can1.registerRxCallback(callback1, nullptr);           // 接收所有帧
can1.registerRxCallback(callback2, 0x201, false, &motor1);  // 只接收 ID=0x201 的帧
can1.registerRxCallback(callback3, 0x205, false, &motor2);  // 只接收 ID=0x205 的帧

// 注销回调
can1.unregisterRxCallback(callback1);

// 清除所有回调
can1.clearAllCallbacks();

// 查询回调数量
uint8_t count = can1.getCallbackCount();
```

#### 1.2 回调过滤

支持基于 CAN ID 的回调过滤，提高处理效率：

```cpp
// 注册带过滤的回调
bool success = can1.registerRxCallback(
    myCallback,      // 回调函数
    0x201,          // 过滤 ID
    false,          // 是否为扩展 ID
    user_data       // 用户数据指针
);
```

#### 1.3 实现细节

- 使用静态数组池管理回调节点（最多 16 个）
- 链表结构，无动态内存分配
- 支持标准帧和扩展帧过滤

### 2. M3508 电机控制增强

#### 2.1 控制模式

支持三种控制模式：

```cpp
enum class ControlMode {
    OPEN_LOOP = 0,      // 开环控制（直接电流）
    SPEED_LOOP = 1,     // 速度环控制
    POSITION_LOOP = 2   // 位置环控制（级联）
};
```

#### 2.2 基本使用流程

```cpp
// 1. 创建电机对象
CanBus can1(&hcan1);
M3508 motor(&can1, 1);  // ID=1 的电机

// 2. 初始化（注册 CAN 回调）
if (!motor.init()) {
    // 初始化失败处理
}

// 3. 配置 PID 参数
motor.setSpeedPID(15.0f, 0.8f, 0.0f);      // 速度环 PID
motor.setPositionPID(0.8f, 0.0f, 0.2f);    // 位置环 PID

// 4. 设置控制模式和目标值
motor.setTargetSpeed(1000.0f);  // 目标速度 1000 rpm

// 5. 在主循环中更新控制器
while(1) {
    // 轮询 CAN 接收
    while(can1.pollOnce());
    
    // 更新电机控制器
    motor.update();  // 使用 PID 内部采样时间
    // 或
    motor.update(0.001f);  // 指定时间间隔（1ms）
    
    HAL_Delay(1);
}
```

#### 2.3 开环控制

直接设置电流值：

```cpp
motor.setControlMode(M3508::ControlMode::OPEN_LOOP);
motor.setCurrent(5000);  // 范围: -16384 ~ 16384
```

#### 2.4 速度环控制

使用 PID 控制速度：

```cpp
// 配置速度环 PID
motor.setSpeedPID(15.0f, 0.8f, 0.0f);

// 设置目标速度（自动切换到速度环模式）
motor.setTargetSpeed(1000.0f);  // 1000 rpm

// 在主循环中更新
motor.update();
```

**速度环 PID 参数调试建议：**
- Kp: 10 ~ 20（主要影响响应速度）
- Ki: 0.5 ~ 1.5（消除稳态误差）
- Kd: 0 ~ 0.5（抑制超调，通常不需要）

#### 2.5 位置环控制

使用级联 PID 控制位置：

```cpp
// 配置位置环和速度环 PID
motor.setPositionPID(0.8f, 0.0f, 0.2f);
motor.setSpeedPID(15.0f, 0.8f, 0.0f);

// 设置目标位置（自动切换到位置环模式）
motor.setTargetPosition(360.0f);  // 360 度

// 在主循环中更新
motor.update();
```

**位置环 PID 参数调试建议：**
- Kp: 0.5 ~ 1.5（主要影响位置跟踪）
- Ki: 0 ~ 0.1（消除位置偏差，通常很小）
- Kd: 0.1 ~ 0.5（抑制震荡）

#### 2.6 多电机控制

```cpp
// 创建多个电机对象
M3508 motor1(&can1, 1);
M3508 motor2(&can1, 2);
M3508 motor3(&can1, 3);
M3508 motor4(&can1, 4);

// 初始化所有电机
motor1.init();
motor2.init();
motor3.init();
motor4.init();

// 设置不同的控制模式
motor1.setTargetSpeed(1000.0f);     // 速度环
motor2.setTargetPosition(360.0f);   // 位置环
motor3.setCurrent(5000);            // 开环

// 主循环
while(1) {
    while(can1.pollOnce());  // 轮询 CAN（触发所有电机的回调）
    
    motor1.update();
    motor2.update();
    motor3.update();
    motor4.update();
    
    HAL_Delay(1);
}
```

#### 2.7 状态查询

```cpp
// 获取当前测量值
const M3508::Measure& meas = motor.measure();
uint16_t ecd = meas.ecd;              // 编码器原始值 (0-8191)
int16_t speed = meas.speed_rpm;       // 速度 (rpm)
int32_t angle = meas.total_angle;     // 累计角度（度）
int32_t rounds = meas.total_rounds;   // 累计圈数
uint8_t temp = meas.temperature;      // 温度

// 获取控制状态
M3508::ControlMode mode = motor.getControlMode();
float current_speed = motor.getCurrentSpeed();      // rpm
float current_pos = motor.getCurrentPosition();     // 度
int16_t output = motor.getLastCurrent();           // 输出电流
```

#### 2.8 重置控制器

```cpp
// 重置 PID 状态和累计角度
motor.reset();
```

### 3. 编码器角度计算

M3508 使用 8192 线编码器，自动处理：
- 编码器溢出（0 ↔ 8191）
- 累计圈数统计
- 累计角度计算（度）

```cpp
// 累计角度 = 圈数 × 360 + 当前编码器角度
int32_t total_angle = motor.getCurrentPosition();

// 示例：
// 转 1.5 圈 → total_angle = 540 度
// 转 -2 圈 → total_angle = -720 度
```

### 4. PID 参数调试流程

#### 4.1 速度环调试

1. **设置初始参数**：Kp=10, Ki=0, Kd=0
2. **增大 Kp**：直到出现轻微震荡，然后减小 20%
3. **增加 Ki**：消除稳态误差，通常 0.5~1.5
4. **调整 Kd**：如果有超调，增加 Kd（通常不需要）

#### 4.2 位置环调试

1. **先调好速度环**
2. **设置位置环初始参数**：Kp=0.5, Ki=0, Kd=0
3. **增大 Kp**：提高响应速度
4. **增加 Kd**：抑制震荡
5. **微调 Ki**：消除静差（通常很小或为 0）

### 5. 注意事项

1. **初始化顺序**：
   - 先初始化 BSP 和 CAN
   - 再创建 CanBus 和 M3508 对象
   - 调用 `motor.init()` 注册回调

2. **控制频率**：
   - 建议 1ms 更新一次控制器
   - CAN 轮询频率越高越好

3. **电流限制**：
   - M3508 电流范围：-16384 ~ 16384
   - 速度环输出已自动限幅

4. **回调数量**：
   - 最多支持 16 个回调
   - 每个电机占用 1 个回调

5. **线程安全**：
   - 当前实现不是线程安全的
   - 如果使用 RTOS，需要添加互斥锁

### 6. 测试程序

参考 `src/test/motor/M3508_advanced_demo.cpp`，包含：
- 开环控制测试
- 速度环控制测试
- 位置环控制测试
- 多电机协同测试

### 7. 常见问题

**Q: 电机不转？**
- 检查 CAN 是否正确初始化
- 检查是否调用了 `motor.init()`
- 检查是否在主循环中调用 `motor.update()`
- 检查 CAN 线是否正确连接

**Q: 速度环震荡？**
- 减小 Kp
- 增加微分滤波
- 检查机械负载是否过大

**Q: 位置环不稳定？**
- 先确保速度环稳定
- 减小位置环 Kp
- 增加位置环 Kd

**Q: 累计角度不准确？**
- 检查 CAN 接收是否正常
- 检查是否有丢帧
- 必要时调用 `motor.reset()` 重置

## 总结

本次优化实现了：
1. ✅ CAN 多回调管理（无 STL，静态内存）
2. ✅ M3508 速度环控制（PID）
3. ✅ M3508 位置环控制（级联 PID）
4. ✅ 累计角度计算
5. ✅ 多电机协同控制
6. ✅ 完整的测试程序

---

**作者**: RM2026 Team  
**日期**: 2025-12-02  
**版本**: v2.0
