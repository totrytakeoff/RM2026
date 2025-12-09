# RoboMaster 嵌入式框架 - 分层CMakeLists系统

## 项目概述

本项目基于HNUYueLuRM开源框架，采用分层CMakeLists管理系统，将开源框架封装为静态链接库，并提供模块化的单元测试管理。

## 项目结构

```
framework/
├── CMakeLists.txt              # 根CMakeLists文件，管理整个项目
├── lib/                        # 开源框架库
│   ├── HNUYueLuRM/            # HNUYueLuRM开源框架源代码
│   └── CMakeLists.txt         # 将HNUYueLuRM编译为静态库
├── application/                # 应用层代码
│   └── CMakeLists.txt         # 构建应用程序
├── test/                       # 单元测试
│   ├── motor_test/            # 电机测试
│   │   ├── CMakeLists.txt     # 电机测试CMakeLists
│   │   └── main.cpp           # 测试源文件
│   ├── test_imu/              # IMU测试
│   │   ├── CMakeLists.txt     # IMU测试CMakeLists
│   │   └── main.c             # 测试源文件
│   └── CMakeLists.txt         # 测试管理CMakeLists
├── Inc/                        # 公共头文件
├── Src/                        # 公共源文件
└── config/                     # 配置文件
    └── linker/                 # 链接脚本
```

## 构建系统说明

### 分层CMakeLists设计

本项目采用分层CMakeLists设计，各层职责如下：

1. **根CMakeLists.txt**：负责全局配置、工具链设置、MCU参数配置和子目录管理
2. **lib/CMakeLists.txt**：将HNUYueLuRM开源框架编译为静态链接库
3. **application/CMakeLists.txt**：构建应用程序，链接静态库
4. **test/CMakeLists.txt**：管理所有单元测试项目
5. **test/*/CMakeLists.txt**：各测试项目的具体构建配置

### 编译命令

#### 基本构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 构建所有目标
make -j$(nproc)
```

#### 分层构建

```bash
# 仅构建静态库
make HNUYueLuRM_Lib

# 仅构建应用程序
make basic_framework

# 构建特定测试项目
make test_motor_test
make test_test_imu

# 构建所有测试项目
make test
```

#### 下载命令

```bash
# 使用DAP下载应用程序
make download-dap

# 使用JLink下载应用程序
make download-jlink

# 使用DAP下载测试项目
make download-motor_test-dap
make download-test_imu-dap

# 一键编译并烧录
make build-and-flash
```

### 自定义目标

- `clean-all`：清理所有构建文件
- `info`：显示项目信息
- `download-dap`：使用DAP下载程序
- `download-jlink`：使用JLink下载程序
- `build-and-flash`：编译并使用DAP下载程序

## 添加新测试项目

要添加新的测试项目，请按以下步骤操作：

1. 在`test`目录下创建新的测试目录，例如`test_new_module`
2. 在新目录中创建测试源文件
3. 复制现有测试目录的CMakeLists.txt并修改项目名称和源文件
4. 在`test/CMakeLists.txt`中添加新测试目录到`add_subdirectory`命令

## 注意事项

1. 确保ARM交叉编译工具链已正确安装并配置
2. 确保OpenOCD或JLink工具已安装，用于下载程序
3. 根据实际硬件修改链接脚本和配置文件
4. 新增测试项目时，确保正确链接HNUYueLuRM静态库

## 技术细节

### 静态库构建

lib/CMakeLists.txt将HNUYueLuRM开源框架编译为静态库，包括：
- Drivers：STM32 HAL驱动
- Middlewares：FreeRTOS、SEGGER RTT等中间件
- bsp：板级支持包
- modules：功能模块（IMU、电机等）

### 测试项目构建

每个测试项目独立构建，但共享：
- 相同的工具链和MCU配置
- HNUYueLuRM静态库
- 公共头文件和源文件

### 输出文件组织

构建输出文件按类型组织：
- ELF文件：`${CMAKE_BINARY_DIR}/output/` 或 `${CMAKE_BINARY_DIR}/test_output/`
- 静态库：`${CMAKE_BINARY_DIR}/obj/`
- HEX/BIN文件：与ELF文件同目录

## 故障排除

1. **编译错误**：检查工具链是否正确安装，路径是否正确
2. **链接错误**：确保静态库已正确构建，检查链接脚本路径
3. **下载失败**：检查下载工具是否正确安装，硬件连接是否正常

## 联系方式

如有问题，请联系：YZ-Control/myself