# =============================================================================
# @file test/basic_test/pitch_comp_scan_demo/CMakeLists.txt
# @brief Pitch补偿扫描 Demo
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(pitch_comp_scan_demo C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(pitch_comp_scan_demo
    SOURCES
        main.c
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test/infantry/infantry_minimal
    LINK_LIBRARIES
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_middlewares
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
    LINKER_SCRIPT ${MCU_LINKER_SCRIPT}
)
