# RM2026 CMake构建系统

## 项目结构

本项目采用CMake构建系统，将代码组织为以下几个主要部分：

- **lib**: HNUYueLuRM开源框架，编译成静态库
- **src**: STM32 HAL初始化函数，编译成静态库
- **application**: 应用层代码，编译成可执行文件
- **test**: 单元测试项目，每个测试编译成独立的可执行文件

## 构建步骤

### 1. 创建构建目录

```bash
mkdir build && cd build
```

### 2. 配置项目

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### 3. 编译项目

```bash
make -j$(nproc)
```

## 常用命令

### 构建特定组件

```bash
# 构建静态库
make HNUYueLuRM_Lib    # 构建HNUYueLuRM静态库
make Src_Lib           # 构建Src静态库

# 构建应用程序
make basic_framework   # 构建主应用程序

# 构建测试项目
make test_motor_test   # 构建电机测试
make test_test_imu     # 构建IMU测试
```

### 清理项目

```bash
make clean-all         # 清理所有构建文件
```

### 查看项目信息

```bash
make info              # 显示项目信息
```

### 下载程序

#### 主程序下载

```bash
make upload            # 使用DAP下载主程序
make upload-jlink      # 使用JLink下载主程序
```

#### 应用层程序下载

```bash
make download-app-dap    # 使用DAP下载
make download-app-jlink # 使用JLink下载
```

#### 测试程序下载

```bash
# 电机测试
make download-motor_test-dap    # 使用DAP下载motor_test
make download-motor_test-jlink  # 使用JLink下载motor_test

# IMU测试
make download-test_imu-dap      # 使用DAP下载test_imu
make download-test_imu-jlink    # 使用JLink下载test_imu
```

### 一键编译并烧录

```bash
make build-and-flash    # 编译并使用DAP下载主程序到STM32
make build-and-flash-app # 编译并下载应用层程序
make build-and-flash-motor_test
make build-and-flash-test_imu
```

## 构建类型

支持以下构建类型：

- **Debug**: 最小优化，包含调试信息（默认）
- **Release**: 最大速度优化
- **RelWithDebInfo**: 最大速度优化，包含调试信息
- **MinSizeRel**: 最大尺寸优化

例如，使用Release模式构建：

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

## 输出文件

构建完成后，输出文件位于以下目录：

- **ELF文件**: `build/output/`
- **静态库**: `build/obj/`
- **测试输出**: `build/test_output/`

## 注意事项

1. 确保已安装ARM交叉编译工具链
2. 下载程序需要安装相应的下载工具（OpenOCD或JLink）
3. 首次构建前，确保已正确安装所有依赖项