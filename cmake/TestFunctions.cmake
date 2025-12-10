# =============================================================================
# @file cmake/TestFunctions.cmake
# @brief RoboMaster 嵌入式测试项目函数集合
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 提供 create_embedded_test() 与 create_unit_test()，统一处理链接、下载命令与产物输出路径。
# =============================================================================

# 创建嵌入式测试项目的函数
function(create_embedded_test test_name)
    # 解析参数
    set(options "")
    set(oneValueArgs "LINKER_SCRIPT")
    set(multiValueArgs "SOURCES" "EXTRA_SOURCES" "INCLUDE_DIRS" "LINK_LIBRARIES")
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # 设置默认链接脚本
    if(NOT ARG_LINKER_SCRIPT)
        set(ARG_LINKER_SCRIPT "${MCU_LINKER_SCRIPT}")
    endif()
    
    # 测试项目名称
    set(test_target_name "test_${test_name}")
    
    # 创建测试可执行文件
    # 启动文件 (.s) 已被包含在 HAL_Lib 中，此处不再需要单独添加。
    # 只需要添加测试本身的源文件即可。
    add_executable(${test_target_name}
        ${ARG_SOURCES}
        ${ARG_EXTRA_SOURCES}
        ${CMAKE_SOURCE_DIR}/test/error_handler_stub.c
    )
    
    # 汇编文件属性也不再需要，因为它在 HAL_Lib 中处理
    
    if(NOT DSP_LIB OR NOT EXISTS ${DSP_LIB})
        message(FATAL_ERROR "DSP_LIB not found: ${DSP_LIB}")
    endif()

    # 包含目录
    target_include_directories(${test_target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/Inc
            ${ARG_INCLUDE_DIRS}
    )
    
    # 链接库 (mirroring the main app.elf linkage)
    target_link_libraries(${test_target_name}
        PUBLIC
        -Wl,--start-group
        ${ARG_LINK_LIBRARIES}
        HAL_Lib
        $<TARGET_OBJECTS:HAL_IRQ>
        -Wl,--end-group
        ${DSP_LIB}
        m
        c
    )
    
    # 链接选项 (mirroring the main app.elf options)
    target_link_options(${test_target_name} PRIVATE 
        ${GENERIC_LINK_OPTIONS}
        -T${ARG_LINKER_SCRIPT} 
        -Wl,-Map=${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.map 
        -Wl,--print-memory-usage
    )
    
    # 设置输出目录
    # 目标输出为 .elf，并放到 tests/<name>/ 目录，方便调试和烧录
    set_target_properties(${test_target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}/${test_name}
        OUTPUT_NAME ${test_target_name}
        SUFFIX ".elf"
    )
    
    # 创建输出目录
    file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR}/${test_name})
    
    # 生成HEX和BIN文件
    add_custom_command(TARGET ${test_target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${test_target_name}> ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.hex
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${test_target_name}> ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${test_target_name}> ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.elf
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${test_target_name}>
        COMMENT "为测试项目 ${test_name} 生成 ELF / HEX / BIN 文件"
    )
    
    # 添加到测试目标列表 (此函数未定义,暂时注释掉)
    # add_test_executable(${test_target_name})
    
    # =============================================================================
    # 下载目标
    # =============================================================================
    
    # OpenOCD下载目标
    find_program(OPENOCD openocd PATHS
        /usr/bin
        /usr/local/bin
        /home/myself/.platformio/packages/tool-openocd/bin
        /opt/openocd/bin
    )
    
    if(OPENOCD)
        add_custom_target(flash-${test_target_name}
            COMMAND ${OPENOCD} 
                    -f ${CMAKE_SOURCE_DIR}/config/openocd/openocd_dap.cfg 
                    -c init -c halt 
                    -c "flash write_image erase ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin 0x08000000" 
                    -c "verify_image ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin 0x08000000"
                    -c reset -c shutdown
            COMMENT "下载测试项目 ${test_name} 到STM32"
            DEPENDS ${test_target_name}
        )
        
        add_custom_target(build-and-flash-${test_target_name}
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${test_target_name}
            COMMAND ${OPENOCD} 
                    -f ${CMAKE_SOURCE_DIR}/config/openocd/openocd_dap.cfg 
                    -c init -c halt 
                    -c "flash write_image erase ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin 0x08000000" 
                    -c "verify_image ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin 0x08000000"
                    -c reset -c shutdown
            COMMENT "编译并下载测试项目 ${test_name} 到STM32"
        )
    endif()
    
    # JLink下载目标
    find_program(JLINK_EXE JLinkExe PATHS
        /usr/bin
        /usr/local/bin
        /opt/SEGGER/JLink
    )
    
    if(JLINK_EXE)
        # 创建JLink脚本文件
        file(WRITE ${CMAKE_BINARY_DIR}/flash_${test_name}.jlink
            "device STM32F407IG\n"
            "speed 4000\n"
            "connect\n"
            "erase\n"
            "loadfile ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.hex\n"
            "r\n"
            "go\n"
            "exit\n"
        )
        
        add_custom_target(flash-${test_target_name}-jlink
            COMMAND ${JLINK_EXE} -CommanderScript ${CMAKE_BINARY_DIR}/flash_${test_name}.jlink
            COMMENT "使用JLink下载测试项目 ${test_name} 到STM32"
            DEPENDS ${test_target_name}
        )
    endif()
    
    # =============================================================================
    # 输出信息
    # =============================================================================
    message(STATUS "创建测试项目: ${test_target_name}")
    message(STATUS "  - 源文件: ${ARG_SOURCES}")
    message(STATUS "  - 额外源文件: ${ARG_EXTRA_SOURCES}")
    message(STATUS "  - 链接库: ${ARG_LINK_LIBRARIES}")
    message(STATUS "  - 输出目录: ${TEST_OUTPUT_DIR}/${test_name}")
    
endfunction()

# =============================================================================
# 创建单元测试函数（用于主机测试）
# =============================================================================
function(create_unit_test test_name)
    # 解析参数
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs "SOURCES" "INCLUDE_DIRS" "LINK_LIBRARIES")
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # 测试目标名称
    set(test_target_name "unit_test_${test_name}")
    
    # 创建测试可执行文件（主机环境）
    add_executable(${test_target_name}
        ${ARG_SOURCES}
    )
    
    # 包含目录
    target_include_directories(${test_target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/Inc
            ${ARG_INCLUDE_DIRS}
    )
    
    # 链接库
    target_link_libraries(${test_target_name}
        PRIVATE
            ${ARG_LINK_LIBRARIES}
    )
    
    # 添加为测试
    add_test(NAME ${test_target_name}
        COMMAND ${test_target_name}
    )
    
    # 添加到测试目标列表 (此函数未定义,暂时注释掉)
    # add_test_executable(${test_target_name})
    
    message(STATUS "创建单元测试: ${test_target_name}")
endfunction()
