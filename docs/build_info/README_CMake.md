# RM2026 CMake 构建指南

当前构建系统按源码所有权组织，不再通过单一框架库聚合第三方代码、
平台适配和机器人逻辑。完整的目录目标与迁移约束见
[仓库重组基线](../architecture/repository_reorganization_baseline.md)。

## 构建层次

```text
third_party -> hal/platform -> components -> application -> system -> firmware -> app.elf
```

- `third_party/`：CMSIS、CMSIS-DSP、STM32 HAL、FreeRTOS、USB Device 和
  SEGGER RTT；每项依赖均有独立 vendor target。
- `hal/`：当前 CubeMX 生成的板级初始化代码和中断对象。
- `platform/stm32f4/`：CAN、UART、SPI、DWT、日志、USB 等平台适配。
- `components/`：算法、设备驱动和通用服务。
- `application/infantry/`：步兵机器人行为与参数。
- `system/freertos/`：任务、调度策略和 RTOS hooks。
- `firmware/infantry/`：正式固件组合层。
- `Src/`：迁移阶段的 `app.elf` 入口。
- `test/`：板级测试和主机单元测试。

## 环境与常用命令

需要 CMake 3.16 以上版本和可从 `PATH` 找到的
`arm-none-eabi-gcc/g++/objcopy/size`。OpenOCD 仅在烧录时需要。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target app.elf test_infantry_minimal --parallel
cmake --build build --parallel
```

正式产物会同时生成到 `build/Src/` 和 `build/output/`。板级测试产物位于
`build/tests/<name>/`。

主机安全测试使用本机编译器，必须单独配置：

```bash
cmake -S test/unit -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

烧录与校验命令：

```bash
cmake --build build --target upload
cmake --build build --target verify
```

## 主要目标

| 层 | 实现 target | CMake alias |
| --- | --- | --- |
| CMSIS | `rm_vendor_cmsis` | `RM::vendor_cmsis` |
| STM32 HAL driver | `rm_vendor_stm32_hal` | `RM::vendor_stm32_hal` |
| FreeRTOS | `rm_vendor_freertos` | `RM::vendor_freertos` |
| USB Device | `rm_vendor_usb_device` | `RM::vendor_usb_device` |
| SEGGER RTT | `rm_vendor_segger_rtt` | `RM::vendor_segger_rtt` |
| STM32F4 platform | `rm_platform_stm32f4` | `RM::platform_stm32f4` |
| reusable components | `rm_components` | `RM::components` |
| safety service | `rm_service_safety` | `RM::service_safety` |
| infantry application | `rm_app_infantry` | `RM::app_infantry` |
| FreeRTOS system | `rm_system_freertos` | `RM::system_freertos` |
| infantry firmware | `rm_firmware_infantry` | `RM::firmware_infantry` |

`HAL_Lib` 和 `HAL_IRQ` 是 CubeMX 内容迁入具体 board 目录前保留的过渡目标。
`rm_components` 也是行为保持阶段的临时聚合目标，后续会按算法、服务和设备
继续拆分。

## 构建约束

- 维护中的 target 必须显式列出源码和公开 include 目录。
- 新代码不得使用递归源码或递归 include 搜索。
- 第三方代码只放在 `third_party/`，不得依赖仓库自有 target。
- 依赖默认使用 `PRIVATE`；只有公开头文件暴露依赖时才使用 `PUBLIC`。
- 正式结构调整后必须同时通过主机测试、`app.elf`、
  `test_infantry_minimal` 和全量板级构建。
- `--start-group` 仍是迁移期兼容措施；组件依赖闭环拆除后必须移除。
