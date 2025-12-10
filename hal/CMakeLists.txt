# =============================================================================
# @file hal/CMakeLists.txt
# @brief CubeMX 生成的 HAL 源码构建配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 收集 HAL 源/启动文件，生成 HAL_Lib 静态库与 HAL_IRQ OBJECT 库，并设置必须的包含/编译选项。
# =============================================================================

# 收集 hal 目录下的所有 CubeMX 生成的源文件（中断向量/IT 单独处理）
file(GLOB HAL_SOURCES "adc.c" "can.c" "crc.c" "dac.c" "dma.c" "freertos.c" "gpio.c" "i2c.c" "rng.c" "rtc.c" "spi.c" "stm32f4xx_hal_msp.c" "stm32f4xx_hal_timebase_tim.c" "system_stm32f4xx.c" "tim.c" "usart.c" "usb_device.c" "usbd_cdc_if.c" "usbd_conf.c" "usbd_desc.c")

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
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Include
)
target_compile_options(HAL_IRQ PRIVATE ${MCU_FLAGS})

# 包含路径
target_include_directories(HAL_Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}                                     # hal 自身
    ${CMAKE_SOURCE_DIR}/Inc                                          # 全局 Inc
    # HAL 库需要 STM32 官方驱动的头文件
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Drivers/CMSIS/Include
    # USB device library headers
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    # FreeRTOS headers - 添加完整的FreeRTOS路径
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    # SEGGER RTT headers
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/SEGGER/RTT
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/Middlewares/Third_Party/SEGGER/Config
    # BSP headers
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/bsp
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/bsp/dwt
    ${CMAKE_SOURCE_DIR}/lib/HNUYueLuRM/bsp/log
)

# 添加编译选项以确保 FPU 设置一致
target_compile_options(HAL_Lib PRIVATE ${MCU_FLAGS})

# 链接依赖
target_link_libraries(HAL_Lib PUBLIC
    HNUYueLuRM_middlewares
    HNUYueLuRM_drivers
    HNUYueLuRM_common
)

message(STATUS "Library HAL_Lib Configured.")
