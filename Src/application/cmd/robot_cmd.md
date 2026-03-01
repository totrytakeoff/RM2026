# robot_cmd

## 模块职责

`robot_cmd` 是整车控制中枢，职责固定为：

1. 拉取 `chassis/gimbal/shoot` 反馈。
2. 根据当前输入后端生成统一控制命令。
3. 计算底盘-云台偏航偏差（offset）。
4. 做急停仲裁与恢复仲裁。
5. 发布控制命令到各应用（双板时底盘命令走 CANComm）。

## 输入后端架构（当前版本）

`robot_cmd` 已从“内嵌遥控逻辑”重构为“后端插件式输入”：

1. `input_et08.c`：主用链路，当前可用。
2. `input_dt7.c`：兼容链路，当前可用。
3. `input_datalink.c`：图传占位，当前默认急停。

入口文件：

1. `cmd_input.h`
2. `cmd_input.c`

选择方式：`robot_def.h` 中 `ROBOT_CMD_INPUT_SOURCE`。

## 当前运行流程

`RobotCMDTask()` 主流程：

1. 获取反馈消息。
2. 调用输入后端 `RobotCMDInputUpdate()` 生成统一命令。
3. `CalcOffsetAngle()` 计算底盘偏转角。
4. `EmergencyHandler()` 做安全裁决。
5. 发布命令（单板 pub/sub，双板底盘命令通过 CANComm）。

## 迁移后维护建议

1. 新控制链路统一接入 `cmd/input`，不要在 `robot_cmd.c` 写协议细节。
2. 默认参数、接口句柄、模式映射统一写入 `robot_def.h`。
3. 紧急状态只由 `EmergencyHandler()` 统一裁决，避免多处冲突停机逻辑。

---

落款：YZControl/myself  
日期：2026-02-27
