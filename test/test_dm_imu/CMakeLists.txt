# =============================================================================
# @file test/test_dm_imu/CMakeLists.txt
# @brief DM-IMU 测试固件的 CMake 配置
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(test_dm_imu C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(dm_imu
    SOURCES
        main.c
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    LINK_LIBRARIES
        DM_IMU
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_middlewares
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
    LINKER_SCRIPT ${MCU_LINKER_SCRIPT}
)
