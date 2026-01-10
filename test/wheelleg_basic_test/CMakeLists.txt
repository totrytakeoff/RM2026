# =============================================================================
# @file test/wheelleg_basic_test/CMakeLists.txt
# @brief 轮腿底盘基础模块测试（IMU + 轮毂 + 关节）
# =============================================================================

project(wheelleg_basic_test)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

file(GLOB SOURCES "*.c" "*.cpp")

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
