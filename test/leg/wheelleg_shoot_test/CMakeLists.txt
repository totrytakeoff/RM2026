# =============================================================================
# @file test/wheelleg_shoot_test/CMakeLists.txt
# @brief Wheel-leg gimbal + shooter integration test firmware CMake config
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2026-01-06
# @details 构建轮腿云台 + 发射机构联测固件（ET08 遥控）
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(wheelleg_shoot_test C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(wheelleg_shoot_test
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

