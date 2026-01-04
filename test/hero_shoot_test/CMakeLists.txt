# =============================================================================
# @file test/hero_shoot_test/CMakeLists.txt
# @brief Hero shooter test firmware CMake config
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-14
# @details 构建英雄射击机构遥控测试固件
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(hero_shoot_test C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(hero_shoot_test
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
