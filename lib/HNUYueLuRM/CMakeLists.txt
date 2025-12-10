# =============================================================================
# @file lib/HNUYueLuRM/CMakeLists.txt
# @brief HNUYueLuRM 框架分层静态库配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 构建 drivers/middlewares/common/bsp/modules 各层库并聚合为 HNUYueLuRM_Framework 供上层链接。
# =============================================================================

set(HNU_BASE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------- 
# Drivers
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HNU_DRIVERS_SOURCES
    "${HNU_BASE}/Drivers/STM32F4xx_HAL_Driver/Src/*.c"
)

add_library(HNUYueLuRM_drivers STATIC ${HNU_DRIVERS_SOURCES})
add_library(HNUYueLuRM::drivers ALIAS HNUYueLuRM_drivers)

target_include_directories(HNUYueLuRM_drivers PUBLIC
    ${HNU_BASE}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${HNU_BASE}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${HNU_BASE}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Inc
)
target_compile_options(HNUYueLuRM_drivers PRIVATE ${MCU_FLAGS})

# ----------------------------------------------------------------------------- 
# Middlewares (USB device, FreeRTOS, SEGGER RTT, etc.)
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HNU_MIDDLEWARE_SOURCES
    "${HNU_BASE}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c"
    "${HNU_BASE}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c"
    "${HNU_BASE}/Middlewares/Third_Party/FreeRTOS/Source/*.c"
    "${HNU_BASE}/Middlewares/Third_Party/SEGGER/RTT/*.c"
)

add_library(HNUYueLuRM_middlewares STATIC ${HNU_MIDDLEWARE_SOURCES})
add_library(HNUYueLuRM::middlewares ALIAS HNUYueLuRM_middlewares)

target_include_directories(HNUYueLuRM_middlewares PUBLIC
    ${HNU_BASE}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${HNU_BASE}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${HNU_BASE}/Middlewares/ST/ARM/DSP/Include
    ${HNU_BASE}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${HNU_BASE}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    ${HNU_BASE}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${HNU_BASE}/Middlewares/Third_Party/SEGGER/RTT
    ${HNU_BASE}/Middlewares/Third_Party/SEGGER/Config
    ${CMAKE_SOURCE_DIR}/Inc
)
target_compile_options(HNUYueLuRM_middlewares PRIVATE ${MCU_FLAGS})
target_link_libraries(HNUYueLuRM_middlewares PUBLIC HNUYueLuRM_drivers)

# ----------------------------------------------------------------------------- 
# Common utilities
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HNU_COMMON_SOURCES "${HNU_BASE}/common/*.c")
if(HNU_COMMON_SOURCES)
    add_library(HNUYueLuRM_common STATIC ${HNU_COMMON_SOURCES})
else()
    add_library(HNUYueLuRM_common INTERFACE)
endif()
add_library(HNUYueLuRM::common ALIAS HNUYueLuRM_common)

get_target_property(HNU_COMMON_TYPE HNUYueLuRM_common TYPE)
if(HNU_COMMON_TYPE STREQUAL "INTERFACE_LIBRARY")
    target_include_directories(HNUYueLuRM_common INTERFACE ${HNU_BASE}/common)
else()
    target_include_directories(HNUYueLuRM_common PUBLIC ${HNU_BASE}/common)
    target_compile_options(HNUYueLuRM_common PRIVATE ${MCU_FLAGS})
endif()

# ----------------------------------------------------------------------------- 
# BSP
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HNU_BSP_SOURCES "${HNU_BASE}/bsp/*.c")

add_library(HNUYueLuRM_bsp STATIC ${HNU_BSP_SOURCES})
add_library(HNUYueLuRM::bsp ALIAS HNUYueLuRM_bsp)

target_include_directories_recursively(HNUYueLuRM_bsp ${HNU_BASE}/bsp)
target_include_directories(HNUYueLuRM_bsp PUBLIC ${HNU_BASE}/bsp)
target_compile_options(HNUYueLuRM_bsp PRIVATE ${MCU_FLAGS})
target_link_libraries(HNUYueLuRM_bsp PUBLIC
    HNUYueLuRM_middlewares
    HNUYueLuRM_drivers
    HNUYueLuRM_common
)

# ----------------------------------------------------------------------------- 
# Modules
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HNU_MODULE_SOURCES "${HNU_BASE}/modules/*.c")

add_library(HNUYueLuRM_modules STATIC ${HNU_MODULE_SOURCES})
add_library(HNUYueLuRM::modules ALIAS HNUYueLuRM_modules)

target_include_directories_recursively(HNUYueLuRM_modules ${HNU_BASE}/modules)
target_include_directories(HNUYueLuRM_modules PUBLIC ${HNU_BASE}/modules)
target_compile_options(HNUYueLuRM_modules PRIVATE ${MCU_FLAGS})
target_link_libraries(HNUYueLuRM_modules PUBLIC
    HNUYueLuRM_bsp
    HNUYueLuRM_common
    HNUYueLuRM_middlewares
    HNUYueLuRM_drivers
)

# ----------------------------------------------------------------------------- 
# Framework aggregation target
# -----------------------------------------------------------------------------
add_library(HNUYueLuRM_Framework INTERFACE)
add_library(HNUYueLuRM::framework ALIAS HNUYueLuRM_Framework)

target_link_libraries(HNUYueLuRM_Framework INTERFACE
    HNUYueLuRM_modules
    HNUYueLuRM_bsp
    HNUYueLuRM_middlewares
    HNUYueLuRM_common
    HNUYueLuRM_drivers
)

target_include_directories(HNUYueLuRM_Framework INTERFACE
    ${HNU_BASE}
    ${CMAKE_SOURCE_DIR}/Inc
)

message(STATUS "HNUYueLuRM layered libraries configured.")
