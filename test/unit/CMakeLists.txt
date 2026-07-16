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

add_library(rm_device_health_host STATIC
    ../../components/services/device_health/daemon.c
)
target_include_directories(rm_device_health_host PUBLIC
    ../../components/services/device_health
    ../../platform/stm32f4/time
)
target_compile_features(rm_device_health_host PUBLIC c_std_11)
target_compile_options(rm_device_health_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_device_health
    device_health_test.c
)
target_link_libraries(test_device_health PRIVATE rm_device_health_host)
target_compile_features(test_device_health PRIVATE c_std_11)
target_compile_options(test_device_health PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_test(NAME device_health COMMAND test_device_health)

add_library(rm_format_host STATIC
    ../../platform/stm32f4/utils/utils.c
)
target_include_directories(rm_format_host PUBLIC
    ../../platform/stm32f4/utils
)
target_compile_features(rm_format_host PUBLIC c_std_11)
target_compile_options(rm_format_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_format
    format_test.c
)
target_link_libraries(test_format PRIVATE rm_format_host m)
target_compile_features(test_format PRIVATE c_std_11)
target_compile_options(test_format PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_test(NAME format COMMAND test_format)

add_library(rm_rx_queue_host STATIC
    ../../platform/common/transport/rm_rx_queue.c
)
target_include_directories(rm_rx_queue_host PUBLIC
    ../../platform/common/transport
)
target_compile_features(rm_rx_queue_host PUBLIC c_std_11)
target_compile_options(rm_rx_queue_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_rx_queue
    rx_queue_test.c
)
target_link_libraries(test_rx_queue PRIVATE rm_rx_queue_host)
target_compile_features(test_rx_queue PRIVATE c_std_11)
target_compile_options(test_rx_queue PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_test(NAME rx_queue COMMAND test_rx_queue)
