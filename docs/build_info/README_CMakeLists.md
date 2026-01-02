# RoboMaster 嵌入式框架 - CMake 分层设计说明

本项目采用“根 CMakeLists + 多子目录”模式来组织 STM32 工程。所有 MCU 相关配置集中在 `cmake/`，具体功能划分到 `lib/`、`hal/`、`Src/` 与 `test/`。本文概述各层的职责以及如何扩展。

## 结构示意

```
RM2026/
├── CMakeLists.txt
├── cmake/
│   ├── Toolchain.cmake
│   ├── Functions.cmake
│   └── TestFunctions.cmake
├── lib/
│   └── HNUYueLuRM/        # Drivers/Middlewares/BSP/Modules/Common
├── hal/                   # CubeMX HAL 源码 + IRQ
├── Src/                   # main.c 与 application/*
└── test/
    ├── CMakeLists.txt     # 自动遍历子目录
    ├── motor_test/
    └── test_imu/
```

## 根 CMakeLists.txt

- 自动加载 `cmake/Toolchain.cmake`，统一设置编译器、MCU Flags、链接脚本等。
- 依序 `add_subdirectory(lib)` → `add_subdirectory(hal)` → `add_subdirectory(Src)` → `add_subdirectory(test)`，确保依赖先行编译。
- 通过 `include(cmake/Functions.cmake)`、`include(CTest)` 提供辅助函数与测试开关。
- 定义通用目标：`clean-all`、`upload`、`upload-verbose`、`verify`。这些命令直接调用 OpenOCD，把 `build/Src/app.elf` 下载或校验至 STM32F407。

## cmake/ 目录

| 文件 | 作用 |
| --- | --- |
| `Toolchain.cmake` | 指定 `arm-none-eabi-*` 工具链、`MCU_FLAGS`、`MCU_LINKER_SCRIPT`、`DSP_LIB` 以及默认 `CMAKE_BUILD_TYPE`。 |
| `Functions.cmake` | `color_message()` 与 `target_include_directories_recursively()` 等辅助函数，供 lib/Src 等模块复用。 |
| `TestFunctions.cmake` | 定义 `create_embedded_test()` 与 `create_unit_test()`，用于在 `test/*` 中快速生成目标、HEX/BIN 以及 OpenOCD/JLink 下载命令。 |

## 子模块 CMakeLists

### lib/HNUYueLuRM

- 将开源框架划分为多个静态库：`HNUYueLuRM_drivers`、`_middlewares`、`_common`、`_bsp`、`_modules`。
- 每个库都继承 `MCU_FLAGS`，并通过 `target_include_directories`/`target_include_directories_recursively` 暴露头文件。
- 最终提供 `HNUYueLuRM_Framework`（INTERFACE 库）作为聚合目标，方便其它模块整体链接。

### hal/

- `hal/CMakeLists.txt` 使用 `file(GLOB ...)` 收集 CubeMX 生成的 `*.c`，并额外把 `startup_stm32f407xx.s` 加入 HAL 静态库。
- `HAL_Lib`：封装所有 HAL 源码与 USB/FreeRTOS/SEGGER 依赖的 include。
- `HAL_IRQ`：把 `stm32f4xx_it.c` 构建为 OBJECT 库，在链接时直接注入，保证中断向量不会被静态链接器优化掉。

### Src/

- 收集 `main.c` 与 `application/**` 的全部 `.c/.cpp`，构建成 `app.elf`。
- 通过 `target_include_directories_recursively()` 将 `application` 下的子目录自动加入 include path。
- 链接顺序中加入 `-Wl,--start-group/--end-group`，确保多个静态库间的循环依赖被正确解析。
- 后处理命令生成 `.hex`、`.bin`，并写出 `.map`、`--print-memory-usage` 报告。

### test/

- 顶层 `test/CMakeLists.txt` 会遍历所有子目录，只要含有 `CMakeLists.txt` 就 `add_subdirectory`。
- 各测试子目录只需 `include(cmake/TestFunctions.cmake)` 并调用 `create_embedded_test()`，即可自动继承主工程的工具链、HAL/HNU 库、DSP 链接等配置。
- `create_embedded_test()` 在 `${TEST_OUTPUT_DIR}`（默认为 `build/tests`）下生成 ELF/HEX/BIN，并附带 `flash-test_<name>`、`build-and-flash-test_<name>` 与（若找到 JLink）`flash-test_<name>-jlink`。

## 扩展建议

### 新增测试项目

1. 在 `test/` 下创建目录，例如 `imu_calibration/`。
2. 复制 `test/motor_test/CMakeLists.txt`，替换 `project()` 名称。
3. 根据需要设置 `SOURCES`、`EXTRA_SOURCES`、`INCLUDE_DIRS`、`LINK_LIBRARIES` 等参数；默认会自动链接 HAL/HNU/DSP。
4. 重新运行 `cmake --build build`，即可生成 `test_imu_calibration` 目标以及对应的下载命令。

### 引入新的子库或模块

- 如果是 HNU 框架内部库，可直接在 `lib/HNUYueLuRM/CMakeLists.txt` 中追加一个 `add_library()` 并通过 `HNUYueLuRM_Framework` 链接。
- 若是项目级组件（如新增 `Src/foo/`），建议在 `Src/CMakeLists.txt` 中通过 `file(GLOB_RECURSE ...)` 或手动追加源文件，同时使用 `target_include_directories()` 暴露头文件。

## 目标依赖 & 输出

- `app.elf` 自动依赖 `HNUYueLuRM_*` 与 `HAL_Lib/HAL_IRQ`。构建顺序由 CMake 负责，无需手工指定。
- 主固件产物位于 `build/Src/`，测试产物位于 `build/tests/<test_name>/`，JLink 脚本位于 `build/flash_<test_name>.jlink`。
- 全局 `clean-all` 通过 `cmake -E remove_directory` 清理 `build/`，不会误删源码。

理解以上分层后，就可以更容易地定位编译问题或添加模块，避免在巨大单一的 CMakeLists 中迷失。若需要进一步调试，可在任意目标上调用 `print_build_info(<target>)`（来自 `cmake/Functions.cmake`）查看源文件、编译/链接选项。
