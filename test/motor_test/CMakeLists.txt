# =============================================================================
# @file test/motor_test/CMakeLists.txt
# @brief 电机测试固件的 CMake 配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 通过 create_embedded_test() 生成 test_motor_test 目标，并继承框架库与 HAL 依赖。
# =============================================================================

project(motor_test)

# Include the cmake file that defines our test creation function
include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

# This test only needs its own source files.
# All other dependencies are handled by linking against the framework libraries.
file(GLOB SOURCES "*.c" "*.cpp")

# Create the test executable by calling the correct function.
# It will link against HAL_Lib and HNUYueLuRM_Lib, inheriting their PUBLIC include directories.
create_embedded_test(${PROJECT_NAME}
    SOURCES ${SOURCES}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    LINK_LIBRARIES
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_middlewares
        HNUYueLuRM_common
        HNUYueLuRM_drivers
)
