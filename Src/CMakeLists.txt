# 链接脚本
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/config/linker/STM32F407IGHx_FLASH.ld")

# -----------------------------------------------------------------------------
# 1. 定义 DSP 库路径 (请根据你实际文件位置修改这里!!)
# -----------------------------------------------------------------------------
# 尝试在标准位置寻找 DSP 库，如果你的库文件名不同，请修改这里
set(DSP_LIB "${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/ST/ARM/DSP/Lib/libarm_cortexM4lf_math.a")

if(NOT EXISTS ${DSP_LIB})
    message(FATAL_ERROR "无法找到 DSP 库文件，请检查 Src/CMakeLists.txt 中的 DSP_LIB 路径是否正确: ${DSP_LIB}")
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
    HNUYueLuRM_Framework
    HAL_Lib
    -Wl,--end-group
    ${DSP_LIB}
    m
)

# 编译选项 (确保 app.elf 的源文件使用正确的 FPU 设置)
target_compile_options(app.elf PRIVATE ${MCU_FLAGS})

# 链接选项
target_link_options(app.elf PRIVATE 
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT} 
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/app.map 
    -Wl,--gc-sections 
    -Wl,--print-memory-usage
    -Wl,--no-warn-rwx-segments
    -specs=nano.specs 
    -specs=nosys.specs
)

# 生成 Hex 和 Bin
add_custom_command(TARGET app.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app.elf> ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    COMMENT "Generating BIN and HEX files..."
)