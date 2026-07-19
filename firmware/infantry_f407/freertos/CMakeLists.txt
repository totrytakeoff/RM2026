add_library(rm_firmware_infantry_freertos STATIC
    infantry_tasks.c
    rtos_hooks.c
)
add_library(RM::firmware_infantry_freertos ALIAS rm_firmware_infantry_freertos)

target_include_directories(rm_firmware_infantry_freertos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(rm_firmware_infantry_freertos PRIVATE ${MCU_FLAGS})

target_link_libraries(rm_firmware_infantry_freertos PUBLIC
    rm_app_infantry
    rm_components_runtime
    rm_platform_stm32f4
    rm_vendor_runtime
)
