# GM6020 上电触发 CAN 中断洪流问题（诊断与解决方案）

日期：2025-11-24

作者：自动生成（基于代码审查）

简介
----
本文档记录在本工程中遇到的一个常见问题：当在 CAN 启动后**立即启用接收中断（CAN_IT_RX_FIFO0_MSG_PENDING）** 时，GM6020 电机上电会产生大量回报帧，触发大量中断（“中断风暴”），从而导致 MCU 无法稳定驱动 GM6020；而在相同条件下，M3508 电机则没有此类问题。

目的
----
- 说明问题发生的原因与证据
- 提供短期可快速验证的修复（低风险）
- 提供长期稳健的方案（推荐）
- 给出具体的代码片段与验证步骤，便于在本仓库内快速应用和验证

问题描述（现象）
----
- 现象：在 `can_filter_init()` 中对 CAN 做 `HAL_CAN_Start(...)` 后，如果同时立即执行 `HAL_CAN_ActivateNotification(..., CAN_IT_RX_FIFO0_MSG_PENDING)`，系统会进入大量中断服务，CPU 被 ISR 压满，导致看起来“驱动 6020 失败／无响应”。
- 对照：注释掉 `HAL_CAN_ActivateNotification(...)` 后（仍保留 `HAL_CAN_Start`），发送命令（TX）可以正常到达并驱动 GM6020。
- 另外，M3508 在相同设置下一直工作正常（说明并非所有电机或所有设备都会在上电产生洪流）。

关键代码位置（证据）
----
- `test_board/src/can.c`：

```c
// Start CAN but暂时禁用接收中断，避免GM6020上电时的中断风暴
HAL_CAN_Start(&hcan1);
// HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // 暂时注释掉

HAL_CAN_Start(&hcan2);
// HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING); // 暂时注释掉
```

- `examples/CAN_receive.c`：`HAL_CAN_RxFifo0MsgPendingCallback` 实现中断回调并做报文解析；这会在中断上下文进行读取和解析操作。
- `test_board/src/stm32f4xx_it.c`：`CAN1_RX0_IRQHandler` / `CAN2_RX0_IRQHandler` 直接调用 `HAL_CAN_IRQHandler(&hcanX)`，进入 HAL 中断处理路径。

根因分析（简要）
----
- GM6020 在上电或复位时常常会广播或发送大量的状态/诊断帧（厂商固件行为），这会使在 CAN 启动并开启 RX 中断后的短窗口内产生大量中断。
- 中断处理函数（HAL 层 + 用户回调）即便只是读取并放到缓冲，仍然会消耗 CPU 时间，频率高时会阻塞主循环与发送流程（TX）。
- M3508 在上电时通常不会产生类似级别的广播，因此不会引发中断风暴，所以表现正常。
- 额外因素：若 `AutoRetransmission` 被设为 DISABLE，在 TX 失败或总线繁忙时不能自动重试，也会让驱动看起来“失效”。

短期可行且低风险的修复（优先尝试）
----
1) 延迟启用接收中断（推荐快速验证）

说明：在系统初始化并完成重要模块就绪后，等待一段时间（例如 200~1000 ms），再调用 `HAL_CAN_ActivateNotification`。这样能避开上电瞬时的广播洪流。

示例（可放在 `main.c` 的初始化序列结束后）：

```c
HAL_CAN_Start(&hcan1);
HAL_CAN_Start(&hcan2);
HAL_Delay(500); // 等待电机上电并停止初始广播
HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
```

2) 将 `AutoRetransmission` 设为 `ENABLE`

说明：当总线短暂繁忙时自动重传能提高 TX 成功率。

示例（`MX_CAN1_Init` / `MX_CAN2_Init`）：

```c
hcan1.Init.AutoRetransmission = ENABLE;
hcan2.Init.AutoRetransmission = ENABLE;
```

3) 降低 CAN RX 中断優先级

说明：把 CAN RX 中断優先级设为更低，以避免其抢占关键任务或导致 TX 路径/主循环被长时间阻塞。例如将 `6` 改为 `12`（数值越大优先级越低）。

示例（`HAL_CAN_MspInit`）：

```c
HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 12, 0);
HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 12, 0);
HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
```

长期稳健的推荐方案（生产级）
----
1) 使用硬件过滤器（强烈推荐）

说明：在 CAN 控制器的硬件层只接受你真正关心的 StdId（或一个 ID 列表），其他报文在硬件层就被丢弃，不会进入 FIFO，自然不会触发中断或占用 CPU 时间。对高流量和实时系统这是最根本的解决方案。

实现方式：
- 用 `CAN_FilterTypeDef` 配置为 `IDMASK` 或 `IDLIST` 模式，精确指定需要接收的 ID 或 ID 范围。
- 如果你告诉我需要接受的 ID 列表（例如只接收 `0x200~0x20F` 或只接收 `0x205~0x208`），我可以给出精确的 HAL 过滤器配置 patch。

2) 精简 ISR 内处理量

说明：`HAL_CAN_RxFifo0MsgPendingCallback` 内仅做最短处理（读取报文并把原始数据拷贝到环形缓冲或消息队列），不要在 ISR 中做复杂解析与业务逻辑，后续由任务/主循环消费。

3) 启动阶段分阶段启用策略

说明：先启动 CAN（Start），但只启用最小必要 filter / 不启用中断；待设备稳定后逐步启用更多 filter 与中断。

逐步实现与验证建议
----
步骤 A（快速，优先级最高） —— 立即可试

1. 在 `MX_CANx_Init` 中把 `AutoRetransmission` 设为 `ENABLE`。
2. 在 `can_filter_init()` 保留 `HAL_CAN_Start(&hcanX)`，不要在这里激活通知。
3. 在 `main.c` 初始化完成后调用延迟激活中断（例如 `HAL_Delay(500)` 后 `HAL_CAN_ActivateNotification`）。
4. 编译并烧录，重复上电测试若干次，观察是否稳定。

编译/上传示例（在 `test_board` 目录，PlatformIO）：

```bash
cd /home/myself/workspace/RM/code/test_board
pio run -e <your_env>
pio run -t upload -e <your_env>
```

步骤 B（针对生产/长期）

1. 确定需要接收的 CAN 标准 ID 列表或范围。
2. 在 `can_filter_init()` 中配置硬件过滤器（IDLIST 或 IDMASK）。
3. 优化 `HAL_CAN_RxFifo0MsgPendingCallback`：改为将消息入队，由任务解析。
4. 在多次真实上电场景下验证稳定性并用 CAN 分析工具或示波器确认总线行为。

补充说明：为何 M3508 不出现问题？
----
M3508 在上电不产生相同规模的广播回报帧（或回报帧频率较低），因此即便启用中断，也不会触发“中断风暴”。不同电机固件行为差异是常见情况。

如果需要我现在就做的事情（可选）
----
1. 我可以在仓库里应用“快速 patch”：
   - 把 `AutoRetransmission` 设为 `ENABLE`；
   - 在 `can_filter_init()` 保留 `HAL_CAN_Start`，移除或注释掉 `HAL_CAN_ActivateNotification`；
   - 在 `main.c` 中增加延迟并激活 RX 中断的示例；
   - 把 CAN RX 中断优先级从 `6` 调低到 `12`。
2. 或者我可以直接实现硬件过滤器（需要你确认要接收的 ID 列表/范围）。

请回复：
- “快速 patch” —— 我会直接在仓库里提交并说明改动；或
- “实现 hardware filter” 并给出要接收的 ID 列表/范围（例如 `0x200~0x20F`），我会提交具体 filter 配置 patch。

附录：参考的关键代码文件
----
- `test_board/src/can.c`（CAN 初始化、Start、注释的 ActivateNotification）
- `examples/CAN_receive.c`（`HAL_CAN_RxFifo0MsgPendingCallback` 的具体实现）
- `test_board/src/stm32f4xx_it.c`（`CAN1_RX0_IRQHandler` / `CAN2_RX0_IRQHandler`）

-- 结束 --
