# =============================================================================
# 测试项目函数
# =============================================================================
# 描述: 创建嵌入式测试项目的通用函数
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
        set(ARG_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/config/linker/STM32F407IGHx_FLASH.ld")
    endif()
    
    # 测试项目名称
    set(test_target_name "test_${test_name}")
    
    # 汇编源文件
    set(ASM_SOURCES
        ${CMAKE_SOURCE_DIR}/config/linker/startup_stm32f407xx.s
    )
    
    # 创建测试可执行文件
    add_executable(${test_target_name}
        ${ARG_SOURCES}
        ${ARG_EXTRA_SOURCES}
        ${ASM_SOURCES}
    )
    
    # 设置汇编文件属性
    set_source_files_properties(${ASM_SOURCES}
        PROPERTIES
            LANGUAGE ASM
            COMPILE_FLAGS "-x assembler-with-cpp"
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
            application
            hal
            HNUYueLuRM_Lib
            rm2026_compile_options
            rm2026_link_options
            test_compile_options
            ${ARG_LINK_LIBRARIES}
    )
    
    # 链接选项
    target_link_options(${test_target_name} PRIVATE
        -T${ARG_LINKER_SCRIPT}
    )
    
    # 设置输出目录
    set_target_properties(${test_target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}/${test_name}
        OUTPUT_NAME ${test_target_name}
    )
    
    # 创建输出目录
    file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR}/${test_name})
    
    # 生成HEX和BIN文件
    add_custom_command(TARGET ${test_target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${test_target_name}> ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.hex
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${test_target_name}> ${TEST_OUTPUT_DIR}/${test_name}/${test_target_name}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${test_target_name}>
        COMMENT "为测试项目 ${test_name} 生成HEX和BIN文件"
    )
    
    # 添加到测试目标列表
    add_test_executable(${test_target_name})
    
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
    
    # 添加到测试目标列表
    add_test_executable(${test_target_name})
    
    message(STATUS "创建单元测试: ${test_target_name}")
endfunction()
