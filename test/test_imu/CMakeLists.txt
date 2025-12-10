# =============================================================================
# @file test/test_imu/CMakeLists.txt
# @brief IMU 测试固件的 CMake 配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 定义 test_imu 工程并调用 create_embedded_test()，链接框架与 HAL 以输出可下载的固件。
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(test_imu C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(imu
    SOURCES
        main.c
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    LINK_LIBRARIES
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_middlewares
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
    LINKER_SCRIPT ${MCU_LINKER_SCRIPT}
)
