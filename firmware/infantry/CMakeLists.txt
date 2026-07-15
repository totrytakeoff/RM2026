add_library(rm_firmware_infantry STATIC
    infantry_firmware.c
)
add_library(RM::firmware_infantry ALIAS rm_firmware_infantry)

target_include_directories(rm_firmware_infantry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(rm_firmware_infantry PRIVATE ${MCU_FLAGS})

target_link_libraries(rm_firmware_infantry PUBLIC
    rm_system_freertos
    rm_app_infantry
    HNUYueLuRM_bsp
    HAL_Lib
)
