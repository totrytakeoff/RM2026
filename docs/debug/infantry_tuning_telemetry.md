# 步兵正式固件云台调参遥测

更新日期：2026-07-21

## 1. 用途与开关

当前遥测服务于云台双轴三态控制、IMU 串级 PID 限幅和底盘耦合分析：

- `MINIMAL_DEBUG_ENABLE=0`：关闭原来的周期文本/RTT镜像；
- `INFANTRY_TUNING_TELEMETRY_ENABLE=1`：启用独立 `tuning_uart` 任务；
- `INFANTRY_TUNING_GIMBAL_AXIS=INFANTRY_TUNING_GIMBAL_AXIS_YAW`：选择当前
  VOFA+ 主调试轴；改为 `..._PITCH` 即切换到 Pitch；
- UART6，115200，8N1，20 ms 周期；
- VOFA+ `JustFloat`，DMA 非阻塞发送，UART 忙时丢弃本帧。

配置位于 `applications/infantry/config/infantry_config.h`。VT 后端占用 UART6，编译期
互斥检查会禁止 VT 与当前调参任务同时启用。

## 2. 当前 31 个通道

通道从 0 开始编号，顺序与
[字段 CSV](infantry_gimbal_tuning_fields.csv)完全一致。字段中的 `axis` 表示由
`INFANTRY_TUNING_GIMBAL_AXIS` 选中的主调试轴：

| 通道 | 名称 | 单位/说明 |
| ---: | --- | --- |
| 0 | `time_ms` | 系统时间，ms |
| 1 | `axis_ctrl_mode` | `0=ANGLE, 1=SPEED, 2=BRAKE` |
| 2 | `axis_operator_speed_cmd_deg_s` | 操作者速度指令，deg/s |
| 3 | `axis_hold_target_deg` | 角度保持目标，deg |
| 4 | `axis_imu_angle_deg` | 所选轴 IMU 角反馈，deg |
| 5 | `axis_angle_error_deg` | 电机任务实际角度 PID 误差，deg |
| 6 | `axis_imu_gyro_deg_s` | 角速度环实际使用的 IMU 反馈，deg/s |
| 7 | `axis_angle_p_deg_s` | 角度 PID 的 P 项，deg/s |
| 8 | `axis_angle_i_deg_s` | 角度 PID 的 I 项，deg/s |
| 9 | `axis_angle_d_deg_s` | 角度 PID 的 D 项，deg/s |
| 10 | `axis_angle_output_deg_s` | 角度 PID 最终输出，即速度环参考，deg/s |
| 11 | `axis_angle_output_limit_ratio` | 角度环输出限幅比例，到 ±1 即限幅 |
| 12 | `axis_speed_ref_deg_s` | 电机任务实际速度 PID 参考，deg/s |
| 13 | `axis_speed_fdb_deg_s` | 电机任务实际速度 PID 反馈，deg/s |
| 14 | `axis_speed_error_deg_s` | 速度 PID 误差，deg/s |
| 15 | `axis_speed_p_current` | 速度 PID 的 P 项，GM6020 电流命令单位 |
| 16 | `axis_speed_i_current` | 速度 PID 的 I 项，GM6020 电流命令单位 |
| 17 | `axis_speed_d_current` | 速度 PID 的 D 项，GM6020 电流命令单位 |
| 18 | `axis_speed_output_current` | 速度 PID 输出；尚未叠加前馈和最终电机方向变换 |
| 19 | `axis_speed_output_limit_ratio` | 速度环输出限幅比例，到 ±1 即限幅 |
| 20 | `axis_motor_current_fdb` | GM6020 电调原始反馈电流；Pitch 电机反装时符号不同于 IMU 控制轴 |
| 21 | `axis_encoder_ecd` | GM6020 原始编码器值，0~8191 |
| 22 | `axis_current_feedforward` | 控制轴坐标中的电流前馈；Yaw 小陀螺补偿或 Pitch 重力补偿 |
| 23 | `yaw_offset_logic_deg` | Yaw 相对底盘标定零位误差，deg |
| 24 | `chassis_wz_cmd_rad_s` | 底盘最终旋转命令，rad/s |
| 25 | `yaw_base_rate_estimate_rad_s` | `IMU角速度-Yaw相对角速度` 得到的底盘基座角速度估计 |
| 26 | `chassis_input_x_intent` | 云台坐标系横移输入，左正，归一化值 |
| 27 | `chassis_input_y_intent` | 云台坐标系前进输入，前正，归一化值 |
| 28 | `chassis_body_vx_cmd_m_s` | 坐标变换后的车体横移命令，左正，m/s |
| 29 | `chassis_body_vy_cmd_m_s` | 坐标变换后的车体前进命令，前正，m/s |
| 30 | `chassis_spin_translation_scale` | 小陀螺为旋转保留轮速后实际平移比例，0~1 |

每帧为 31 个小端 `float`，帧尾固定为 `00 00 80 7F`。一帧共 128 字节，
50 Hz 时串口线速约 64 kbit/s，低于 115200 bit/s。

正式云台接口使用统一的 `GimbalTuningSnapshot`，其中 `yaw`、`pitch` 都是相同的
`GimbalAxisTuningSnapshot`。UART 通过一个配置宏选择 `yaw` 或 `pitch`，编码器、IMU、
角度 PID、速度 PID、电流反馈和电流前馈不需要再增加底层接口或修改 CSV。

## 3. Yaw 标定顺序

1. SA 下位，架空底盘，将云台机械中心线与底盘正前方精确对齐；
2. 保持主调试轴为 Yaw，记录 `axis_encoder_ecd`；
3. 将 `YAW_CHASSIS_ALIGN_ECD` 改为记录到的整数编码器值；
4. 首轮将 `YAW_OFFSET_LOGIC_ZERO_DEG` 设为 `0.0f`，重新编译烧录；
5. 对正时确认 `axis_encoder_ecd` 接近标定值、`yaw_offset_logic_deg` 接近 0；
6. 若只有小于一个编码器刻度的安装残差，再使用 `YAW_OFFSET_LOGIC_ZERO_DEG` 微调。

`/home/myself/Downloads/vofa+.csv` 中原先选择的 `ECD=25` 姿态经实机复核存在机械
偏差，不再作为正式零位。2026-07-20 将云台重新摆到确认后的机械正前方，并从 UART6
连续采集 6 秒、共 300 帧：`ECD=7821` 占 283 帧，`ECD=7822` 占 17 帧，平均
`7821.0567`；编码器单圈角平均 `343.69634°`。正式配置改为：

```c
#define YAW_CHASSIS_ALIGN_ECD     7821U
#define YAW_OFFSET_LOGIC_ZERO_DEG 0.0f
```

按新配置重新烧录后，当前机械对正位置的 raw/logic offset 理论均值约
`0.00249°`；允许因编码器一格跳变出现约 `0.043945°` 的量化波动。

完成零位标定后，先使用 1~22 通道调稳 Yaw，再看 23~30 通道分析底盘耦合和
小陀螺平移解算。
Yaw 三态控制的预期顺序是：

```text
摇杆活动: axis_ctrl_mode=1，axis_speed_ref 跟随操作者速度指令
摇杆释放: axis_ctrl_mode=2，axis_speed_ref=0 主动制动
速度稳定: axis_ctrl_mode=0，axis_hold_target 锁定当前 IMU 角度
```

Pitch 始终保持 `ANGLE(0)`，不再进入 SPEED/BRAKE，也不会因摇杆活动或松开而重置 PID。
`axis_operator_speed_cmd_deg_s` 是执行层按摇杆计算的目标推进速率；控制任务按实际经过时间
将其累计到 `axis_hold_target_deg`，松杆后目标立即冻结。

若通道 11 到达 ±1，是角度外环输出受限；若通道 19 到达 ±1，是速度环电流输出
受限。小陀螺下还应同时观察通道 22 的基座电流前馈、通道 25 的基座角速度估计和
通道 30 的平移保留比例。

## 4. Pitch 坐标与调参判据

Pitch 主调试轴下，通道 3~19 已统一为 IMU Pitch 坐标：

```text
正角度/正速度参考 -> 正 IMU Pitch/正 GyroX
ANGLE 模式          -> hold_target 与 imu_angle 直接比较
axis_speed_ref       -> 角度外环产生的内环速度参考
```

电机安装反向只在最终执行输出处理，所以通道 20 仍是电调原始电流；它可与控制轴电流
通道符号相反。本车当前 Pitch 的近似关系为
`motor_command = -(axis_speed_output_current + axis_current_feedforward)`。
2026-07-20 修复前 `hold_target` 使用电机编码器角，曾与 IMU Pitch 满足
约 `motor+imu=64°` 的镜像关系，该行为已废止，不得再以编码器角作为 Pitch 保持目标。

当前 `PITCH_GRAVITY_FF_K=0` 是去掉重力补偿后的临时调试值。稳定保持时重点确认通道 16
不长期卡在 ±`PITCH_SPEED_I_MAX`；否则应恢复并标定重力前馈，而不是持续提高角度 P。

## 5. 自定义入口

字段唯一组帧入口是
`applications/infantry/diagnostics/infantry_tuning_telemetry.c` 中的
`InfantryTuningTelemetry_FillChannels()`。修改顺序时必须同步更新通道数量、本文档和 CSV。
