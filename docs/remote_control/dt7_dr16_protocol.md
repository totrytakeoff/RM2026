# DT7 & DR16 通信协议说明

本说明文档整理自 DJI 《DT7&DR16 2.4 GHz 遥控接收系统用户手册》以及工程中 `lib/HNUYueLuRM/modules/remote/remote_control.*` 的实现，解释 DT7 遥控器与 DR16 接收机之间通过 **D-BUS**（亦称 SBUS）接口交换的 18 Byte 帧结构，方便进行固件开发与调试。

> 参阅原厂文档：`docs/ref_docs/DT7&DR16_RC_System_User_Manual_v2.00_cn.pdf`

## 1. 系统概览

- **工作频段**：2.4 GHz ISM，最大空旷通信距离约 1 km，DT7 续航约 12 h。
- **通道数**：摇杆 4 路 + 拨轮 1 路 + 左/右三挡开关各 1 路 + 鼠标 2 轴 2 键 + 键盘 16 键，共 16 个逻辑通道。
- **物理接口**：DR16 提供 D-BUS（串口信号，兼容 DJI 飞控的 DBus/D-Bus 接口）与 EXP（扩展 PWM）输出；机器人场景通常直接读取 D-BUS。
- **波特率**：100 kbps，8 位数据位，偶校验，1 位停止位（8E1），空闲帧周期约 14 ms（≈70 Hz）。

## 2. 串口帧结构

D-BUS 数据帧长度固定 18 Byte，不含起止字节，采用 **Little Endian**。遥控器所有模拟输入均以 11 bit 定点值表示（0~2047），中位值为 1024。`remote_control.c` 中通过 `RC_CH_VALUE_OFFSET=1024` 将数据移到 ±660 的对称区间并做异常抑制 (`RectifyRCjoystick`, 行 19-47)。

### 2.1 帧字段总览

| Byte | Bits                   | 字段             | 说明 |
| ---- | ---------------------- | ---------------- | ---- |
| 0-1  | b0..b10, b11..b15      | Channel 0        | 右手水平（J1，roll/行进） |
| 1-2  | b5..b15, b0..b5        | Channel 1        | 右手垂直（J2，pitch） |
| 2-4  | b7..b15, b0..b9        | Channel 2        | 左手水平（J3，yaw） |
| 4-5  | b1..b15                | Channel 3        | 左手垂直（J4，throttle） |
| 5    | b4..b5, b6..b7         | S1/S2 三位开关   | b[5:4] 右开关，b[7:6] 左开关，值 1=上、3=中、2=下 |
| 6-7  | 16 bit                 | Mouse X          | PC 鼠标 X，低字节在前 |
| 8-9  | 16 bit                 | Mouse Y          | PC 鼠标 Y |
| 10-11| —                      | 保留             | DJI 保留，恒 0 |
| 12   | 8 bit                  | Mouse L          | 鼠标左键，0/1 |
| 13   | 8 bit                  | Mouse R          | 鼠标右键，0/1 |
| 14-15| 16 bit 位域            | PC 键盘          | W/S/D/A/Shift/Ctrl/Q/E/.../B，共 16 个键 |
| 16-17| 11 bit                 | Channel 4 / LD   | 拨轮（云台/模式），同样减 1024 居中 |

> 工程中只解析到 Byte17；DBUS 第 18 字节偶尔作为帧结束校验，但 DR16 输出为固定 18 字节且 `remote_control.c` 将完整缓冲传给 `sbus_to_rc`。

### 2.2 11 bit 模拟通道打包方式

通道打包遵循 Futaba SBUS 的“顺序拼接”方式，每个通道 11 bit，低位在前。以下示意图以 **Channel0-3** 为例，`b0` 表示最低位：

```
Byte0       Byte1       Byte2       Byte3       Byte4       Byte5
76543210    76543210    76543210    76543210    76543210    76543210
[Ch0 0-7][Ch0 8-10|Ch1 0-4][Ch1 5-10|Ch2 0-1][Ch2 2-9][Ch2 10|Ch3 0-6][Ch3 7-10|SW]
```

解包后每个通道执行：

```
value = raw_11bit - 1024
if abs(value) > 660: value = 0   // 防止异常跳变
```

`remote_control.c` 第 35-44 行给出了全部的取位公式，对应 `rc_ctrl[TEMP].rc.rocker_*`、`dial`。

### 2.3 开关字段

- 右开关（S2）：位于 Byte5 的低 2 bit（`(byte5>>4)&0x03`）。常量定义参考 `remote_control.h:35-42`。
- 左开关（S1）：Byte5 的高 2 bit（`((byte5>>4)&0x0C)>>2`）。
- 数值约定：`1=上`，`3=中`，`2=下`。因此判断宏 `switch_is_up/down/mid` 直接比较。

### 2.4 鼠标字段

`mouse.x`, `mouse.y` 均为带符号 16 bit。`mouse.press_l`、`mouse.press_r` 仅使用 0/1。该字段便于在裁判系统或上位机中模拟鼠标控制。

### 2.5 键盘位域

`Byte14-15` 为 16 bit 键值，定义在 `remote_control.h:45-61`：

```
bit0=W, bit1=S, bit2=D, bit3=A, bit4=Shift, bit5=Ctrl,
bit6=Q, bit7=E, bit8=R, bit9=F, bit10=G, bit11=Z,
bit12=X, bit13=C, bit14=V, bit15=B
```

在固件中采用 `Key_t` 位域联合体便于单个 bit 访问，并派生出 “Ctrl 组合键” 与 “Shift 组合键” 的镜像缓冲，同时通过 `key_count[3][16]` 统计上升沿次数，用于实现“按下一次触发一次” (`remote_control.c:54-87`)。

## 3. 典型时序与在线检测

1. **DMA 接收**：串口收到完整 18 字节后触发 `RemoteControlRxCallback`，立即喂看门狗并调用 `sbus_to_rc`。
2. **看门狗超时**：若 100 ms 内没有新的帧，`Daemon` 触发 `RCLostCallback`，清空 `rc_ctrl` 并重启串口，保证上层识别“遥控掉线” (`remote_control.c:94-109,120-135`)。
3. **帧率**：DBUS 输出约 70 Hz，可根据 `reload_count=10`（10×10 ms）估算。

## 4. 调试与校准要点

- **校准**：使用 DJI 官方 RC SYSTEM 工具做摇杆/开关校准后再通过 DBUS 读取，确保 1024 偏置准确（见官方手册“遥控器校准”章节）。
- **死区与滤波**：若机器人控制需要更大死区，可在 `RectifyRCjoystick` 基础上增加自定义滤波；现实现仅裁剪 ±660 之外的异常值。
- **键鼠扩展**：若要增加组合键，可参考 `KEY_PRESS_WITH_CTRL/SHIFT` 的镜像机制，再扩展新索引。
- **连接**：DR16 的 D-BUS 输出为 3.3 V TTL；连接 MCU 时需确保串口配置 100 kbps 8E1，必要时通过示波器确认空闲电平为高。

## 5. 快速对照表

| 物理输入 | 通道/字段 | signed 值范围（解析后） | 常见用途 |
| -------- | --------- | ------------------------ | -------- |
| 右手水平 (J1) | Channel 0 | -660 ~ +660 | 底盘左右、Roll |
| 右手垂直 (J2) | Channel 1 | -660 ~ +660 | 前进/后退、Pitch |
| 左手水平 (J3) | Channel 2 | -660 ~ +660 | 旋转、Yaw |
| 左手垂直 (J4) | Channel 3 | -660 ~ +660 | 油门、升降 |
| 拨轮 (LD) | Channel 4 | -660 ~ +660 | 云台俯仰、档位 |
| S1 / S2 | switch_left/right | 1/2/3 | 模式切换 |
| 鼠标 X/Y | mouse.x/y | int16_t | 上位机瞄准 |
| 鼠标键 | mouse.press_l/r | 0/1 | 射击/自瞄 |
| 键盘 | key 位域 | 0/1 | 拓展功能 |

通过以上说明，即可快速理解 DT7 & DR16 在机器人框架中的数据流，并根据需要扩展遥控策略或进行协议调试。

## 6. 常见问题

**Q1：DT7 的 D-BUS 协议是否固定？能否自定义帧格式或键位？**  
A：协议由 DJI 固件写死，DR16 只会输出 18 Byte 的标准 D-BUS 帧。用户无法改动帧字段定义，也无法“增删键位”。唯一能做的是通过 DJI RC SYSTEM 软件调整通道映射、控制模式、校准等，而解析端（单片机）必须遵从既定协议读取。

**Q2：按键/鼠标数据是否需要 PC 上位机参与？**  
A：需要。DT7 自身不采集键鼠事件，只有在 USB 连接 PC 并运行 RoboMaster/DJI 客户端或电比赛裁判系统时，客户端才会把本地键鼠状态写入遥控器，随后遥控器将这些位直接带入 D-BUS 帧。若未连接上位机，Byte12~15 会保持 0，单片机端读不到键鼠输入。

**Q3：遥控器透传的键值能否修改？**  
A：不能。帧中 bit0~15 对应的键位（W、S、D、A、Shift、Ctrl、Q…B）由官方固件固定，接收端可以任意映射功能，但不能改变遥控器发送的位编号或增加额外键。
