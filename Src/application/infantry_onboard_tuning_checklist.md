# infantry 上车调参清单（底盘/云台/发射）

## 适用范围

1. 车型：全向轮步兵（infantry omni）
2. 架构：`Src/application` + FreeRTOS
3. 板型：`ONE_BOARD`
4. 输入：ET08 主链路（`ROBOT_CMD_INPUT_SOURCE = ROBOT_CMD_INPUT_SRC_ET08`）

## 使用说明

1. 本清单基于当前 `robot_def.h` 默认值编写。
2. 建议按“底盘 -> 云台 -> 发射”顺序上车调参。
3. 每次仅调整一组参数，留存变更记录并做 A/B 对比。

## 一、上车前基础项（必须先确认）

| 项目 | 宏 | 当前默认值 | 说明 |
|---|---|---:|---|
| 板型 | `ONE_BOARD` | 已启用 | 当前按单板整车联调 |
| 输入源 | `ROBOT_CMD_INPUT_SOURCE` | `ROBOT_CMD_INPUT_SRC_ET08` | 主控链路使用 ET08 |
| 底盘 CAN/ID | `CHASSIS_CAN_HANDLE` + `CHASSIS_MOTOR_*_ID` | `hcan1` + `1/2/4/3` | 与 infantry demo 基线一致 |
| 云台 CAN/ID | `GIMBAL_*_CAN_HANDLE` + `GIMBAL_*_MOTOR_ID` | yaw:`hcan1,id1` pitch:`hcan2,id1` | 与 demo 基线一致 |
| 发射 CAN/ID | `SHOOT_*_CAN_HANDLE` + `SHOOT_*_MOTOR_ID` | friction:`hcan2,id1/2` loader:`hcan2,id6` | 与 demo 基线一致 |
| 图传输入后端 | `input_datalink.c` | 占位急停 | 当前未实现，不能作为可用输入源 |

## 二、底盘调参清单

### 2.1 输入映射与手感

| 宏 | 当前默认值 | 调参方向 | 异常现象 |
|---|---:|---|---|
| `CMD_ET08_STICK_SCALE_DEN` | `660.0f` | 减小更灵敏，增大更平滑 | 太小会“窜车”，太大响应迟钝 |
| `CMD_ET08_CHASSIS_RC_DEADZONE` | `50` | 增大抑制漂移，减小提响应 | 静止抖动/慢爬通常死区太小 |
| `CMD_ET08_CHASSIS_VEL_SCALE` | `10.0f` | 线速度总增益 | 直线速度不足/过猛 |
| `CMD_ET08_CHASSIS_WZ_SCALE` | `6.0f` | 旋转速度总增益 | 转向过快/过慢 |

### 2.2 模式参数

| 宏 | 当前默认值 | 调参方向 | 异常现象 |
|---|---:|---|---|
| `CHASSIS_FOLLOW_WZ_GAIN` | `1.5f` | 跟随力度增益 | 跟随过软或过冲 |
| `CHASSIS_ROTATE_WZ_REF` | `4000.0f` | 小陀螺角速度 | 自旋太慢或过激 |

### 2.3 电机方向与 PID

| 宏 | 当前默认值 | 说明 |
|---|---:|---|
| `CHASSIS_MOTOR_LF_DIR` | `MOTOR_DIRECTION_REVERSE` | 轮向错误先改方向宏，不要先改控制符号 |
| `CHASSIS_MOTOR_RF_DIR` | `MOTOR_DIRECTION_REVERSE` | 同上 |
| `CHASSIS_MOTOR_LB_DIR` | `MOTOR_DIRECTION_REVERSE` | 同上 |
| `CHASSIS_MOTOR_RB_DIR` | `MOTOR_DIRECTION_REVERSE` | 同上 |
| `CHASSIS_SPEED_PID_KP/KI/KD` | `10/0/0` | 速度环默认偏保守 |
| `CHASSIS_CURRENT_PID_KP/KI/KD` | `0.5/0/0` | 电流环默认偏保守 |
| `CHASSIS_SPEED_PID_MAX_OUT` | `12000` | 输出上限 |
| `CHASSIS_CURRENT_PID_MAX_OUT` | `15000` | 输出上限 |

## 三、云台调参清单

### 3.1 零点与限位（优先级最高）

| 宏 | 当前默认值 | 说明 |
|---|---:|---|
| `YAW_CHASSIS_ALIGN_ECD` | `2711` | 底盘正向与云台正向对齐参考 |
| `PITCH_HORIZON_ECD` | `3412` | pitch 水平参考 |
| `GIMBAL_PITCH_LIMIT_ENABLE` | `0u` | 当前关闭软件限位 |
| `PITCH_MAX_ANGLE` | `0` | 仅在限位开启时生效 |
| `PITCH_MIN_ANGLE` | `0` | 仅在限位开启时生效 |

### 3.2 输入映射

| 宏 | 当前默认值 | 调参方向 | 异常现象 |
|---|---:|---|---|
| `CMD_ET08_GIMBAL_RC_DEADZONE` | `50` | 增大抑制漂移 | 云台静止抖动 |
| `CMD_ET08_GIMBAL_PITCH_SCALE` | `0.001f` | pitch 手感增益 | 俯仰过慢/过快 |
| `CMD_ET08_GIMBAL_YAW_STEP` | `0.4f` | yaw 步进增益 | 左右摇头慢/快 |

### 3.3 云台 PID

| 宏 | 当前默认值 | 说明 |
|---|---:|---|
| `GIMBAL_YAW_ANGLE_PID_KP/KI/KD` | `8/0/0` | yaw 角度环 |
| `GIMBAL_YAW_SPEED_PID_KP/KI/KD` | `50/200/0` | yaw 速度环 |
| `GIMBAL_PITCH_ANGLE_PID_KP/KI/KD` | `10/0/0` | pitch 角度环 |
| `GIMBAL_PITCH_SPEED_PID_KP/KI/KD` | `50/350/0` | pitch 速度环 |

## 四、发射调参清单

### 4.1 机械参数（必须正确）

| 宏 | 当前默认值 | 说明 |
|---|---:|---|
| `ONE_BULLET_DELTA_ANGLE` | `45` | 单发拨盘角度增量 |
| `REDUCTION_RATIO_LOADER` | `49.0f` | 拨盘减速比 |
| `NUM_PER_CIRCLE` | `10` | 单圈装弹量 |

### 4.2 发射状态机与速度参数

| 宏 | 当前默认值 | 调参方向 | 异常现象 |
|---|---:|---|---|
| `SHOOT_LOADER_SINGLE_DEADTIME_MS` | `150.0f` | 单发不应期 | 单发连跳或响应迟滞 |
| `SHOOT_LOADER_THREE_DEADTIME_MS` | `300.0f` | 三发不应期 | 三发重复触发 |
| `SHOOT_LOADER_CONTINUOUS_SLEW_PER_MS` | `40.0f` | 连发斜坡 | 连发突变/拖沓 |
| `SHOOT_FRICTION_SPEED_DEFAULT` | `30000.0f` | 默认摩擦轮目标 | 弹速不足或过高 |
| `SHOOT_FRICTION_SPEED_15/18/30` | `30000/30000/30000` | 各档位独立调 | 档位无区分感 |

### 4.3 发射 PID 与策略（当前默认较保守）

| 宏 | 当前默认值 | 说明 |
|---|---:|---|
| `SHOOT_FRICTION_SPEED_PID_KP/KI/KD` | `0/0/0` | 摩擦轮速度环，需实车补参 |
| `SHOOT_FRICTION_CURRENT_PID_KP/KI/KD` | `0/0/0` | 摩擦轮电流环，需实车补参 |
| `SHOOT_LOADER_ANGLE_PID_KP/KI/KD` | `0/0/0` | 拨盘角度环，需实车补参 |
| `SHOOT_LOADER_SPEED_PID_KP/KI/KD` | `0/0/0` | 拨盘速度环，需实车补参 |
| `SHOOT_LOADER_CURRENT_PID_KP/KI/KD` | `0/0/0` | 拨盘电流环，需实车补参 |
| `SHOOT_*_PID_IMPROVE` | 已配置 | Improve 组合已可通过宏切换 |
| `SHOOT_*_INIT_*_LOOP` | 已配置 | 初始化 outer/close loop 策略可配 |
| `SHOOT_LOADER_LOOP_*` | 已配置 | 运行态模式外环策略可配 |

## 五、建议调参顺序（上车执行）

1. 检查 CAN/ID 与电机方向，确保“方向正确再调 PID”。
2. 调底盘输入比例与死区，先达到“可稳停、可控速”。
3. 校云台零点，再调云台手感比例和 PID。
4. 最后调发射：先机械参数，再摩擦轮转速，再拨盘 PID。
5. 每次改动仅改一组参数，记录变更前后表现。

## 六、当前默认值说明（风险提示）

1. 发射相关 PID 当前多数为 `0`，这是迁移期保守默认值，不代表可直接比赛使用。
2. 若只做“功能回归”，可先保持现值完成链路验证，再进入实射调参。
3. 若启用 `GIMBAL_PITCH_LIMIT_ENABLE`，必须同步填写有效 `PITCH_MIN/MAX_ANGLE`。

---

落款：YZControl/myself  
日期：2026-02-27
