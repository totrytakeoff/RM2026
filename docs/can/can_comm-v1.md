# 通用 CAN 封装类 CanBus 使用说明

本文档介绍 `src/can_comm.hpp` 中 `CanBus` 的设计目标、API 说明与使用示例，适用于资源受限的嵌入式项目（STM32 HAL）。

---

## 设计目标
- 仅使用基础 C++ 特性（无 STL/无动态内存/无异常），可在 MCU 环境稳定运行。
- 基于 HAL 的 `CAN_HandleTypeDef`，提供简洁的发送与接收轮询接口。
- 通过回调在主循环中处理接收数据，避免在中断中做复杂耗时逻辑。

---

## 头文件位置
- `src/can_comm.hpp`

引入方式（C/C++ 混编）：
```cpp
extern "C" {
#include "stm32f4xx_hal.h"
}
#include "can_comm.hpp"
```

---

## 初始化前置条件
- 底层 CAN 外设必须已经完成初始化与启动：
  - `MX_CANx_Init();`
  - `can_filter_init(); // 内部会 HAL_CAN_Start + 开启接收通知`
- 选择你要绑定的 CAN 句柄：`&hcan1` 或 `&hcan2`。

---

## 类接口概览
```cpp
class CanBus {
public:
    typedef void (*RxCallback)(const CAN_RxHeaderTypeDef* header, const uint8_t* data, void* user);

    explicit CanBus(CAN_HandleTypeDef* handle);
    void attachRxCallback(RxCallback cb, void* user);

    HAL_StatusTypeDef sendStd(uint16_t std_id, const uint8_t* data, uint8_t len);
    HAL_StatusTypeDef sendExt(uint32_t ext_id, const uint8_t* data, uint8_t len);

    bool pollOnce();
    CAN_HandleTypeDef* handle();
};
```

### 构造
- `CanBus(CAN_HandleTypeDef* handle)`：
  - 传入 HAL 句柄指针，例如 `&hcan1` 或 `&hcan2`。

### 绑定接收回调（可选）
- `attachRxCallback(RxCallback cb, void* user)`：
  - 设置在 `pollOnce()` 成功取到一帧数据时回调。
  - `user` 是用户自定义上下文指针，可传入对象指针或结构体指针。

### 发送接口
- `sendStd(uint16_t id, const uint8_t* data, uint8_t len)`：发送标准 11 位 ID 帧（例如 0x1FF）。
- `sendExt(uint32_t id, const uint8_t* data, uint8_t len)`：发送扩展 29 位 ID 帧。
- 返回值为 `HAL_StatusTypeDef`，`HAL_OK` 表示成功入邮箱（最终发送由硬件完成）。

### 接收轮询
- `bool pollOnce()`：从 FIFO0 拉取一帧数据（若有），成功则触发回调。
- 推荐在主循环中以固定节拍调用，或在空闲时尽量多次调用以降低延迟。

---

## 基本使用示例（驱动 GM6020 电流控制）
以下示例展示了如何通过 CanBus 以 `0x1FF` 组播发送 4 路电流指令来驱动 GM6020（ID=1..4 对应 0x205~0x208）。

```cpp
extern "C" {
#include "can.h"          // 提供 hcan1 / hcan2
}
#include "can_comm.hpp"

// 选择要使用的 CAN 口（根据你的工程：GIMBAL 通常用 CAN2/也可以切到 CAN1）
static CanBus g_can(&hcan1); // 或者 &hcan2

// 将 4 路 int16_t 电流值打包为 8 字节
static inline void pack_currents(int16_t i1, int16_t i2, int16_t i3, int16_t i4, uint8_t out[8])
{
    out[0] = (uint8_t)(i1 >> 8); out[1] = (uint8_t)(i1);
    out[2] = (uint8_t)(i2 >> 8); out[3] = (uint8_t)(i2);
    out[4] = (uint8_t)(i3 >> 8); out[5] = (uint8_t)(i3);
    out[6] = (uint8_t)(i4 >> 8); out[7] = (uint8_t)(i4);
}

void gm6020_drive_demo_once()
{
    uint8_t payload[8];
    pack_currents(10000, 0, 0, 10000, payload);
    // 0x1FF: 云台/发射总线的组播控制帧（1..4号电机 -> 0x205..0x208）
    g_can.sendStd(0x1FF, payload, 8);
}

// 在你的 main 循环中周期调用：
//   gm6020_drive_demo_once();
//   HAL_Delay(5);
```

---

## 接收反馈（可选）
GM6020 的反馈帧 ID 为 `0x204 + 电机ID`，例如 ID=1 -> 0x205。可以绑定回调并在主循环轮询：

```cpp
static void on_can_rx(const CAN_RxHeaderTypeDef* h, const uint8_t* d, void* user)
{
    // 这里可根据 h->StdId 判断是否为 0x205..0x208，然后解析角度/速度/电流/温度
}

void can_user_setup()
{
    g_can.attachRxCallback(&on_can_rx, 0);
}

void loop_poll()
{
    // 在主循环里反复调用，尽可能降低接收延迟
    while (g_can.pollOnce()) { /* 拉取所有待处理帧 */ }
}
```

---

## 常见问题排查
- 发送不出：确认 `HAL_CAN_Start()` 是否已执行（我们在 `can_filter_init()` 中已统一启动）。
- 接收不到：检查过滤器配置、是否启用 FIFO0 通知、中断优先级、总线终端电阻与线序。
- 没反应但无错误：确认你发送到的 CAN 口与硬件实际接线一致（hcan1 ↔ 2-pin、hcan2 ↔ 4-pin），以及电机 ID 对应 `0x205..0x208`。

---

## 版本与维护
- 本类为轻量封装，API 稳定。后续可在不引入 STL 的前提下增加更丰富的工具函数（例如批量发送、固定打包器等）。
