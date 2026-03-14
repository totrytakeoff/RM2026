# =============================================================================
# @file test/infantry_omni_demo/CMakeLists.txt
# @brief Infantry omni demo CMake config
# @project RM2026
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(infantry_omni_demo C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(infantry_omni_demo
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
