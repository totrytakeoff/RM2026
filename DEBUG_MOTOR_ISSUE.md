# M3508 电机不转问题诊断

## 问题现象
测试1（单电机1000rpm速度控制）- 电机完全不转

## 关键代码执行流程

### 1. 初始化流程
```cpp
// main() -> init_motors()
motor1.init();  // 注册CAN回调
motor1.setSpeedPID(3.0f, 0.2f, 0.0f);  // 设置PID参数
```

### 2. 控制循环
```cpp
while (1) {
    test_speed_single(1000.0f);  // 目标1000rpm
    HAL_Delay(1);
}

// test_speed_single实现：
void test_speed_single(float target_speed) {
    motor1.setTargetSpeed(target_speed);  // 设置目标，切换到SPEED_LOOP模式
    motor1.update();  // PID计算并发送电流
}
```

### 3. PID计算过程
```cpp
// M3508::update() - dt=0，使用PID内部采样时间
float current_speed = meas_.speed_rpm;  // 当前速度
float output = speed_pid_.compute(target_speed_, current_speed);

// PID计算（Kp=3.0, Ki=0.2）：
error = 1000 - 0 = 1000rpm
p_term = 3.0 * 1000 = 3000
i_term = 0 (首次)
output = 3000

// **关键检查点：输出限制**
// M3508构造函数中设置：
speed_pid_.setOutputLimits(-16384.0f, 16384.0f);

// PID默认限制：
out_min_ = -100.0f  // ← 问题所在！
out_max_ = 100.0f   // ← 问题所在！

// 如果setOutputLimits没有生效，output会被限制到±100！
// 100的电流完全无法驱动M3508！
```

## 可能的原因

### 原因1：PID输出限制问题 ⭐⭐⭐⭐⭐
**最可能的原因**：PIDController默认输出限制±100，虽然M3508构造函数调用了setOutputLimits(-16384, 16384)，但可能：
1. 调用顺序问题
2. setOutputLimits未生效
3. 某处被重置

**验证方法**：
- 检查PID实际输出值
- 确认setOutputLimits是否生效

**解决方案**：
- ✅ 已修改PID默认限制为±20000
- 确保M3508构造中的setOutputLimits调用正确

### 原因2：Kp过小导致启动力矩不足
- 原Kp=15.0 → 现Kp=3.0
- 误差1000rpm时，输出从15000降到3000
- 如果限制生效，3000应该够；但如果被限到100，完全不够

### 原因3：CAN通信问题
- 电机未收到反馈数据
- 电流指令未发送成功
- CAN总线初始化失败

### 原因4：电机未收到CAN反馈
- meas_.speed_rpm一直是0
- PID计算正常但反馈异常
- 导致电机以为自己在转，实际没转

## 调试建议

### 1. 添加调试输出
```cpp
void test_speed_single(float target_speed) {
    motor1.setTargetSpeed(target_speed);
    motor1.update();
    
    // 添加调试
    printf("Target: %.0f, Current: %d, Output: %d\n", 
           target_speed, 
           motor1.measure().speed_rpm, 
           motor1.getLastCurrent());
}
```

### 2. 检查PID输出限制
```cpp
// 在init_motors()后添加：
printf("Speed PID limits: min=%.0f, max=%.0f\n", 
       motor1.getSpeedPID().getOutputMin(),
       motor1.getSpeedPID().getOutputMax());
```

### 3. 简化测试 - 直接给电流
```cpp
// 绕过PID，直接给固定电流测试
while (1) {
    motor1.setCurrent(3000);  // 直接给3000电流
    HAL_Delay(100);
}
```

### 4. 测试CAN通信
```cpp
// 检查是否收到反馈
static int last_ecd = -1;
if (motor1.measure().ecd != last_ecd) {
    printf("Received feedback: ecd=%d, speed=%d\n",
           motor1.measure().ecd,
           motor1.measure().speed_rpm);
    last_ecd = motor1.measure().ecd;
}
```

## 临时解决方案

### 方案1：提高Kp（快速验证）
```cpp
motor1.setSpeedPID(10.0f, 0.5f, 0.0f);  // 恢复接近原来的参数
```

### 方案2：使用开环控制
```cpp
// 直接给电流，不用PID
while (1) {
    motor1.setCurrent(5000);  // 给固定电流
    HAL_Delay(1);
}
```

### 方案3：使用test_speed_direct
```cpp
// 使用简化P控制
while (1) {
    test_speed_direct(1000.0f, 0, 0, 0, 10.0f);  // Kp=10.0
    HAL_Delay(1);
}
```

## 预期修复效果

修改PID默认输出限制为±20000后：
- PID输出不会被限制到±100
- 即使M3508构造函数中的setOutputLimits未生效，也能正常工作
- Kp=3.0时，1000rpm误差产生3000电流输出
- 3000电流应该足以驱动M3508转动

## 下一步检查

1. 重新编译并烧录
2. 测试电机是否转动
3. 如果仍不转，使用调试方案1-4逐一排查
4. 如果转了但不稳定，需要重新调整PID参数
