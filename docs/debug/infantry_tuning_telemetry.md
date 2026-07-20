# 步兵正式固件 Yaw 调参遥测

更新日期：2026-07-20

## 1. 用途与开关

当前遥测专门服务于“先做 Yaw 机械零位标定，再调底盘跟随”：

- `MINIMAL_DEBUG_ENABLE=0`：关闭原来的周期文本/RTT镜像；
- `INFANTRY_TUNING_TELEMETRY_ENABLE=1`：启用独立 `tuning_uart` 任务；
- UART6，115200，8N1，20 ms 周期；
- VOFA+ `JustFloat`，DMA 非阻塞发送，UART 忙时丢弃本帧。

配置位于 `applications/infantry/config/infantry_config.h`。VT 后端占用 UART6，编译期
互斥检查会禁止 VT 与当前调参任务同时启用。

## 2. 当前 15 个通道

通道从 0 开始编号，顺序与
[字段 CSV](infantry_yaw_tuning_fields.csv)完全一致：

| 通道 | 名称 | 单位/说明 |
| ---: | --- | --- |
| 0 | `time_ms` | 系统时间，ms |
| 1 | `yaw_encoder_ecd` | GM6020 原始编码器值，0~8191 |
| 2 | `yaw_encoder_single_deg` | 编码器单圈角，deg |
| 3 | `yaw_target_total_deg` | Yaw 世界系累计角目标，deg |
| 4 | `yaw_imu_total_deg` | Yaw 世界系累计角反馈，deg |
| 5 | `yaw_imu_single_deg` | IMU 单圈 Yaw，deg |
| 6 | `yaw_imu_gyro_z_rad_s` | IMU Z 轴角速度，rad/s |
| 7 | `yaw_offset_raw_deg` | 扣除 `YAW_CHASSIS_ALIGN_ECD` 后的相对角，deg |
| 8 | `yaw_offset_logic_deg` | 再扣除逻辑零偏后的最终跟随误差，deg |
| 9 | `yaw_relative_speed_rad_s` | GM6020 相对底盘角速度，rad/s |
| 10 | `follow_p_rad_s` | 底盘跟随 P 项，rad/s |
| 11 | `follow_d_rad_s` | 底盘跟随 D 项，rad/s |
| 12 | `follow_wz_raw_rad_s` | P+I+D 未限幅输出，rad/s |
| 13 | `follow_wz_limited_rad_s` | 跟随外环限幅输出，rad/s |
| 14 | `chassis_wz_cmd_rad_s` | 底盘最终旋转命令，rad/s |

每帧为 15 个小端 `float`，帧尾固定为 `00 00 80 7F`。一帧共 64 字节。

## 3. Yaw 标定顺序

1. SA 下位，架空底盘，将云台机械中心线与底盘正前方精确对齐；
2. 记录 `yaw_encoder_ecd` 和 `yaw_encoder_single_deg`；
3. 将 `YAW_CHASSIS_ALIGN_ECD` 改为记录到的整数编码器值；
4. 首轮将 `YAW_OFFSET_LOGIC_ZERO_DEG` 设为 `0.0f`，重新编译烧录；
5. 对正时确认 `yaw_offset_raw_deg`、`yaw_offset_logic_deg` 都接近 0；
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

完成零位标定后，再观察 10~14 通道调整底盘跟随。正误差收敛时应满足：

```text
yaw_offset_logic_deg > 0
yaw_relative_speed_rad_s < 0
follow_d_rad_s < 0
```

## 4. 自定义入口

字段唯一组帧入口是
`applications/infantry/diagnostics/infantry_tuning_telemetry.c` 中的
`InfantryTuningTelemetry_FillChannels()`。修改顺序时必须同步更新通道数量、本文档和 CSV。
