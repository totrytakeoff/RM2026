# 收集 hal 下所有的 .c 文件
file(GLOB HAL_SOURCES "*.c")

# 添加启动文件 (.s)
# 注意：你的文件结构中 startup 在 config/linker/ 下
list(APPEND HAL_SOURCES "${CMAKE_SOURCE_DIR}/config/linker/startup_stm32f407xx.s")

# 设置汇编文件属性
set_source_files_properties("${CMAKE_SOURCE_DIR}/config/linker/startup_stm32f407xx.s" 
    PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# 创建静态库
add_library(HAL_Lib STATIC ${HAL_SOURCES})

# 包含路径
target_include_directories(HAL_Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                                     # hal 自身
    ${CMAKE_SOURCE_DIR}/Inc                                          # 全局 Inc
    # # HAL 库需要 STM32 官方驱动的头文件
    # ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/STM32F4xx_HAL_Driver/Inc
    # ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    # ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Include
    # # FreeRTOS 头文件
    # ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/FreeRTOS/Source/include
    # ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
)
target_include_directories_recursively(HAL_Lib ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares)
target_include_directories_recursively(HAL_Lib ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers)
target_include_directories_recursively(HAL_Lib ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/bsp)

# 添加编译选项以确保 FPU 设置一致
target_compile_options(HAL_Lib PRIVATE ${MCU_FLAGS})

# 链接依赖
# HAL 层不应该反向依赖上层 Framework。
# 所有的链接关系应该在最终生成可执行文件时处理。
# target_link_libraries(HAL_Lib PUBLIC HNUYueLuRM_Framework)

message(STATUS "Library HAL_Lib Configured.")