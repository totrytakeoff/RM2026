# =============================================================================
# @file test/infantry/infantry_minimal/CMakeLists.txt
# @brief Infantry最小框架 CMake配置
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(infantry_minimal C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(infantry_minimal
    SOURCES
        main.c
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    LINK_LIBRARIES
        rm_app_infantry
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
    LINKER_SCRIPT ${MCU_LINKER_SCRIPT}
)
