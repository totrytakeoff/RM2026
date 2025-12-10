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
)
