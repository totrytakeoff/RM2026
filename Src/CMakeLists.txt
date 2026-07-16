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

# 统一输出目录（由顶层 CMakeLists 定义，若未定义则退回到 build/output）
if(NOT DEFINED GLOBAL_OUTPUT_DIR)
    set(GLOBAL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)
endif()

# 正式入口。应用源码由分层target显式提供，不再递归收集旧application目录。
set(APP_SOURCES "main.c")

# 创建可执行文件
add_executable(app.elf ${APP_SOURCES})
set_property(TARGET app.elf APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

target_include_directories(app.elf PUBLIC ${CMAKE_SOURCE_DIR}/Inc)

# 链接库
# 迁移阶段仍使用链接组处理现有静态库之间的循环依赖；后续按组件拆分后移除。
# vendor 归档显式放入组内，确保 FreeRTOS 回调和平台适配符号能被重新扫描。
target_link_libraries(app.elf 
    PRIVATE
    rm_platform_syscalls
    -Wl,--start-group
    rm_firmware_infantry
    rm_system_freertos
    rm_app_infantry
    rm_components_runtime
    rm_platform_stm32f4
    rm_vendor_freertos
    rm_vendor_usb_device
    rm_vendor_segger_rtt
    rm_common
    rm_vendor_stm32_hal
    HAL_Lib
    $<TARGET_OBJECTS:HAL_IRQ>
    -Wl,--end-group
    m
    c
)

# 编译选项 (确保 app.elf 的源文件使用正确的 FPU 设置)
target_compile_options(app.elf PRIVATE ${MCU_FLAGS})

# 链接选项
target_link_options(app.elf PRIVATE 
    ${GENERIC_LINK_OPTIONS}
    -T${LINKER_SCRIPT}
    -Wl,--defsym=RM_MIN_HEAP_SIZE=0
    -Wl,--defsym=RM_MIN_STACK_SIZE=0x2000
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/app.map 
    -Wl,--print-memory-usage
)

# 生成 Hex 和 Bin
add_custom_command(TARGET app.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DNM=${CMAKE_NM}
            -DELF=$<TARGET_FILE:app.elf>
            -P ${CMAKE_SOURCE_DIR}/cmake/CheckFormalFirmwareSymbols.cmake
    COMMAND ${CMAKE_COMMAND}
            -DREADELF=${CMAKE_READELF}
            -DELF=$<TARGET_FILE:app.elf>
            -P ${CMAKE_SOURCE_DIR}/cmake/CheckElfSegments.cmake
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLOBAL_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:app.elf> ${GLOBAL_OUTPUT_DIR}/app.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/app.hex ${GLOBAL_OUTPUT_DIR}/app.hex
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/app.bin ${GLOBAL_OUTPUT_DIR}/app.bin
    COMMENT "Generating BIN and HEX files..."
)
