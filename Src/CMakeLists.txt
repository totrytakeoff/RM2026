# =============================================================================
# @file Src/CMakeLists.txt
# @brief 主固件 app.elf 构建脚本
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 收集 main.c 与 application 目录源码，设置包含/链接选项，并生成 ELF/HEX/BIN 及映射文件。
# =============================================================================

# 链接脚本
if(NOT DEFINED LINKER_SCRIPT)
    set(LINKER_SCRIPT "${MCU_LINKER_SCRIPT}")
endif()

if(NOT EXISTS ${DSP_LIB})
    message(FATAL_ERROR "无法找到 DSP 库文件，请检查 cmake/Toolchain.cmake 中的 DSP_LIB 路径是否正确: ${DSP_LIB}")
endif()


# 1. 收集 main.c
set(APP_SOURCES "main.c")

# 2. 递归收集 application 下的所有源文件
file(GLOB_RECURSE APP_LOGIC_SOURCES "application/*.c" "application/*.cpp")
list(APPEND APP_SOURCES ${APP_LOGIC_SOURCES})

# 创建可执行文件
add_executable(app.elf ${APP_SOURCES})

# 包含路径 (application 内部的头文件)
target_include_directories_recursively(app.elf ${CMAKE_CURRENT_SOURCE_DIR}/application)
target_include_directories(app.elf PUBLIC ${CMAKE_SOURCE_DIR}/Inc)

# 链接库
# 使用链接组 (--start-group) 来处理静态库之间复杂的或潜在的循环依赖。
# 这能确保链接器在 HNUYueLuRM_Framework 和 HAL_Lib 之间反复查找符号，直到所有依赖都解决。
# 同时链接 DSP 库和标准数学库 m。
target_link_libraries(app.elf 
    PRIVATE
    -Wl,--start-group
    HNUYueLuRM_modules
    HNUYueLuRM_bsp
    HNUYueLuRM_middlewares
    HNUYueLuRM_common
    HNUYueLuRM_drivers
    HAL_Lib
    $<TARGET_OBJECTS:HAL_IRQ>
    -Wl,--end-group
    ${DSP_LIB}
    m
    c
)

# 编译选项 (确保 app.elf 的源文件使用正确的 FPU 设置)
target_compile_options(app.elf PRIVATE ${MCU_FLAGS})

# 链接选项
target_link_options(app.elf PRIVATE 
    ${GENERIC_LINK_OPTIONS}
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/app.map 
    -Wl,--print-memory-usage
)

# 生成 Hex 和 Bin
add_custom_command(TARGET app.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    COMMENT "Generating BIN and HEX files..."
)
