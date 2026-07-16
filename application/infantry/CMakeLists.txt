add_library(rm_infantry_config INTERFACE)
add_library(RM::infantry_config ALIAS rm_infantry_config)

target_include_directories(rm_infantry_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

add_library(rm_app_infantry STATIC
    infantry_app.c
    command/infantry_input.c
    chassis/infantry_chassis.c
    diagnostics/infantry_debug.c
    gimbal/infantry_gimbal.c
    referee/infantry_referee.c
    shoot/infantry_shoot.c
)
add_library(RM::app_infantry ALIAS rm_app_infantry)

target_include_directories(rm_app_infantry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/command
    ${CMAKE_CURRENT_SOURCE_DIR}/chassis
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/gimbal
    ${CMAKE_CURRENT_SOURCE_DIR}/referee
    ${CMAKE_CURRENT_SOURCE_DIR}/shoot
)

target_compile_options(rm_app_infantry PRIVATE ${MCU_FLAGS})

# Transitional dependencies. These are replaced component-by-component during
# the ownership migration; application code no longer lives in the legacy lib.
target_link_libraries(rm_app_infantry PUBLIC
    rm_infantry_config
    rm_service_safety
    rm_components
    rm_platform_stm32f4
    rm_common
    rm_vendor_stm32_hal
    HAL_Lib
    m
)
