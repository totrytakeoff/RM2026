# =============================================================================
# @file test/infantry/infantry_gimbal_spin_et08_demo/CMakeLists.txt
# @brief Infantry ET08云台+底盘自旋测试 CMake 配置
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(infantry_gimbal_spin_et08_demo C ASM)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

create_embedded_test(infantry_gimbal_spin_et08_demo
    SOURCES
        ${CMAKE_SOURCE_DIR}/test/infantry/infantry_gimbal_et08_demo/main.c
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/test/infantry/infantry_gimbal_et08_demo
        ${CMAKE_SOURCE_DIR}/test/infantry/infantry_minimal
    LINK_LIBRARIES
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
    LINKER_SCRIPT ${MCU_LINKER_SCRIPT}
)

target_compile_definitions(test_infantry_gimbal_spin_et08_demo
    PRIVATE
        GIMBAL_SPIN_DEMO_ENABLE=1
)
