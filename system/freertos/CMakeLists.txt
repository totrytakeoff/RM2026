add_library(rm_system_freertos STATIC
    infantry_tasks.c
    rtos_hooks.c
)
add_library(RM::system_freertos ALIAS rm_system_freertos)

target_include_directories(rm_system_freertos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(rm_system_freertos PRIVATE ${MCU_FLAGS})

target_link_libraries(rm_system_freertos PUBLIC
    rm_app_infantry
    rm_components_runtime
    rm_platform_stm32f4
    rm_vendor_runtime
    HAL_Lib
)
