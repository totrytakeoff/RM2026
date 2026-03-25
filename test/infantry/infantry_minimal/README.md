存疑:

1.全速度环,没位置环?


# Infantry 最小框架

## 概述

这是一个从 `infantry_omni_demo`提取核心逻辑构建的最小化步兵控制框架。设计目标：

- **稳定性优先**: 移除消息中心等中间层，直接函数调用
- **简洁明了**: 所有配置集中在一个文件，数据流清晰可见
- **易于调试**: 单一主循环，无RTOS任务，无复杂依赖链
- **双输入源支持**: 支持ET08遥控器和VT图传键鼠，通过配置切换

## 架构对比

### 原框架问题

```
robot_def.h (400+行配置)
    ↓
robot.c/h (入口层)
    ↓
robot_task.c/h (FreeRTOS 5任务)
    ↓
application层 (chassis/gimbal/shoot/robot_cmd)
    ↓
modules层 (motor/message_center/daemon/referee)
    ↓
消息中心pub-sub → 数据流不直观，调试困难
```

### 最小框架

```
minimal_config.h (所有参数集中)
    ↓
main.c (单一主循环 5ms)
    ↓
┌─────────────┬─────────────┬─────────────┐
│ chassis     │ gimbal      │ shoot       │
│ (4x M3508)  │ (GM6020x2)  │ (M3508x3)   │
└─────────────┴─────────────┴─────────────┘
    ↓
dji_motor (电机驱动)
    ↓
CAN总线
```

## 文件结构

```
infantry_minimal/
├── main.c              # 主入口，单一while(1)循环
├── minimal_config.h    # 所有参数配置 + 控制源选择
├── minimal_types.h     # 类型定义
├── minimal_input.c/h   # 统一输入模块(ET08/VT)
├── minimal_chassis.c/h # 底盘控制(麦轮)
├── minimal_gimbal.c/h  # 云台控制(跟随/分离模式)
├── minimal_shoot.c/h   # 发射控制(摩擦轮/拨盘)
└── CMakeLists.txt      # 构建配置
```

## 控制源选择

在 `minimal_config.h` 中修改 `INPUT_SOURCE` 宏：

```c
#define INPUT_SOURCE_ET08       0U      // ET08遥控器
#define INPUT_SOURCE_VT         1U      // VT图传键鼠

#define INPUT_SOURCE            INPUT_SOURCE_VT  // 当前使用的控制源
```

## 控制映射

### VT图传键鼠 (INPUT_SOURCE_VT)

| 按键/鼠标      | 功能                  |
| -------------- | --------------------- |
| W/S            | 底盘前后              |
| A/D            | 底盘左右              |
| Q/E            | 底盘旋转              |
| Shift          | 加速模式              |
| Ctrl           | 慢速模式              |
| 鼠标X          | 云台Yaw               |
| 鼠标Y          | 云台Pitch             |
| F              | 切换云台跟随/分离模式 |
| R              | 切换摩擦轮开关        |
| 鼠标左键       | 单发                  |
| 鼠标中键       | 双发                  |
| 鼠标右键(按住) | 连发                  |
| Pause          | 急停                  |

**档位说明**：

- C档: 停止状态
- N档: 停止状态
- S档: 响应键鼠控制

**摇杆兜底**：当键盘无输入时，右摇杆控制底盘移动

### ET08遥控器 (INPUT_SOURCE_ET08)

| 摇杆/开关 | 功能              |
| --------- | ----------------- |
| 左摇杆X   | 底盘横移          |
| 左摇杆Y   | 底盘前后          |
| 右摇杆X   | 云台Yaw           |
| 右摇杆Y   | Pitch手动控制     |
| SA开关    | 摩擦轮 ON/OFF     |
| SB开关    | 拨盘 连发/停/单发 |
| SD开关    | 云台跟随/分离模式 |

## 云台模式

### 跟随模式 (GIMBAL_FOLLOW_CHASSIS)

- 云台随底盘旋转
- 使用Yaw电机编码器闭环
- 底盘旋转时自动补偿，保持相对角度

### 分离模式 (GIMBAL_SEPARATE)

- 云台独立于底盘
- 预留IMU闭环接口
- 不随底盘移动，始终朝向指定方向
- 为后续底盘小陀螺打下基础

**切换方式**：

- VT键鼠: 按F键切换
- ET08遥控: SD开关切换

## 快速使用

1. **修改配置**: 编辑 `minimal_config.h`

   - 修改CAN ID映射
   - 调整PID参数
   - 调整速度限制
2. **编译**:

   ```bash
   cd build
   cmake .. -DTEST_TARGET=infantry_minimal
   make
   ```
3. **烧录**: 生成的bin文件在 `build/test/infantry/infantry_minimal/`

## 核心逻辑流程

```
main()
  ├── HAL_Init()
  ├── DWT_Init()
  ├── Chassis_Init()    # 初始化4个M3508
  ├── Gimbal_Init()     # 初始化2个GM6020
  ├── Shoot_Init()      # 初始化3个M3508
  ├── RC_Init()         # 初始化ET08
  │
  └── while(1) {  // 5ms周期
        RC_GetData()
        ├── 遥控离线 → 所有模块停止
        └── 遥控在线
              Chassis_Update()  // 麦轮运动学+PID
              Gimbal_Update()   // Pitch保持+重力补偿
              Shoot_Update()    // 摩擦轮+拨盘控制
      }
```

## 与原框架差异

| 方面     | 原框架             | 最小框架             |
| -------- | ------------------ | -------------------- |
| 任务调度 | FreeRTOS 5任务     | 单一while循环        |
| 数据传递 | 消息中心pub-sub    | 直接函数调用         |
| 配置管理 | robot_def.h宏开关  | minimal_config.h参数 |
| 裁判系统 | 集成UI/裁判通信    | 暂未集成             |
| 调试难度 | 中间层多，追踪困难 | 数据流直接，易于调试 |

## 待完善

1. **裁判系统集成**: 后续可添加minimal_referee.c/h
2. **UI显示**: 后续可添加minimal_ui.c/h
3. **IMU集成**: 如需云台姿态控制，添加minimal_imu.c/h

## 依赖

- `HNUYueLuRM_modules`: dji_motor, et08_remote
- `HNUYueLuRM_bsp`: bsp_dwt, bsp_log, bsp_can
- `HNUYueLuRM_common`: user_lib
- `HNUYueLuRM_drivers`: HAL驱动
