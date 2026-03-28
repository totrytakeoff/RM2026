# infantry_gimbal_spin_et08_demo

在 `infantry_gimbal_et08_demo` 的云台逻辑基础上，增加底盘自旋扰动。

- `SD/SC` 上拨: `FOLLOW`
- `SD/SC` 下拨: `SEPARATE`
- `SA/SB` 上拨: 开启底盘自旋
- `SA/SB` 下拨: 关闭底盘自旋

用途:

- 更稳定地观察 `SEPARATE` 模式对底盘旋转扰动的跟随误差
- 为后续 `yaw` 前馈和小陀螺模式验证提供基础
