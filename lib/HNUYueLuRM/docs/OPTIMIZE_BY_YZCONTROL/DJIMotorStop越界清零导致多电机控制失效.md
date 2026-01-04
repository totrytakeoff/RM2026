# DJIMotorStop 越界清零导致多电机控制失效（云台无响应）

## 背景

在 `test_hero_shoot_test` 中将云台控制（GM6020 yaw/pitch）与发射机构控制（M3508 摩擦轮/拨弹）合并后，出现现象：

- 云台在合并工程里“完全没反应”（yaw/pitch 无输出）
- 单独烧录 `test_gimbal_demo` 时云台正常可控

## 复现条件

- 合并工程里存在对部分电机频繁 `DJIMotorStop()` 的逻辑（例如拨轮松开就停拨弹、摩擦轮未开就停等）
- 同时同一时刻还有其他电机在持续控制（例如云台 yaw/pitch 持续 `DJIMotorSetRef()`）

## 根因分析（关键）

问题根因在 `DJIMotorControl()` 中对 **停止电机** 的处理存在 **越界写**：

- DJI 电机发送帧每帧 8 字节（4 路 * 2 字节）
- `DJIMotorControl()` 会把每个电机最终输出写入 `sender_assignment[group].tx_buff[2*num]` 和 `[2*num+1]`
- 当电机 `stop_flag == MOTOR_STOP` 时，原实现使用：
  - `memset(sender_assignment[group].tx_buff + 2 * num, 0, 16u);`

由于 `tx_buff` 实际只有 8 字节，上述 `16u` 会从当前电机的 2 字节开始清零 **连续 16 字节内存**，必然越界，导致：

- 同一 CAN 发送组里其它电机的控制字节被清零（表现为其它电机“没反应/被停”）
- 甚至可能写坏相邻结构体/内存区域，造成更随机的异常

为什么 `test_gimbal_demo` 单独运行时“看起来没问题”：

- `test_gimbal_demo` 中基本不频繁调用 `DJIMotorStop()`（更多是设定值为 0），因此很少触发上述越界清零路径。
- 一旦在合并工程中频繁 Stop/Enable 切换，就会稳定触发该 bug，云台输出被反复清零，所以“完全不动”。

## 修复方案

修复原则：停止电机时只清零该电机对应的 **2 字节**，不要影响同组其它电机。

修改点：

- 文件：`lib/HNUYueLuRM/modules/motor/DJImotor/dji_motor.c`
- 函数：`DJIMotorControl()`
- 将清零长度从 `16u` 改为 `2u`

修复后逻辑：

- `memset(sender_assignment[group].tx_buff + 2 * num, 0, 2u);`

## 验证结果

- 修复后重新编译并烧录 `test_hero_shoot_test`，云台在合并工程中恢复正常响应。
- 说明问题确实由 Stop 路径的越界清零引起，而非遥控映射、CAN 初始化或电机 ID 本身。

## 经验与建议

1. **不要在发送缓冲区上做越界清零**：每电机只占 2 字节，清零必须精确。
2. 对于“希望电机静止”的场景，很多情况下使用 `DJIMotorSetRef(..., 0)` 比频繁 `DJIMotorStop()/Enable()` 更稳定（取决于控制策略与安全需求）。
3. 合并多个子系统（底盘/云台/发射）时，若出现“某一模块完全无输出但单独工程正常”，优先排查：
   - 多电机分组发送缓冲区是否互相覆盖
   - Stop/Enable 是否会影响同组其它电机
   - ID/反馈 ID 是否发生冲突（框架内已对 rx_id 冲突做了死循环保护，但发送侧的缓冲区越界不会触发该保护）

