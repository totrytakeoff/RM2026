# =============================================================================
# @file lib/DM/CMakeLists.txt
# @brief DM 系列模块库
# =============================================================================

file(GLOB_RECURSE DM_IMU_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/DM-IMU/*.c"
)

add_library(DM_IMU STATIC ${DM_IMU_SOURCES})

target_include_directories(DM_IMU PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/DM-IMU
)

target_compile_options(DM_IMU PRIVATE ${MCU_FLAGS})

target_link_libraries(DM_IMU PUBLIC
    HNUYueLuRM_bsp
    HNUYueLuRM_common
    HNUYueLuRM_drivers
)
