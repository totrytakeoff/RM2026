# infantry 迁移总结（裸机 demo -> Src/FreeRTOS）

## 文档目的

记录本次 infantry（全向轮步兵，单板优先）从裸机 demo 向 `Src/application` 正式框架迁移的结果、要点、限制和当前可上车状态，作为后续联调与维护基线。

## 迁移范围

1. 目标：`test/infantry_omni_demo` 核心链路迁移到 `Src/application`。
2. 边界：本阶段聚焦单板全向轮步兵；双板保留接口；wheelleg 不在本阶段。
3. 主线：以 `robot_def.h` 为统一配置入口，以 `robot_cmd` 输入后端化为主架构改造点。

## 已完成的核心迁移

### 1) 配置层收口（`robot_def.h`）

1. 输入源与接口配置：
   - `ROBOT_CMD_INPUT_SOURCE`（DT7/ET08/DATALINK）
   - 遥控/视觉串口、裁判串口、双板 CANComm 接口宏
2. CAN/ID 基线迁移：
   - 底盘、云台、发射全部 ID 和 CAN 句柄按 infantry demo 对齐
3. 控制参数收口：
   - ET08/DT7 输入映射参数
   - 底盘模式参数（follow gain/自旋参考）
   - 云台限幅开关
   - 发射状态机与摩擦轮目标参数
4. 闭环参数收口：
   - 底盘 PID
   - 云台 PID
   - 发射 PID + Improve 组合 + 初始化 outer/close loop
   - 发射运行态各模式外环策略（冷却、停转、单发、三发、连发、反转）

### 2) robot_cmd 重构（输入后端化）

1. `robot_cmd` 主体职责固定为：
   - 拉取反馈
   - 调用输入后端生成统一命令
   - 急停仲裁
   - 发布命令
2. 新增输入后端目录：
   - `cmd/input/input_et08.c`
   - `cmd/input/input_dt7.c`
   - `cmd/input/input_datalink.c`
3. 当前状态：
   - ET08：可用
   - DT7：可编译可用
   - DATALINK：占位（默认急停）

### 3) 各应用按 demo 对齐

1. `chassis`：
   - CAN/ID 由 `robot_def` 统一配置
   - 跟随/自旋参数可配
   - 电机方向可配
2. `gimbal`：
   - CAN/ID 与 PID 参数可配
   - pitch 限幅接口已预留（可开关）
3. `shoot`：
   - 单发/三发冷却、连发斜坡、弹速档位映射迁入
   - PID 与策略参数可配
   - 关键流程与 demo 行为对齐

### 4) 迁移期框架问题修复

1. `message_center` 队列索引未定义行为修复。
2. `daemon` 初始化计数覆盖问题修复。
3. `shoot` 任务 `now` 变量作用域编译错误修复。

## 当前框架现状（2026-02-27）

### 已具备

1. 单板全链路主框架可编译（`app.elf` 持续通过）。
2. ET08 主控链路 + chassis/gimbal/shoot 控制主干可联调。
3. 参数入口高度集中，后续以调参为主，不需要大改架构。

### 仍是限制/占位

1. `input_datalink` 未接入协议，选用时会默认急停。
2. 发射舱盖动作逻辑仍为占位。
3. `LOAD_REVERSE` 反转/退弹逻辑仍为占位。
4. 双板仅保留接口，尚未完成本阶段实机联调验收。

## 是否可上车直接跑

结论：可以进入上车联调阶段，但不是“所有功能零改动即满功能可用”。

1. 对于“单板全向步兵 + ET08 + 底盘/云台/基础发射”可上车联调。
2. 期望快速回归 demo 稳定功能，主要工作是实机调参而非重构。
3. 若要使用图传链路、完整弹舱盖、反卡弹退弹，还需补功能实现。

## 上车前建议最小检查清单

1. `robot_def.h`：
   - 板型宏（`ONE_BOARD`）
   - 输入源宏（`ROBOT_CMD_INPUT_SOURCE`）
   - CAN/ID 与电机方向
2. 云台：
   - yaw/pitch 零点参数
   - pitch 限幅开关与上下限
3. 发射：
   - 摩擦轮目标转速档位
   - 拨盘 PID 与冷却时间
4. 底盘：
   - 轮向与速度比例
   - 跟随/自旋参数

详细调参项和默认值说明见：

1. `Src/application/infantry_onboard_tuning_checklist.md`

## 维护建议

1. 后续新增车型时，优先改 `robot_def.h`，尽量不改业务流程代码。
2. 新输入链路统一按 `cmd/input` 后端接口接入，避免污染 `robot_cmd.c`。
3. 在功能新增阶段保持“先等价迁移，再优化增强”的节奏。

---

落款：YZControl/myself  
日期：2026-02-27
