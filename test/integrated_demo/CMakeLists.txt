# =============================================================================
# @file test/integrated_demo/CMakeLists.txt
# @brief 综合演示固件（麦轮底盘 + 云台 + 英雄射击）
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2026-01-04
# @details 统一遥控映射与 CAN ID，整合三个 demo 的控制逻辑
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(integrated_demo C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(integrated_demo
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
