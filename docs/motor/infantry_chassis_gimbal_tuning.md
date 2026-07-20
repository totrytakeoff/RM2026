# 步兵底盘与云台调参说明

更新日期：2026-07-20

## 1. 分层约定

遥控设备层和步兵输入 Profile 只发布 `[-1,1]` 无量纲意图：

- `chassis_x_intent`、`chassis_y_intent`、`chassis_rotate_intent`；
- `gimbal_yaw_intent`、`gimbal_pitch_intent`。

遥控配置中只允许存在中位、量程、死区、串口和在线截止等设备参数。m/s、rad/s、
机械角度、电机减速比和输出饱和只能存在于底盘或云台执行层。

## 2. 底盘关键参数

参数位于 `applications/infantry/config/infantry_config.h`。

| 参数 | 当前值/含义 | 调整建议 |
| --- | --- | --- |
| `M3508_ROTOR_SPEED_LIMIT_RAD_S` | `523.599 rad/s`（5000 rpm），M3508转子速度参考上限 | 满量程总上限；确认电机温升后再改 |
| `CHASSIS_MOTOR_REDUCTION_RATIO` | `19.20320856` | 标准 M3508 减速比；非标准传动必须实测修改 |
| `CHASSIS_WHEEL_RADIUS` | `0.075 m` | 使用有效滚动半径，不只量外径 |
| `CHASSIS_MAX_TRANSLATION_SPEED` | 由上述三项推导，约 `2.045 m/s` | 不单独手填，避免物理量互相矛盾 |
| `CHASSIS_MAX_ROTATION_SPEED_RAD_S` | 由平移能力和半轮距推导，约 `12.03 rad/s` | 纯旋转满意图上限 |
| `CHASSIS_SPEED_FILTER_COEF` | `0.15` | `0` 最直接；增大更平滑也更迟钝 |
| `CHASSIS_FOLLOW_WZ_KP/KI/KD` | 当前 `5/0/0.10`；误差和输出使用 rad、rad/s | 先调 KP，再用正 KD 增加阻尼；KI 默认保持 0 |
| `CHASSIS_FOLLOW_WZ_MAX` | 当前 `2.5 rad/s`，仅限制跟随外环 | 跟随稳定后按 `2.5→3.5→4.5→6.0` 增加，不影响底盘物理总能力 |
| `CHASSIS_SPIN_SPEED_RAD_S` | 当前等于纯旋转能力上限 | 小陀螺太猛时只调这一项，不改遥控映射 |
| `CHASSIS_SPEED_KP/KI/KD` | 四个 M3508 速度环 | 先验证参考和反馈单位/方向，再调 PID |

平移和旋转叠加超过电机能力时，四轮参考统一等比例缩放到
`M3508_ROTOR_SPEED_LIMIT_RAD_S`，从而保持合成运动方向。底盘 M3508 的速度参考、
速度反馈、PID 误差和调试量统一使用转子 `rad/s`，不再换算成 `deg/s`。

## 3. 云台关键参数

| 参数 | 当前值/含义 | 调整建议 |
| --- | --- | --- |
| `GIMBAL_YAW_MAX_SPEED_DEG_S` | `828 deg/s` | Yaw 满摇杆速度，只在执行层生效 |
| `GIMBAL_PITCH_MAX_SPEED_DEG_S` | `540 deg/s` | Pitch 满摇杆速度，只在执行层生效 |
| `GIMBAL_PITCH_MIN/MAX_DEG` | `-25/35 deg` | 实测机械余量后再改 |
| `GIMBAL_PITCH_SOFT_MARGIN_DEG` | `3 deg` | 越大越早减速，越小越接近硬边界才减速 |
| `YAW_ANGLE_*`、`YAW_SPEED_*` | Yaw 串级 PID | 当前所有模式使用同一套有效参数 |
| `PITCH_ANGLE_*`、`PITCH_SPEED_*` | Pitch 保持/手动串级 PID | 不要用积分掩盖重力前馈错误 |

## 4. Pitch 重力补偿

当前执行层使用重力参考的 IMU Pitch：

```text
ff = clamp(K × cos(imu_pitch - horizontal_angle), ±FF_MAX)
```

关键参数：

- `PITCH_GRAVITY_FF_K=+2500`：补偿方向和幅值；方向已确认时只调绝对值；
- `PITCH_GRAVITY_FF_MAX=4000`：前馈绝对上限；
- `PITCH_GRAVITY_HORIZONTAL_DEG=0`：炮管机械水平时的 IMU Pitch 实测值；
- `PITCH_FF_LPF=0.8`：越大越平滑、滞后越明显。

建议顺序：

1. 架空并允许快速断电，先令 `K=0`，确认 Pitch 上下方向、速度环和机械限位正确；
2. 将炮管机械调平，记录日志中的 `pitch_imu`，填入 `PITCH_GRAVITY_HORIZONTAL_DEG`；
3. 从 `K=1000` 开始，每次增加 `500`，直到水平附近松杆不明显下坠；
4. 若云台自行抬头或向下运动明显吃力，立即降低 `K`；
5. 最后再调 `PITCH_ANGLE_*` 和 `PITCH_SPEED_*`，不要先堆积分。

通用文本日志当前默认关闭。连续调参使用独立 UART6/VOFA+ 任务，通道定义见
[步兵正式固件 UART 调参遥测](../debug/infantry_tuning_telemetry.md)。需要 Pitch 专项变量时，
在该任务唯一的 `InfantryTuningTelemetry_FillChannels()` 入口追加通道。

## 5. 底盘跟随控制律

当前控制律已经统一为直观正增益形式：

```text
wz = Kp × error + Ki × integral(error) + Kd × error_rate
```

当正误差正在收敛时，`error_rate<0`，所以正 `Kd` 会产生负的 D 项并降低追赶速度。
旧代码的外层整体负号已移除，禁止再使用负 `Kp/Kd` 维持双重符号。

跟随误差会在底盘执行层再次归一化到 `[-pi, pi]`，所以恢复到
`YAW_CHASSIS_ALIGN_ECD` 标定正姿态时总是选择最近方向。以下两个边沿会立即进入恢复：

- 上电、遥控重连或 SA 安全门重新放行后的首个 FOLLOW 控制周期；
- SB 从小陀螺上位切换到中位或下位后的首个控制周期。

恢复首拍的 `wz` 不经过 `CHASSIS_SPEED_FILTER_COEF` 削弱，后续周期仍按正常低通和
跟随 PID 工作。这里的“上电立即恢复”服从 SA 安全门：SA 未上拨、遥控离线或系统停机时
电机保持掉电，只记录待恢复状态；允许输出后才开始运动。云台继续保持当前世界系 Yaw
目标，恢复动作由底盘旋转完成，不会让云台自行回头破坏瞄准方向。
