# =============================================================================
# @file hal/CMakeLists.txt
# @brief CubeMX 生成的 HAL 源码构建配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 收集 HAL 源/启动文件，生成 HAL_Lib 静态库与 HAL_IRQ OBJECT 库，并设置必须的包含/编译选项。
# =============================================================================

# CubeMX 生成源码显式登记；中断处理文件单独作为 OBJECT target。
set(HAL_SOURCES
    adc.c
    can.c
    crc.c
    dac.c
    dma.c
    freertos.c
    gpio.c
    i2c.c
    rng.c
    rtc.c
    spi.c
    stm32f4xx_hal_msp.c
    stm32f4xx_hal_timebase_tim.c
    system_stm32f4xx.c
    tim.c
    usart.c
    usb_device.c
    usbd_cdc_if.c
    usbd_conf.c
    usbd_desc.c
)

# 添加启动文件 (.s)
list(APPEND HAL_SOURCES "${CMAKE_SOURCE_DIR}/config/linker/startup_stm32f407xx.s")

# 设置汇编文件属性
set_source_files_properties("${CMAKE_SOURCE_DIR}/config/linker/startup_stm32f407xx.s" 
    PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# 创建静态库
add_library(HAL_Lib STATIC ${HAL_SOURCES})

# 中断处理文件单独建 OBJECT 库，避免被归档优化掉
add_library(HAL_IRQ OBJECT stm32f4xx_it.c)
target_include_directories(HAL_IRQ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Inc
)
target_link_libraries(HAL_IRQ PUBLIC rm_vendor_stm32_hal)
target_compile_options(HAL_IRQ PRIVATE ${MCU_FLAGS})

# 包含路径
target_include_directories(HAL_Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                                     # hal 自身
    ${CMAKE_SOURCE_DIR}/Inc                                          # 全局 Inc
    # usbd_conf.c currently uses the platform monotonic timer during USB init.
    ${CMAKE_SOURCE_DIR}/platform/stm32f4/dwt
    ${CMAKE_SOURCE_DIR}/platform/stm32f4/log
)

# 添加编译选项以确保 FPU 设置一致
target_compile_options(HAL_Lib PRIVATE ${MCU_FLAGS})

# 链接依赖
target_link_libraries(HAL_Lib PUBLIC
    rm_vendor_runtime
    rm_vendor_stm32_hal
)

message(STATUS "Library HAL_Lib Configured.")
