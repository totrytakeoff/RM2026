# infantry_chassis_et08_demo

目标：把 infantry 的底盘控制链路独立出来，单独验证 ET08 输入与十字全向轮底盘解算。

## 功能
- ET08(SBUS, USART3) 读取摇杆。
- 仅控制 4 个底盘 M3508（CAN1: ID 1/2/3/4）。
- 十字全向轮逆解：`vx/vy/wz -> FR/FL/BR/BL`。
- 遥控离线 / failsafe / frame_lost 时立即停机。
- USART6 输出关键调试信息（输入与轮速参考）。

## 摇杆映射
- `left.x` -> `vx`（左正右负）
- `left.y` -> `vy`（前正后负）
- `right.x` -> `wz`（逆时针正）

## 构建
```bash
cmake -S . -B build
cmake --build build --target test_infantry_chassis_et08_demo -j8
```

## 烧录
```bash
cmake --build build --target flash-test_infantry_chassis_et08_demo
```
