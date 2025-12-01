# M3508/C620 电机控制与 CAN 通信总结

本文档总结了在使用 M3508 电机和 C620 电调进行 CAN 通信和速度控制时遇到的两个常见问题及其解决方案。

## 1. CAN 接收中断风暴问题 (ISR Storm)

### 现象
在 `main` 函数中启用 CAN 接收中断通知后，程序卡死，电机停止转动：
```cpp
// HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // 启用后卡死
```

### 原因分析
STM32 HAL 库的 CAN 接收中断流程如下：
1.  CAN 接收到消息，FIFO0 挂起。
2.  触发 `CAN1_RX0_IRQHandler`，该函数调用 `HAL_CAN_IRQHandler`。
3.  `HAL_CAN_IRQHandler` 随后调用用户自定义的回调函数 `HAL_CAN_RxFifo0MsgPendingCallback`。
4.  如果用户没有实现此回调函数，或者在回调函数中没有调用 `HAL_CAN_GetRxMessage` 来读取消息，则消息会一直保留在硬件 FIFO 中。
5.  只要 FIFO 非空，中断标志位就保持设置状态。CPU 退出 ISR 后会立即再次进入 ISR，形成无限循环，即**中断风暴 (ISR Storm)**，导致主程序卡死。

### 解决方案
必须在代码中实现 `HAL_CAN_RxFifo0MsgPendingCallback`，并在其中调用读取消息的函数，以清除 FIFO 和中断标志。

在 `src/test/motor/M3508demo.cpp` 中的实现示例：
```cpp
/**
 * @brief HAL库 CAN接收FIFO0消息挂起回调（中断处理）
 *        必须实现此函数以处理中断，否则会发生中断风暴
 */
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    // 假设 g_can 绑定了 hcan1
    if (hcan == g_can.handle()) {
        // pollOnce() 内部会调用 HAL_CAN_GetRxMessage，读取消息并清除中断标志
        g_can.pollOnce();
    }
}
```

## 2. M3508 电机控制模式与闭环需求

### 现象
向 C620 电调发送固定的电流指令（例如 `400`），电机在空载情况下会一直加速，无法稳定在某一速度。

### 核心概念：控制环的分层
C620 电调和 M3508 电机系统采用分层控制：

| 控制环 | 所在位置 | 控制目标 | 输入指令 | 作用 |
| :--- | :--- | :--- | :--- | :--- |
| **电流环 (力矩环)** | C620 电调内部 | 电机电流 (扭矩) | CAN 电流指令 | 保证电机输出的力道与指令一致（高频响应，抗干扰） |
| **速度环 (或位置环)** | STM32 MCU 外部 | 电机转速 (RPM) | 目标速度 | 根据速度误差计算所需的电流指令，实现稳速或定位 |

### 原因分析
1.  **C620 仅接受电流指令**：C620 电调通过 CAN 接收的指令是电流值（范围通常为 `[-16384, 16384]`），它只负责执行电流环。
2.  **恒定电流 = 恒定扭矩**：在物理上，恒定的扭矩（力）会导致恒定的角加速度。
3.  **空载加速**：在空载或低负载情况下，恒定扭矩会使电机持续加速，直到达到物理极限。

### 解决方案：实现速度闭环
为了让电机稳定在目标速度，必须在 MCU 上实现速度闭环控制（通常使用 PID 算法）。

**控制逻辑**：
$$
\text{Current}_{\text{cmd}} = \text{PID}(\text{Target}_{\text{speed}} - \text{Actual}_{\text{speed}})
$$

在 `src/test/motor/M3508demo.cpp` 中，我们添加了简单的 P 控制示例：
```cpp
// 简单的P控制器参数
float Kp = 5.0f;
int16_t target_rpm = 500; // 目标转速

// 主循环中的 P 控制逻辑
int16_t error1 = target_rpm - motor1.measure().speed_rpm;
int16_t current1 = (int16_t)(error1 * Kp);

// 限制电流范围 (M3508/C620 最大 +-16384)
if (current1 > 10000) current1 = 10000;
if (current1 < -10000) current1 = -10000;

M3508::sendCurrentGroup(&g_can, current1, 0, 0, 0);
```

### 负载变化的影响
即使电机带有减速箱或负载，速度闭环也是必要的。如果负载发生变化（例如机器人上坡或下坡），开环控制（固定电流）会导致速度波动。PID 闭环控制能够自动检测到速度变化，并动态调整电流指令，从而实现对速度的精确和稳定控制（抗干扰）。
