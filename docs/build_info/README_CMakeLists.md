# RM2026 CMake target 维护说明

本文说明新增或调整源码时应修改哪个 `CMakeLists.txt`。构建命令和 target
总表见 [CMake 构建指南](README_CMake.md)。

## 根目录加载顺序

根 `CMakeLists.txt` 按以下顺序注册子目录：

```text
third_party
hal
platform
components
application
system
firmware
Src
test
```

这个顺序同时表达依赖方向。低层目录不得反向依赖高层目录。

## 各层职责

### `third_party/CMakeLists.txt`

每个外部依赖使用独立 target、独立公开头文件路径和显式源码列表。
FreeRTOS 的 `heap_4.c` 被明确排除，因为正式配置禁止动态分配。
CMSIS-DSP 的预编译归档由 `DSP_LIB` 指向。

### `hal/CMakeLists.txt`

`HAL_Lib` 暂时承载 CubeMX 生成的外设初始化、USB glue、系统时钟和启动文件；
`HAL_IRQ` 是 OBJECT library，保证中断实现直接进入最终镜像。待板级目录迁移完成
后，这两个目标将归入 `platform/boards/infantry_f407`。

### `platform/stm32f4/CMakeLists.txt`

维护 MCU/板级适配源码和其公开头文件。平台层可以依赖 vendor target，不能依赖
机器人应用。新增平台后端时应优先建立窄 target，而不是继续扩大公共 include 面。

### `components/CMakeLists.txt`

当前使用 `rm_components` 显式聚合算法、设备和服务，以确保结构迁移阶段行为不变。
安全服务和 DM IMU 已有独立 target。后续拆分时，每个组件应拥有自己的源码列表、
公开 include 和最小依赖。

### `application`、`system` 与 `firmware`

- `application/infantry` 只描述机器人行为和配置；最终不得包含 HAL/RTOS 头文件。
- `system/freertos` 管理任务入口、周期、栈、优先级和 hooks。
- `firmware/infantry` 组合具体应用、运行时和板级绑定。
- `Src/app.elf` 目前负责最终链接和产物转换，迁移完成后并入具体固件目录。

## 新增代码

新增第三方依赖：

1. 将原始发行内容放到 `third_party/<name>/`。
2. 保留许可证文件。
3. 在 `third_party/CMakeLists.txt` 建立 `rm_vendor_<name>` 和
   `RM::vendor_<name>`。
4. 显式列出实际使用的源码，不使用 `GLOB_RECURSE`。

新增仓库自有组件：

1. 放到 `components/algorithms`、`components/devices` 或
   `components/services` 中正确的所有权目录。
2. 建立窄 target，并只公开组件 API 所需的 include 目录。
3. 将 HAL/传输细节留在 platform 或设备适配层。
4. 为纯逻辑部分补充主机测试，再接入正式固件。

新增板级测试：

1. 在 `test/<category>/<name>/` 添加源码与 `CMakeLists.txt`。
2. 使用 `create_embedded_test()`，显式填写 `LINK_LIBRARIES`。
3. 构建单个 `test_<name>` target，并在提交前执行全量构建。

## 提交前检查

```bash
rg 'GLOB_RECURSE|target_include_directories_recursively' \
  third_party platform components application system firmware
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

第一条命令应无输出。来源与许可证说明只保留在
`THIRD_PARTY_NOTICES.md` 及必要的架构历史文档中。
