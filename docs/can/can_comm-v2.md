# CAN 通信与电机控制优化更新日志

## 版本 v2.0 - 2025-12-02

### 重大更新

#### 1. CAN 通信封装优化 (`src/drivers/protocol/can_comm.*`)

**新增功能：**
- ✅ 支持多个回调函数注册（最多 16 个）
- ✅ 支持基于 CAN ID 的回调过滤
- ✅ 回调管理使用静态内存池（无动态分配）
- ✅ 链表结构管理回调节点

**新增 API：**
```cpp
bool registerRxCallback(RxCallback cb, void* user = nullptr);
bool registerRxCallback(RxCallback cb, uint32_t filter_id, bool is_ext_id, void* user = nullptr);
bool unregisterRxCallback(RxCallback cb);
void clearAllCallbacks();
uint8_t getCallbackCount() const;
```

**废弃 API：**
- ~~`attachRxCallback()`~~ → 使用 `registerRxCallback()` 替代

#### 2. M3508 电机控制增强 (`src/drivers/motor/m3508.*`)

**新增功能：**
- ✅ 速度环闭环控制（PID）
- ✅ 位置环闭环控制（级联 PID）
- ✅ 累计角度计算（支持多圈）
- ✅ 自动注册 CAN 回调
- ✅ 三种控制模式切换

**新增 API：**
```cpp
// 初始化
bool init();

// 控制模式
void setControlMode(ControlMode mode);
void setTargetSpeed(float target_rpm);
void setTargetPosition(float target_angle);
bool update(float dt = 0.0f);

// PID 配置
void setSpeedPID(float kp, float ki, float kd);
void setPositionPID(float kp, float ki, float kd);
void reset();

// 状态查询
ControlMode getControlMode() const;
float getCurrentSpeed() const;
float getCurrentPosition() const;
int16_t getLastCurrent() const;
```

**数据结构更新：**
```cpp
struct Measure {
    uint16_t ecd;           // 编码器原始值 (0-8191)
    int16_t  speed_rpm;     // 速度 (rpm)
    int16_t  given_current; // 实际转矩电流
    uint8_t  temperature;   // 温度
    uint16_t last_ecd;      // 上次编码器值
    int32_t  total_angle;   // 累计角度（度）- 新增
    int32_t  total_rounds;  // 累计圈数 - 新增
};

enum class ControlMode {
    OPEN_LOOP = 0,      // 开环控制
    SPEED_LOOP = 1,     // 速度环控制
    POSITION_LOOP = 2   // 位置环控制
};
```

#### 3. PID 控制器修复 (`src/common/pid/pid_controller.cpp`)

**修复：**
- ✅ 修正 HAL 头文件引用（stm32f1xx → stm32f4xx）
- ✅ 使用 C++ 标准头文件（math.h → cmath）

#### 4. 测试程序

**新增文件：**
- `src/test/motor/M3508_advanced_demo.cpp` - 完整的测试程序
  - 开环控制测试
  - 速度环控制测试
  - 位置环控制测试
  - 多电机协同测试

#### 5. 文档

**新增文档：**
- `docs/motor/m3508_advanced_usage.md` - 详细使用指南
- `docs/CHANGELOG_CAN_MOTOR.md` - 本更新日志

### 使用示例

#### 基本速度环控制

```cpp
// 创建对象
CanBus can1(&hcan1);
M3508 motor(&can1, 1);

// 初始化
motor.init();
motor.setSpeedPID(15.0f, 0.8f, 0.0f);

// 设置目标速度
motor.setTargetSpeed(1000.0f);  // 1000 rpm

// 主循环
while(1) {
    while(can1.pollOnce());  // 轮询 CAN
    motor.update();          // 更新控制器
    HAL_Delay(1);
}
```

#### 位置环控制

```cpp
// 配置 PID
motor.setPositionPID(0.8f, 0.0f, 0.2f);
motor.setSpeedPID(15.0f, 0.8f, 0.0f);

// 设置目标位置
motor.setTargetPosition(360.0f);  // 转到 360 度

// 主循环
while(1) {
    while(can1.pollOnce());
    motor.update();
    HAL_Delay(1);
}
```

#### 多电机控制

```cpp
M3508 motor1(&can1, 1);
M3508 motor2(&can1, 2);

motor1.init();
motor2.init();

motor1.setTargetSpeed(1000.0f);
motor2.setTargetPosition(360.0f);

while(1) {
    while(can1.pollOnce());
    motor1.update();
    motor2.update();
    HAL_Delay(1);
}
```

### 兼容性说明

**向后兼容：**
- ✅ 原有的 `setCurrent()` 开环控制仍然可用
- ✅ 原有的 `sendCurrentGroup()` 静态方法保持不变
- ✅ 原有的 `parseFeedback()` 方法保持不变

**不兼容变更：**
- ⚠️ `attachRxCallback()` 已废弃，请使用 `registerRxCallback()`
- ⚠️ 需要调用 `motor.init()` 来注册 CAN 回调

### 性能优化

- 回调过滤机制减少不必要的回调调用
- 静态内存池避免动态分配开销
- PID 计算优化，支持自定义采样时间

### 已知限制

1. 最多支持 16 个 CAN 回调
2. 当前实现不是线程安全的（RTOS 环境需要加锁）
3. 位置环不支持多圈限位（需要上层应用处理）

### 下一步计划

- [ ] 添加速度前馈控制
- [ ] 实现电流环控制
- [ ] 添加轨迹规划功能
- [ ] 支持 GM6020 云台电机的位置环
- [ ] 添加电机状态监控和保护

### 测试状态

- ✅ CAN 多回调注册/注销
- ✅ CAN ID 过滤
- ✅ M3508 速度环控制
- ✅ M3508 位置环控制
- ✅ 累计角度计算
- ⏳ 实际硬件测试（待进行）

### 贡献者

- AI Assistant (Cascade)
- RM2026 Team

---

**注意**: 在实际硬件上测试前，请先在仿真环境或低电流下验证控制参数。
