# =============================================================================
# @file test/wheelleg_imu_test/CMakeLists.txt
# @brief Wheel-leg IMU basic test (DM-IMU over CAN2)
# =============================================================================

project(wheelleg_imu_test)

include(${CMAKE_SOURCE_DIR}/cmake/TestFunctions.cmake)

file(GLOB SOURCES "*.c" "*.cpp")

create_embedded_test(${PROJECT_NAME}
    SOURCES ${SOURCES}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
    LINK_LIBRARIES
        DM_IMU
        HNUYueLuRM_modules
        HNUYueLuRM_bsp
        HNUYueLuRM_middlewares
        HNUYueLuRM_common
        HNUYueLuRM_drivers
        HAL_Lib
)
