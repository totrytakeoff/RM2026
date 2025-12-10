# =============================================================================
# @file CMakeLists.txt
# @brief RM2026 顶层 CMake 配置入口
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 统一加载 Toolchain、组织 lib/hal/Src/test 子目录并定义 upload/clean 等全局目标。
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# =============================================================================
# 项目基本设置（简化入口，只保留全局配置和子目录组织）
# =============================================================================
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/Toolchain.cmake" CACHE FILEPATH "Toolchain file")
endif()
include(${CMAKE_TOOLCHAIN_FILE})

project(RM2026 C CXX ASM)

include(cmake/Functions.cmake)
include(CTest)
enable_testing()

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
# 定义所有测试项目的输出根目录, 以便 TestFunctions.cmake 使用
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/tests CACHE INTERNAL "Root directory for test outputs")
add_subdirectory(test)

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

# 查找OpenOCD可执行文件
find_program(OPENOCD_EXECUTABLE 
    NAMES openocd
    PATHS 
        /home/myself/.platformio/packages/tool-openocd/bin
        /usr/bin
        /usr/local/bin
    NO_DEFAULT_PATH
)

# 如果找不到，尝试使用系统PATH
if(NOT OPENOCD_EXECUTABLE)
    find_program(OPENOCD_EXECUTABLE openocd)
endif()

# 设置OpenOCD路径变量
if(OPENOCD_EXECUTABLE)
    set(OPENOCD_PATH ${OPENOCD_EXECUTABLE})
    message(STATUS "Found OpenOCD: ${OPENOCD_PATH}")
else()
    # 设置默认路径作为备用
    set(OPENOCD_PATH "/home/myself/.platformio/packages/tool-openocd/bin/openocd")
    message(WARNING "OpenOCD not found, using default path: ${OPENOCD_PATH}")
endif()

add_custom_target(upload
    COMMAND ${CMAKE_COMMAND} -E echo "[UPLOAD] Starting upload to STM32..."
    COMMAND ${OPENOCD_PATH} -f ${OPENOCD_CFG} -c "init; halt; flash write_image erase ${CMAKE_BINARY_DIR}/Src/app.elf 0x08000000; verify_image ${CMAKE_BINARY_DIR}/Src/app.elf 0x08000000; reset; shutdown"
    COMMAND ${CMAKE_COMMAND} -E echo "[SUCCESS] Upload completed successfully!"
    DEPENDS app.elf
    COMMENT "Uploading to STM32..."
)

# 添加一个更详细的上传命令，显示更多调试信息
add_custom_target(upload-verbose
    COMMAND ${CMAKE_COMMAND} -E echo "[UPLOAD-VERBOSE] Starting verbose upload to STM32..."
    COMMAND ${OPENOCD_PATH} -f ${OPENOCD_CFG} -c "init; halt; flash write_image erase ${CMAKE_BINARY_DIR}/Src/app.elf 0x08000000; verify_image ${CMAKE_BINARY_DIR}/Src/app.elf 0x08000000; reset; shutdown"
    COMMAND ${CMAKE_COMMAND} -E echo "[SUCCESS] Upload completed successfully!"
    DEPENDS app.elf
    COMMENT "Verbose upload to STM32..."
)

# 添加一个仅验证命令，不重新烧录
add_custom_target(verify
    COMMAND ${CMAKE_COMMAND} -E echo "[VERIFY] Verifying STM32 flash..."
    COMMAND ${OPENOCD_PATH} -f ${OPENOCD_CFG} -c "init; halt; verify_image ${CMAKE_BINARY_DIR}/Src/app.elf 0x08000000; shutdown"
    COMMAND ${CMAKE_COMMAND} -E echo "[SUCCESS] Verification completed!"
    DEPENDS app.elf
    COMMENT "Verifying STM32 flash..."
)
