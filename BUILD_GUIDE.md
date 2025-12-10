# RM2026 项目构建指南

本项目已优化了编译输出和上传命令，提供了更友好的用户体验。

## 新增功能

### 1. 彩色编译输出
- 编译器输出现在使用彩色显示，更容易区分警告和错误
- 添加了 `-fdiagnostics-color=always` 编译选项

### 2. 优化的上传命令
- `upload`: 标准上传命令，构建并上传到STM32
- `upload-verbose`: 详细模式上传，显示更多调试信息
- `verify`: 仅验证STM32上的固件，不重新烧录

### 3. 便捷的构建脚本
创建了一个 `build.sh` 脚本，提供以下命令：

```bash
./build.sh clean      # 清理构建目录
./build.sh build      # 仅构建项目
./build.sh upload     # 构建并上传到STM32
./build.sh verbose    # 详细模式构建并上传
./build.sh verify     # 验证STM32上的固件
./build.sh help       # 显示帮助信息
```

## 使用方法

### 方法1：使用构建脚本（推荐）

```bash
# 构建项目
./build.sh build

# 构建并上传
./build.sh upload

# 详细模式上传
./build.sh verbose

# 验证固件
./build.sh verify

# 清理构建目录
./build.sh clean
```

### 方法2：使用CMake和Make

```bash
# 配置项目
cd build
cmake ..

# 构建项目
make

# 上传固件
make upload

# 详细模式上传
make upload-verbose

# 验证固件
make verify
```

## 注意事项

1. 上传命令需要连接STM32设备并确保OpenOCD配置正确
2. 如果上传失败，请检查：
   - 设备是否正确连接
   - OpenOCD配置文件路径是否正确
   - 是否有足够的权限访问设备

## 故障排除

如果遇到"Invalid character escape"错误，请确保使用修复后的CMakeLists.txt文件。

## 内存使用信息

构建成功后，会显示内存使用情况，例如：
```
   text    data     bss     dec     hex  filename
  76780     940   77008  154728   25c68  Src/app.elf
```

- text: 代码段大小
- data: 已初始化数据段大小
- bss: 未初始化数据段大小
- dec: 总大小（十进制）
- hex: 总大小（十六进制）