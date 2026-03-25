# Referee Data 在当前框架中的应用分析

文档目的：

1. 基于当前 `Src` 整体框架，梳理裁判系统数据“已经解析到哪里、实际用到了哪里”。
2. 结合 `RoboMaster 2026 机甲大师高校系列赛通信协议 V1.0.0（2025-11-27）`，明确哪些字段应接入控制或状态逻辑。
3. 为后续接入底盘功率约束、发射热量约束、弹量约束和 UI 修正提供统一基线。

更新时间：2026-03-12

---

## 1. 结论摘要

当前框架对裁判系统的现状可以概括为：

1. `rm_referee.c` 已能解析多类 2026 协议数据，并写入 `referee_info_t`。
2. `Src/application/chassis/chassis.c` 会初始化裁判系统并持有 `referee_info_t *` 指针。
3. 正式控制链路中，裁判数据目前几乎没有形成闭环约束。
4. 当前唯一确定被正式消费的裁判字段是 `GameRobotState.robot_id`，它用于 UI 客户端 ID 计算与初始化等待。
5. UI 中显示的模式和“功率条”目前主要由 `RobotModeTest()` 测试逻辑驱动，不是由 `Src/application` 的真实应用状态或真实裁判功率数据驱动。
6. 按 2025-11-27 的 2026 协议版本，裁判系统不再直接提供实时 `chassis_power`，当前可直接用于底盘约束的核心字段是：
   `0x0201.chassis_power_limit`
   `0x0202.buffer_energy`

---

## 2. 当前代码中的裁判数据链路

### 2.1 初始化与解析链路

当前链路如下：

1. [chassis.c](/home/myself/workspace/RM2026/Src/application/chassis/chassis.c) 在 `ChassisInit()` 中调用 `UITaskInit(&CHASSIS_REFEREE_UART_HANDLE, &ui_data)`。
2. [referee_task.c](/home/myself/workspace/RM2026/lib/HNUYueLuRM/modules/referee/referee_task.c) 中 `UITaskInit()` 再调用 `RefereeInit()`。
3. [rm_referee.c](/home/myself/workspace/RM2026/lib/HNUYueLuRM/modules/referee/rm_referee.c) 中 `RefereeInit()` 完成串口注册、daemon 注册。
4. 裁判串口接收回调 `RefereeRxCallback()` 中调用 `JudgeReadData()`。
5. `JudgeReadData()` 根据 `CmdID` 把 payload 拷贝到 `referee_info_t` 的各字段。

也就是说：

1. 裁判系统在当前框架里是“由 chassis 初始化”的。
2. 数据存储中心是 `referee_info_t`。
3. 解析层与控制层之间目前没有进一步的数据分发抽象，仍然是 `chassis` 直接持有 `referee_data` 指针。

### 2.2 当前已解析到 `referee_info_t` 的数据

当前 `referee_info_t` 已包含：

1. `0x0001 GameState`
2. `0x0002 GameResult`
3. `0x0003 GameRobotHP`
4. `0x0101 EventData`
5. `0x0104 RefereeWarning`
6. `0x0105 DartInfo`
7. `0x0201 GameRobotState`
8. `0x0202 PowerHeatData`
9. `0x0203 GameRobotPos`
10. `0x0204 BuffMusk`
11. `0x0206 RobotHurt`
12. `0x0207 ShootData`
13. `0x0208 ProjectileAllowance`
14. `0x0209 RfidStatus`
15. `0x0301 ReceiveData`

对应定义见 [rm_referee.h](/home/myself/workspace/RM2026/lib/HNUYueLuRM/modules/referee/rm_referee.h)。

---

## 3. 当前框架里“实际被使用”的情况

### 3.1 正式被消费的字段

当前可以确认被正式逻辑消费的裁判字段很少，主要是：

1. `GameRobotState.robot_id`

具体用途：

1. 在 [referee_task.c](/home/myself/workspace/RM2026/lib/HNUYueLuRM/modules/referee/referee_task.c) 的 `DeterminRobotID()` 中，用于判断红蓝方和计算 UI 客户端 ID。
2. 在 `MyUIInit()` 中，用于等待裁判数据上线：`while (referee_recv_info->GameRobotState.robot_id == 0)`.

注意：

1. 当前客户端 ID 仍使用 `0x0100 + robot_id` 线性推导，这在 2026 协议下仍有风险，详见既有状态文档。

### 3.2 已留接口但未形成正式控制逻辑的字段

在 [chassis.c](/home/myself/workspace/RM2026/Src/application/chassis/chassis.c) 中，以下字段有明确预留意图，但未正式生效：

1. `PowerHeatData` 相关
2. `GameRobotState` 中的发射/功率相关字段

具体表现：

1. `LimitChassisOutput()` 注释中仍提到旧字段 `chassis_power` / `chassis_power_buffer`，但当前函数体没有任何限幅逻辑。
2. `ChassisTask()` 末尾注释区尝试从裁判系统取 `bullet_speed` 和 `rest_heat`，但仍未真正写入反馈链路。

因此当前结论是：

1. 底盘没有接入 `chassis_power_limit`。
2. 底盘没有接入 `buffer_energy`。
3. 发射没有接入 `shooter_barrel_cooling_value`、`shooter_barrel_heat_limit`、`shooter_heat0/1`、`ProjectileAllowance`。

### 3.3 当前 UI 与真实业务状态的关系

当前 UI 逻辑和真实业务状态之间存在明显脱节：

1. [referee_task.c](/home/myself/workspace/RM2026/lib/HNUYueLuRM/modules/referee/referee_task.c) 中 `UITask()` 每次都会先调用 `RobotModeTest()`。
2. `RobotModeTest()` 会周期性修改：
   `chassis_mode`
   `gimbal_mode`
   `shoot_mode`
   `friction_mode`
   `lid_mode`
   `Chassis_Power_Data.chassis_power_mx`
3. `MyUIRefresh()` 根据这些测试数据刷新 UI。

这意味着：

1. 当前 UI 上看到的模式和功率条，不代表 `Src/application` 中真实运行的模式。
2. 当前 UI 功率条也不代表真实裁判功率，也不代表真实 `buffer_energy`。
3. 如果后续要把裁判数据真正用于 UI，第一步应该去掉或条件编译 `RobotModeTest()`。

### 3.4 当前 `shoot`、`robot_cmd`、`gimbal` 中的使用情况

对 `Src/application` 全局检索结果如下：

1. [shoot.c](/home/myself/workspace/RM2026/Src/application/shoot/shoot.c) 当前没有直接引用裁判数据。
2. [robot_cmd.c](/home/myself/workspace/RM2026/Src/application/cmd/robot_cmd.c) 当前没有直接引用裁判数据。
3. `gimbal` 当前也没有直接消费裁判字段。

也就是说：

1. 当前裁判数据没有向 `shoot` 侧下沉。
2. 当前裁判数据没有向 `robot_cmd` 侧下沉。
3. 当前裁判数据没有形成跨应用的消息发布。

---

## 4. 协议口径下当前最有价值的数据

基于 `RoboMaster 2026 机甲大师高校系列赛通信协议 V1.0.0（2025-11-27）`，对当前框架最有价值的数据如下。

### 4.1 必须优先考虑接入的字段

#### 1. `0x0201 GameRobotState`

关键字段：

1. `robot_id`
2. `shooter_barrel_cooling_value`
3. `shooter_barrel_heat_limit`
4. `chassis_power_limit`
5. `power_management_gimbal_output`
6. `power_management_chassis_output`
7. `power_management_shooter_output`

原因：

1. `robot_id` 是裁判身份、UI 目标和阵营判断的基础。
2. `shooter_barrel_cooling_value` / `shooter_barrel_heat_limit` 是发射热量控制的上层限额。
3. `chassis_power_limit` 是当前协议下唯一直接提供的底盘功率上限。
4. `power_management_*_output` 可用于电源口状态联锁和异常提示。

#### 2. `0x0202 PowerHeatData`

关键字段：

1. `buffer_energy`
2. `shooter_heat0`
3. `shooter_heat1`

原因：

1. 当前协议中已无实时 `chassis_power`，`buffer_energy` 是底盘动态降额的重要依据。
2. `shooter_heat0/1` 是发射机构热量闭环的直接反馈。

#### 3. `0x0208 ProjectileAllowance`

关键字段：

1. `projectile_allowance_17mm`
2. `projectile_allowance_42mm`
3. `remaining_gold_coin`
4. `projectile_allowance_fortress`

原因：

1. 发射控制不能只看热量，还要看允许发弹量。
2. 这类数据适合直接驱动“禁止发弹/降速/提示”的上层逻辑。

### 4.2 推荐接入的字段

#### 1. `0x0001 GameState`

关键字段：

1. `game_progress`
2. `stage_remain_time`

作用：

1. 比赛未开始阶段自动上锁危险动作。
2. 倒计时阶段自动进入预热或待机。
3. UI 显示剩余时间和比赛阶段。

#### 2. `0x0207 ShootData`

关键字段：

1. `launching_frequency`
2. `initial_speed`
3. 发射机构编号

作用：

1. 统计真实射频和弹速。
2. 用于卡弹检测、拨盘行为验证、赛后分析。

#### 3. `0x0206 RobotHurt`

关键字段：

1. `armor_id`
2. `hurt_type`

作用：

1. 受击方向提示。
2. 撞击/掉线/被弹打等异常区分。
3. 可做被击后机动策略或日志。

#### 4. `0x0204 BuffMusk`

关键字段：

1. `cooling_buff`
2. `attack_buff`
3. `defence_buff`
4. `remaining_energy`

作用：

1. 热量冷却增益可直接影响发射节流策略。
2. 攻防增益适合联动上层策略或 UI。
3. 剩余能量可辅助操作手理解当前状态。

#### 5. `0x0209 RfidStatus`

作用：

1. 判断是否进入补给区/高地/基地等关键区域。
2. 可驱动区域提示或策略切换。

### 4.3 可选接入的字段

#### 1. `0x0203 GameRobotPos`

作用：

1. 小地图与记录分析。
2. 与自定位做交叉校验。

对于当前底盘与发射闭环，不属于首要项。

#### 2. `0x0003 GameRobotHP`

作用：

1. 己方血量、前哨站、基地状态展示。
2. 策略层状态感知。

更偏上层，不是控制闭环的首要依赖。

---

## 5. 当前框架中各字段的应用现状矩阵

### 5.1 当前最值得关心的数据矩阵

| 数据 ID | 关键字段 | 当前解析 | 当前正式消费 | 当前问题 | 推荐使用位置 |
| --- | --- | --- | --- | --- | --- |
| `0x0201` | `robot_id` | 已解析 | 已消费 | 客户端 ID 映射仍偏旧 | `referee_task` |
| `0x0201` | `chassis_power_limit` | 已解析 | 未消费 | 底盘限幅未接入 | `chassis` |
| `0x0201` | `shooter_barrel_cooling_value` | 已解析 | 未消费 | 发射冷却策略未接入 | `shoot` |
| `0x0201` | `shooter_barrel_heat_limit` | 已解析 | 未消费 | 热量保护未接入 | `shoot` |
| `0x0201` | `power_management_*_output` | 已解析 | 未消费 | 缺少电源口状态联锁 | `robot_cmd` / `UI` |
| `0x0202` | `buffer_energy` | 已解析 | 未消费 | 底盘动态降额未接入 | `chassis` |
| `0x0202` | `shooter_heat0/1` | 已解析 | 未消费 | 发射热量闭环未接入 | `shoot` |
| `0x0208` | `projectile_allowance_*` | 已解析 | 未消费 | 发弹量约束未接入 | `shoot` |
| `0x0001` | `game_progress` | 已解析 | 未消费 | 比赛阶段无自动联锁 | `robot_cmd` |
| `0x0207` | `initial_speed` / `launching_frequency` | 已解析 | 未消费 | 无实射验证 | `shoot` / 日志 |
| `0x0206` | `armor_id` / `hurt_type` | 已解析 | 未消费 | 无受击反馈和异常诊断 | `robot_cmd` / `UI` |
| `0x0204` | `cooling_buff` 等 | 已解析 | 未消费 | 无增益联动 | `shoot` / `UI` |
| `0x0209` | 区域状态 bit | 已解析 | 未消费 | 无区域策略联动 | `robot_cmd` / `UI` |

---

## 6. 这些数据在当前框架中“应该如何被使用”

下面按 `Src/application` 的实际模块边界给出建议。

### 6.1 `chassis` 应该使用的数据与场景

建议接入字段：

1. `GameRobotState.chassis_power_limit`
2. `PowerHeatData.buffer_energy`
3. 可选：`power_management_chassis_output`

推荐场景：

1. 根据 `chassis_power_limit` 对底盘输出做基准功率预算。
2. 根据 `buffer_energy` 做动态降额，缓冲低时更保守，缓冲高时允许更激进输出。
3. 若 `power_management_chassis_output == 0`，触发安全停机或禁止使能。

推荐落点：

1. [chassis.c](/home/myself/workspace/RM2026/Src/application/chassis/chassis.c) 的 `LimitChassisOutput()`

当前缺口：

1. `LimitChassisOutput()` 仍是空实现。
2. 注释仍引用旧版 `chassis_power/chassis_power_buffer` 字段，需要按新协议改口径。

### 6.2 `shoot` 应该使用的数据与场景

建议接入字段：

1. `GameRobotState.shooter_barrel_cooling_value`
2. `GameRobotState.shooter_barrel_heat_limit`
3. `PowerHeatData.shooter_heat0`
4. `PowerHeatData.shooter_heat1`
5. `ProjectileAllowance.projectile_allowance_17mm`
6. `ProjectileAllowance.projectile_allowance_42mm`
7. 可选：`ShootData.launching_frequency`
8. 可选：`ShootData.initial_speed`
9. 可选：`BuffMusk.cooling_buff`

推荐场景：

1. 当剩余热量不足时，禁止拨盘进入连发或降低发射频率。
2. 当允许发弹量为 0 时，禁止拨盘发射。
3. 根据冷却值和热量上限动态调整 `shoot_rate` 上限。
4. 用 `ShootData` 验证“是否真的打出弹丸”，辅助做卡弹检测和拨盘故障判断。

推荐落点：

1. [shoot.c](/home/myself/workspace/RM2026/Src/application/shoot/shoot.c) 主状态机

当前缺口：

1. `shoot.c` 当前完全不读裁判数据。
2. 现在的发射逻辑主要是命令直达，不受热量和弹量约束。

### 6.3 `robot_cmd` 应该使用的数据与场景

建议接入字段：

1. `GameState.game_progress`
2. `GameState.stage_remain_time`
3. `power_management_gimbal_output`
4. `power_management_shooter_output`
5. 可选：`RfidStatus`
6. 可选：`RobotHurt`
7. 可选：`BuffMusk`

推荐场景：

1. 比赛未开始阶段自动锁住危险动作。
2. 当某一 24V 输出关闭时，对应应用进入安全模式。
3. 基于 RFID 区域状态做策略提示或模式切换。
4. 基于受击事件做反馈提示或上层状态改变。

推荐落点：

1. [robot_cmd.c](/home/myself/workspace/RM2026/Src/application/cmd/robot_cmd.c)

当前缺口：

1. `robot_cmd` 当前不持有裁判数据。
2. 没有统一的 `referee_feed` 消息中心话题。

### 6.4 UI 应该使用的数据与场景

建议接入字段：

1. 应用真实模式：`chassis/gimbal/shoot/friction/lid`
2. 底盘真实约束或真实状态：`chassis_power_limit`、`buffer_energy`、超电功率或估算功率
3. 发射热量与弹量
4. 比赛阶段与剩余时间
5. RFID / buff / hurt 等状态

推荐场景：

1. 显示真实模式，不再显示测试模式。
2. 显示底盘“功率上限/缓冲能量/估算功率”，而不是虚构的 `chassis_power_mx`。
3. 显示热量剩余、弹量剩余、当前区域或增益状态。

当前缺口：

1. 当前 UI 没有绑定真实业务数据。
2. 现有 `Chassis_Power_Data_s` 命名仍是旧思路，不适合直接表达 2026 协议语义。

---

## 7. 建议的数据流重构方向

当前最大结构性问题不是“没解析”，而是“解析后没有统一分发”。

建议方向：

1. 保留 `rm_referee.c` 作为唯一解析入口。
2. 新增一个统一的 `referee_feed` 发布话题，周期性发布当前需要下沉给应用层的裁判关键数据。
3. `chassis`、`shoot`、`robot_cmd` 各自订阅自己真正关心的子集。

建议最小化发布内容：

1. `robot_id`
2. `game_progress`
3. `stage_remain_time`
4. `chassis_power_limit`
5. `buffer_energy`
6. `shooter_barrel_cooling_value`
7. `shooter_barrel_heat_limit`
8. `shooter_heat0`
9. `shooter_heat1`
10. `projectile_allowance_17mm`
11. `projectile_allowance_42mm`
12. `power_management_*_output`

这样做的好处：

1. `chassis` 不必直接依赖完整裁判模块内部结构。
2. `shoot` 可以在不直接 include `referee_task.h` 的情况下接入限额。
3. 后续协议字段变化时，影响范围更可控。

---

## 8. 推荐落地顺序

### P0

1. 修正文档与代码对 2026 协议的功率字段认知，统一改为：
   `chassis_power_limit + buffer_energy`
2. 去掉或条件编译 `RobotModeTest()`，避免 UI 继续显示虚假状态。
3. 在 `chassis` 中接入：
   `chassis_power_limit`
   `buffer_energy`
4. 在 `shoot` 中接入：
   `shooter_barrel_cooling_value`
   `shooter_barrel_heat_limit`
   `shooter_heat0/1`
   `ProjectileAllowance`

### P1

1. 在 `robot_cmd` 中接入 `game_progress` 和 `power_management_*_output`。
2. 把 `ShootData` 接入发射验证和日志。
3. 把 `RobotHurt` / `RfidStatus` / `BuffMusk` 接入 UI 与策略提示。

### P2

1. 抽象出统一的 `referee_feed` 消息。
2. 将 UI 完全改为使用真实应用状态和真实裁判状态。

---

## 9. 当前阶段的明确判断

对当前 `Src` 整体框架，可以做出以下明确判断：

1. 裁判模块的“接收与解析”已经基本到位。
2. 裁判模块的“控制侧消费”仍基本空缺。
3. 当前最值得优先接入的不是全部协议数据，而是：
   `0x0201`
   `0x0202`
   `0x0208`
   其次是 `0x0001`
4. 当前 UI 逻辑带有明显测试残留，不能把它当作裁判数据已接入业务的证据。
5. 在 2026 协议口径下，底盘功率约束设计应围绕：
   `最大功率限制 + 缓冲能量`
   而不是旧版的 `实时功率 + 缓冲`

