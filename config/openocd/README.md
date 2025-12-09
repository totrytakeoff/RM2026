# OpenOCD 调试配置

此目录包含用于OpenOCD的调试器配置文件。

## 文件说明

- `openocd_dap.cfg` - 使用CMSIS-DAP调试器的配置
- `openocd_jlink.cfg` - 使用J-Link调试器的配置

## 使用方法

在下载程序时，选择对应的配置文件：

```bash
# 使用CMSIS-DAP
openocd -f config/openocd_dap.cfg -c "program firmware.bin verify reset exit"

# 使用J-Link
openocd -f config/openocd_jlink.cfg -c "program firmware.bin verify reset exit"
```