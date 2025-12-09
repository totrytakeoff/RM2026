
set(DSP_LIB "${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/ST/ARM/DSP/Lib/libarm_cortexM4lf_math.a")

if(NOT EXISTS ${DSP_LIB})
    message(FATAL_ERROR "无法找到 DSP 库文件，请检查 Src/CMakeLists.txt 中的 DSP_LIB 路径是否正确: ${DSP_LIB}")
endif()

# 定义一个宏，用于快速创建测试目标
macro(create_test_executable TEST_NAME SOURCE_FILES)
    add_executable(${TEST_NAME}.elf ${SOURCE_FILES})
    
    # 添加编译选项以确保 FPU 设置一致
    target_compile_options(${TEST_NAME}.elf PRIVATE ${MCU_FLAGS})
    
    # 链接核心库，使用链接组处理循环依赖
    target_link_libraries(${TEST_NAME}.elf 
        -Wl,--start-group
        HNUYueLuRM_Framework
        HAL_Lib 
        -Wl,--end-group
        ${DSP_LIB}
        m
    )
    
    # 设置链接脚本 (测试通常也需要链接脚本)
    target_link_options(${TEST_NAME}.elf PRIVATE 
        ${MCU_FLAGS}
        -T${CMAKE_SOURCE_DIR}/config/linker/STM32F407IGHx_FLASH.ld
        -Wl,--no-warn-rwx-segments
        -specs=nano.specs 
        -specs=nosys.specs
    )
    
    # 包含全局 Inc
    target_include_directories(${TEST_NAME}.elf PUBLIC ${CMAKE_SOURCE_DIR}/Inc)
endmacro()

# --------------------------------------------------------
# 1. 配置 motor_test
# --------------------------------------------------------
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/motor_test")
    file(GLOB_RECURSE MOTOR_TEST_SRC "motor_test/*.cpp" "motor_test/*.c")
    # 排除掉可能存在的 build 目录下的文件
    list(FILTER MOTOR_TEST_SRC EXCLUDE REGEX ".*build/.*")
    
    create_test_executable(test_motor "${MOTOR_TEST_SRC}")
    message(STATUS "Test Target Added: test_motor.elf")
endif()

# --------------------------------------------------------
# 2. 配置 test_imu
# --------------------------------------------------------
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test_imu")
    # 注意：test_imu 下有自己的 main.c
    file(GLOB_RECURSE IMU_TEST_SRC "test_imu/*.c" "test_imu/*.cpp")
    list(FILTER IMU_TEST_SRC EXCLUDE REGEX ".*build/.*") # 排除 build 垃圾
    list(FILTER IMU_TEST_SRC EXCLUDE REGEX ".*cmake.*")  # 排除 cmake 文件

    create_test_executable(test_imu "${IMU_TEST_SRC}")
    message(STATUS "Test Target Added: test_imu.elf")
endif()