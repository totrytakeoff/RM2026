# =============================================================================
# @file test/wheelleg_joint_test/CMakeLists.txt
# @brief Wheel-leg joint MIT control test (DM8009P)
# =============================================================================

project(wheelleg_joint_test)

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
        HAL_Lib
)
