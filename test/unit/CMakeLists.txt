cmake_minimum_required(VERSION 3.16)

project(rm2026_host_unit_tests C)

enable_testing()

add_library(rm_safety_host STATIC
    ../../components/services/safety/safety_manager.c
)
target_include_directories(rm_safety_host PUBLIC
    ../../components/services/safety
)
target_compile_features(rm_safety_host PUBLIC c_std_11)
target_compile_options(rm_safety_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_safety_manager
    safety_manager_test.c
)
target_link_libraries(test_safety_manager PRIVATE rm_safety_host)
target_compile_features(test_safety_manager PRIVATE c_std_11)
target_compile_options(test_safety_manager PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_test(NAME safety_manager COMMAND test_safety_manager)
