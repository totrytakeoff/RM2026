# RM2026 CMake构建系统指南

RM2026 使用标准 CMake 构建流程，但在 `cmake/Toolchain.cmake`、`lib/`、`hal/`、`Src/` 与 `test/` 中做了分层封装。本文档用于快速了解项目中与 CMake 相关的约定、常用目标以及如何扩展。

## 目录速览

```
RM2026/
├── CMakeLists.txt          # 根配置：设置工具链、全局目标、上传命令
├── cmake/
│   ├── Toolchain.cmake     # ARM GCC、MCU Flags、链接脚本、DSP 库等通用配置
│   ├── Functions.cmake     # 公共函数：递归 include、彩色输出等
│   └── TestFunctions.cmake # create_embedded_test() & create_unit_test()
├── lib/                    # HNUYueLuRM 框架分层静态库
├── hal/                    # CubeMX 生成的 HAL/HAL IRQ 源码
├── Src/                    # main.c 与 application/ 业务代码
└── test/                   # 嵌入式测试工程（自动遍历子目录）
```

构建顺序为：`lib` → `hal` → `Src`（主固件） → `test`（按需）。所有目标共享同一套交叉编译器配置和链接脚本。

## 准备环境

1. **ARM GCC 工具链**：需包含 `arm-none-eabi-gcc/g++/objcopy/size`，必须可被 `cmake/Toolchain.cmake` 找到。
2. **CMake ≥ 3.16** 与常用构建工具（`make`/`ninja`），建议直接使用 `cmake --build`。
3. **OpenOCD 或 JLink**（可选）：用于 `upload` 或测试固件 `flash-*` 目标。
4. 如需修改 MCU 配置，可在 `cmake/Toolchain.cmake` 或 CMake 配置阶段覆盖 `MCU_FLAGS`、`MCU_LINKER_SCRIPT`、`DSP_LIB`。

## 快速构建流程

```bash
# 1. 生成构建目录
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 2. 编译主固件
cmake --build build --target app.elf -j

# 3. 使用 OpenOCD 烧录（依赖 config/openocd/openocd_dap.cfg）
cmake --build build --target upload
```

若切换为 Release：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 常见构建目标

- **静态库**
  - `HNUYueLuRM_drivers` / `HNUYueLuRM_middlewares` / `HNUYueLuRM_common` / `HNUYueLuRM_bsp` / `HNUYueLuRM_modules`
  - `HAL_Lib`（CubeMX HAL 源）与 `HAL_IRQ`（独立 OBJECT 库，避免 IRQ 被链接器回收）
- **主固件**
  - `app.elf`：链接上述所有库、`DSP_LIB` 与 C 标准库，生成 `app.elf/.bin/.hex`（产物位于 `build/Src/`）
  - `upload` / `upload-verbose` / `verify`：使用 OpenOCD 下载或校验 `app.elf`
- **清理**
  - `clean-all`：删除整个 `build/` 目录（直接通过 CMake command 实现）
- **测试工程**
  - 自动生成的 `test_<name>` 目标，例如 `test_motor_test`、`test_test_imu`
  - 下载相关目标：`flash-test_<name>`（OpenOCD）与 `build-and-flash-test_<name>`（先构建再烧录）

> 提示：所有嵌入式测试会把 ELF/HEX/BIN 输出到 `build/tests/<测试名>/`，便于调试与烧写。

## 构建类型

支持 CMake 标准的 `Debug`、`Release`、`RelWithDebInfo`、`MinSizeRel`。默认 `Debug`，对应 `-Og -g`，Release 分支会切换为 `-Ofast`。使用示例：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target app.elf
```

## 定制编译配置

- **覆盖 DSP 库或链接脚本**
  ```bash
  cmake -S . -B build \
        -DDSP_LIB=/path/to/libarm_cortexM4lf_math.a \
        -DMCU_LINKER_SCRIPT=/path/to/your.ld
  ```
- **附加编译定义/选项**
  在对应目标上使用 `target_compile_definitions/target_compile_options` 即可，也可在命令行传入 `-DCMAKE_C_FLAGS=...`。
- **Toolchain 自动加载**：根 CMakeLists 会在未显式设置 `CMAKE_TOOLCHAIN_FILE` 时自动包含 `cmake/Toolchain.cmake`，通常无需手动指定。

## 测试项目的增量构建与烧录

1. 在 `test/<your_test>/` 下创建源文件与 `CMakeLists.txt`，调用 `create_embedded_test()`。
2. 重新运行一次 `cmake --build`，根 `test/CMakeLists.txt` 会自动 `add_subdirectory` 新目录。
3. 构建与烧录命令示例：
   ```bash
   cmake --build build --target test_motor_test
   cmake --build build --target build-and-flash-test_motor_test
   ```
4. 输出位于 `build/tests/motor_test/`，包含 `.elf/.bin/.hex/.map`。

## 输出目录对照

- `build/lib/`：HNUYueLuRM 分层静态库中间文件（由 CMake 管理）
- `build/hal/`：HAL_Lib、HAL_IRQ 中间文件
- `build/Src/`：`app.elf/.bin/.hex/.map`
- `build/tests/<name>/`：各测试固件及下载脚本
- `build/flash_<name>.jlink`：JLink 下载脚本（按需生成）

## 常见问题排查

1. **arm-none-eabi-* 找不到**：确认工具链已安装并在 PATH 中，或在 `cmake -S -B` 时显式传入 `-DARM_NONE_EABI_GCC=/path/...` 等变量。
2. **DSP 库路径无效**：根据 `cmake/Toolchain.cmake` 提示更新 `DSP_LIB`，确保文件存在。
3. **OpenOCD 没找到**：脚本会尝试在 `/home/myself/.platformio`, `/usr/bin`, `/usr/local/bin` 搜索，必要时手动安装或调整 `OPENOCD_PATH`。
4. **链接失败**：确认所有静态库均已成功构建；`target_link_libraries` 中使用 `--start-group/--end-group`，如果仍缺符号，请检查新增源码是否正确加入对应库。

按照上述指南即可快速理解 RM2026 的 CMake 构建逻辑，并在此基础上进行扩展或调试。
