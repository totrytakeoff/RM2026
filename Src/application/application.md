# application

application 层负责整车业务逻辑，核心结构为并行 app + pub/sub 通信。

## 当前结构

`robot.c` 统一初始化并调度 4 个核心应用：

1. `robot_cmd`
2. `chassis`
3. `gimbal`
4. `shoot`

各应用通过 `message_center` 发布/订阅消息，不直接相互包含。

## 当前迁移结论（infantry）

本阶段已完成全向轮步兵从裸机 demo 到 `Src/application` 的主链路迁移：

1. 配置层统一到 `robot_def.h`（输入源、CAN/ID、PID、策略宏）。
2. `robot_cmd` 完成输入后端化（ET08/DT7/图传占位）。
3. `chassis/gimbal/shoot` 关键控制与参数入口完成收口。
4. 可进入单板上车联调与调参阶段。

详细状态与限制见：

- `Src/application/infantry_freertos_migration_summary.md`
- `Src/application/infantry_onboard_tuning_checklist.md`

## 任务频率建议

1. `INS`：1kHz（必需）
2. `MotorTask`：200Hz~1kHz（按总线负载权衡）
3. `RobotTask`：建议 >=150Hz
4. `Daemon/Monitor`：100Hz 左右

## 单板/双板说明

板型由 `robot_def.h` 宏控制：

1. `ONE_BOARD`
2. `CHASSIS_BOARD`
3. `GIMBAL_BOARD`

双板通信当前保留 CANComm 接口配置；本阶段以单板为优先验收目标。

## 开发原则

1. 参数优先改 `robot_def.h`，少改业务流程代码。
2. 新输入链路接入 `cmd/input/*` 后端，不直接写进 `robot_cmd.c`。
3. 应用间通信仅使用 `message_center`。

---

落款：YZControl/myself  
日期：2026-02-27
