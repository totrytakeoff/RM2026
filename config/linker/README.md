# STM32F407IGHx 链接脚本

此目录包含STM32F407IGHx微控制器的链接脚本和启动文件。

## 文件说明

- `STM32F407IGHx_FLASH.ld` - STM32F407IGHx的链接脚本，定义内存布局
- `startup_stm32f407xx.s` - STM32F407xx的启动文件

## 使用方法

在项目构建时，通过CMake或Makefile引用这些文件：

```makefile
LDSCRIPT = config/STM32F407IGHx_FLASH.ld
```

或

```cmake
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/config/STM32F407IGHx_FLASH.ld")
```