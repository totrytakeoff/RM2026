# CAN 启动顺序与自动恢复说明

## 现象
- 电机断电 → 烧录固件 → 电机重新上电
- 此时电机不转，需要“重启开发板”后才恢复正常

## 根因
- MCU 启动后立即开始发送 0x200 组播电流帧，但此时电机电调未上电，CAN 总线上没有 ACK。
- 控制器发送失败并累计错误计数，可能进入 Error Passive/Bus-Off 状态。
- 若 CAN 未启用自动恢复/唤醒/重发，控制器不会在电机后上电时自行恢复，导致一直“无响应”。
- 只有重启 MCU（重新初始化并 Start CAN）才会恢复。

## 修复
在 `src/hal/can.c` 中对 CAN1/2 的初始化进行了如下修改：

- AutoBusOff = ENABLE
  - 允许进入 Bus-Off 后由硬件自动恢复
- AutoWakeUp = ENABLE
  - 总线恢复活动后自动唤醒控制器
- AutoRetransmission = ENABLE
  - 因无 ACK 等情况自动重发，提高可靠性
- 启用接收中断与错误/Bus-Off 通知
  - `HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_BUSOFF | CAN_IT_ERROR);`
  - 结合应用层回调 `HAL_CAN_RxFifo0MsgPendingCallback` 中对 `g_can.pollOnce()` 的调用，保证反馈及时更新

以上设置保证：
- 即便 MCU 先启动、后电机上电，CAN 控制器也能在总线恢复后自动进入工作状态，无需重启板子。

## 建议的上电顺序（工程实践）
- 优先方案：先为电机上电，再启动/复位控制板。
- 或者：保持当前硬件顺序，但启用上述自动恢复配置。

## FAQ

- Q: 为什么要启用 AutoRetransmission？
  - A: 在没有 ACK 的情况下，帧会失败；自动重发能在电机刚上电时快速建立通信，避免控制器短时间内频繁进入错误状态。

- Q: 是否需要应用层定期检测并手动恢复？
  - A: 不是必须。你也可以增加容错，例如周期检查错误寄存器，检测到 Bus-Off 时执行 `HAL_CAN_Stop` + `HAL_CAN_Start` 并重新 `HAL_CAN_ActivateNotification`。

- Q: 这会不会导致中断风暴？
  - A: RX 中断只在有报文来时触发；错误/Bus-Off 通知只在异常发生时触发。正常工作下中断频率可控。若确实需要，可仅开启 `CAN_IT_RX_FIFO0_MSG_PENDING`。

## 与控制抖动的关系
- 启用 RX 中断后，反馈 `speed_rpm` 会及时更新，配合合理的 PID 参数（Kp 建议 6~10，Ki 0.2~0.5），可显著降低 `test_speed_group` 的抖动。

