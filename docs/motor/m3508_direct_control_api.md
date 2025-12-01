# M3508 直接控制 API 说明

## 概述

新增的直接控制函数允许你在**不创建电机实例**的情况下，直接批量控制 4 个电机。这些函数适合快速测试和简单应用场景。

## API 参考

### 1. 直接速度环控制

```cpp
static bool M3508::controlSpeedDirect(
    CanBus* can,                    // CAN 总线指针
    float target1, float target2,   // 电机 1-2 目标速度 (rpm)
    float target3, float target4,   // 电机 3-4 目标速度 (rpm)
    float current1, float current2, // 电机 1-2 当前速度 (rpm)
    float current3, float current4, // 电机 3-4 当前速度 (rpm)
    float kp, float ki, float kd,   // PID 参数（当前仅使用 kp）
    uint8_t group = 1               // 组号：1=ID 1-4, 2=ID 5-8
);
```

**功能**：
- 直接计算 4 个电机的速度环 PID 输出
- 批量发送电流控制指令
- 无需创建电机实例

**参数说明**：
- `target1-4`：目标速度 (rpm)
- `current1-4`：当前速度 (rpm)，从电机反馈获取
- `kp`：比例系数，建议 10-20
- `ki, kd`：积分和微分系数（当前实现仅使用 P 控制）
- `group`：1=控制 ID 1-4 的电机，2=控制 ID 5-8 的电机

**返回值**：
- `true`：发送成功
- `false`：发送失败

**使用示例**：
```cpp
// 获取当前速度
float spd1 = motor1.measure().speed_rpm;
float spd2 = motor2.measure().speed_rpm;
float spd3 = motor3.measure().speed_rpm;
float spd4 = motor4.measure().speed_rpm;

// 直接控制
M3508::controlSpeedDirect(&g_can,
                         1000.0f, -1000.0f, 500.0f, -500.0f,  // 目标速度
                         spd1, spd2, spd3, spd4,              // 当前速度
                         15.0f, 0.0f, 0.0f,                   // PID 参数
                         1);                                   // 组 1
```

---

### 2. 直接位置环控制

```cpp
static bool M3508::controlPositionDirect(
    CanBus* can,                    // CAN 总线指针
    float target_pos1, float target_pos2,   // 电机 1-2 目标位置（度）
    float target_pos3, float target_pos4,   // 电机 3-4 目标位置（度）
    float current_pos1, float current_pos2, // 电机 1-2 当前位置（度）
    float current_pos3, float current_pos4, // 电机 3-4 当前位置（度）
    float current_spd1, float current_spd2, // 电机 1-2 当前速度 (rpm)
    float current_spd3, float current_spd4, // 电机 3-4 当前速度 (rpm)
    float pos_kp, float pos_ki, float pos_kd,   // 位置环 PID 参数
    float spd_kp, float spd_ki, float spd_kd,   // 速度环 PID 参数
    uint8_t group = 1               // 组号：1=ID 1-4, 2=ID 5-8
);
```

**功能**：
- 级联 PID 控制（位置环 → 速度环 → 电流）
- 批量发送电流控制指令
- 无需创建电机实例

**参数说明**：
- `target_pos1-4`：目标位置（度）
- `current_pos1-4`：当前位置（度），从累计角度获取
- `current_spd1-4`：当前速度 (rpm)
- `pos_kp`：位置环比例系数，建议 0.5-1.5
- `spd_kp`：速度环比例系数，建议 10-20
- `pos_ki, pos_kd, spd_ki, spd_kd`：积分和微分系数（当前实现仅使用 P 控制）
- `group`：1=控制 ID 1-4 的电机，2=控制 ID 5-8 的电机

**返回值**：
- `true`：发送成功
- `false`：发送失败

**使用示例**：
```cpp
// 获取当前位置和速度
float pos1 = motor1.measure().total_angle;
float pos2 = motor2.measure().total_angle;
float pos3 = motor3.measure().total_angle;
float pos4 = motor4.measure().total_angle;

float spd1 = motor1.measure().speed_rpm;
float spd2 = motor2.measure().speed_rpm;
float spd3 = motor3.measure().speed_rpm;
float spd4 = motor4.measure().speed_rpm;

// 直接控制
M3508::controlPositionDirect(&g_can,
                            360.0f, 720.0f, 180.0f, 540.0f,  // 目标位置
                            pos1, pos2, pos3, pos4,          // 当前位置
                            spd1, spd2, spd3, spd4,          // 当前速度
                            0.8f, 0.0f, 0.0f,                // 位置环 PID
                            15.0f, 0.0f, 0.0f,               // 速度环 PID
                            1);                               // 组 1
```

---

## 对比：实例控制 vs 直接控制

### 实例控制（推荐用于复杂应用）

**优点**：
- 完整的 PID 控制（P+I+D）
- 自动管理状态和累计角度
- 支持多种控制模式切换
- 更精确的控制效果

**缺点**：
- 需要创建电机实例
- 需要调用 `init()` 注册回调
- 代码稍复杂

**示例**：
```cpp
M3508 motor(&g_can, 1);
motor.init();
motor.setSpeedPID(15.0f, 0.8f, 0.0f);
motor.setTargetSpeed(1000.0f);

while(1) {
    motor.update();
    HAL_Delay(1);
}
```

---

### 直接控制（推荐用于快速测试）

**优点**：
- 无需创建实例
- 代码简洁
- 适合快速测试
- 批量控制 4 个电机

**缺点**：
- 仅支持简化 P 控制
- 需要手动获取反馈数据
- 控制精度较低

**示例**：
```cpp
// 仍需创建实例来接收反馈
M3508 motor1(&g_can, 1);
M3508 motor2(&g_can, 2);
M3508 motor3(&g_can, 3);
M3508 motor4(&g_can, 4);

motor1.init();
motor2.init();
motor3.init();
motor4.init();

while(1) {
    float spd1 = motor1.measure().speed_rpm;
    float spd2 = motor2.measure().speed_rpm;
    float spd3 = motor3.measure().speed_rpm;
    float spd4 = motor4.measure().speed_rpm;
    
    M3508::controlSpeedDirect(&g_can,
                             1000.0f, -1000.0f, 500.0f, -500.0f,
                             spd1, spd2, spd3, spd4,
                             15.0f, 0.0f, 0.0f, 1);
    HAL_Delay(1);
}
```

---

## 注意事项

1. **反馈数据获取**：
   - 直接控制函数不会自动获取反馈
   - 仍需创建电机实例来接收和解析 CAN 反馈
   - 或者手动解析 CAN 数据

2. **PID 实现**：
   - 当前直接控制函数仅实现简化 P 控制
   - 如需完整 PID，建议使用实例控制方式
   - 未来可能会增强为完整 PID

3. **控制频率**：
   - 建议 1ms 调用一次
   - 频率过低会影响控制效果

4. **电流限幅**：
   - 自动限制在 ±16384 范围内
   - 无需手动限幅

5. **组号选择**：
   - `group=1`：控制 ID 1-4 的电机（CAN ID 0x200）
   - `group=2`：控制 ID 5-8 的电机（CAN ID 0x1FF）

---

## 使用建议

### 快速测试场景
使用直接控制函数：
```cpp
// 简单的速度控制测试
M3508::controlSpeedDirect(&g_can, 
                         1000, 1000, 1000, 1000,
                         spd1, spd2, spd3, spd4,
                         15.0f, 0, 0, 1);
```

### 正式应用场景
使用实例控制：
```cpp
motor1.setSpeedPID(15.0f, 0.8f, 0.0f);
motor1.setTargetSpeed(1000.0f);
motor1.update();
```

---

**作者**: RM2026 Team  
**日期**: 2025-12-02  
**版本**: v2.1
