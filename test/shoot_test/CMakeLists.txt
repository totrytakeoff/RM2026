# =============================================================================
# @file test/shoot_test/CMakeLists.txt
# @brief Shooter test firmware CMake config
# @project RM2026
# @author Codex
# @version 1.0.0
# @date 2025-12-07
# @details Builds a standalone shooter test that drives 3x M3508 (2 friction + 1 loader).
# =============================================================================

project(shoot_test)

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
