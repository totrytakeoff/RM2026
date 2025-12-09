cmake_minimum_required(VERSION 3.16)

# =============================================================================
# 项目基本设置
# =============================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 在 project 之前强制指定编译器
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

project(RM2026 C CXX ASM)

# 引入辅助函数
include(cmake/Functions.cmake)

# =============================================================================
# 全局编译选项 (MCU Flags)
# =============================================================================
# 将 MCU 标志设为缓存变量，使其在子目录中可见
# The flags must be in a semicolon-separated list to be treated as separate arguments
set(MCU_FLAGS "-mcpu=cortex-m4;-mthumb;-mthumb-interwork;-mfloat-abi=hard;-mfpu=fpv4-sp-d16" CACHE STRING "MCU specific compiler flags")

add_compile_options(
    ${MCU_FLAGS} -pipe -Wall -Wno-unused-variable -fmessage-length=0 
    -ffunction-sections -fdata-sections -fno-common
)
# C语言标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

# 调试/发布模式设置
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_options(-Og -g -gdwarf-2)
    add_definitions(-DDEBUG)
else()
    add_compile_options(-Ofast)
endif()

# 全局宏定义
add_definitions(
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
    -DARM_MATH_CM4
)

# =============================================================================
# 添加子模块 (顺序很重要)
# =============================================================================
# 1. 编译 Lib (HNUYueLuRM_Framework: 包含官方驱动 + 机器人框架)
add_subdirectory(lib)

# 2. 编译 HAL (HAL_Lib: 包含 CubeMX 生成的配置代码 + 启动文件)
add_subdirectory(hal)

# 3. 编译 Src (App: 包含 main.c 和 application 逻辑)
add_subdirectory(Src)

# 4. 编译 Test (测试代码)
# add_subdirectory(test)

# =============================================================================
# 自定义全局命令 (Clean / Upload)
# =============================================================================
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Removing build directory..."
)

# OpenOCD 下载命令 (默认下载 app.bin)
# 注意：这里假设最终生成的目标在 Src 目录下，如果找不到需要在 Src/CMakeLists 指定输出位置
set(OPENOCD_CFG "${CMAKE_SOURCE_DIR}/config/openocd/openocd_dap.cfg")
add_custom_target(upload
    COMMAND openocd -f ${OPENOCD_CFG} -c "program ${CMAKE_BINARY_DIR}/Src/app.bin verify reset exit"
    DEPENDS app.elf
    COMMENT "Uploading to STM32..."
)
