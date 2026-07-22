cmake_minimum_required(VERSION 3.16)

project(rm2026_host_unit_tests C CXX)

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

add_executable(test_remote_control_adapter
    remote_control_adapter_test.cpp
)
target_include_directories(test_remote_control_adapter PRIVATE
    ../../components/devices/remote
)
target_compile_features(test_remote_control_adapter PRIVATE cxx_std_14)
target_compile_options(test_remote_control_adapter PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME remote_control_adapter COMMAND test_remote_control_adapter)

add_library(rm_infantry_et08_profile_host STATIC
    ../../applications/infantry/command/infantry_et08_input_profile.cpp
)
target_include_directories(rm_infantry_et08_profile_host PUBLIC
    ../../applications/infantry/command
    ../../applications/infantry/include
    ../../applications/infantry/referee
    ../../components/devices/remote
    ../../components/devices/remote/et08
)
target_compile_features(rm_infantry_et08_profile_host PUBLIC cxx_std_14)
target_compile_options(rm_infantry_et08_profile_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_infantry_et08_input_profile
    infantry_et08_input_profile_test.cpp
)
target_link_libraries(test_infantry_et08_input_profile PRIVATE
    rm_infantry_et08_profile_host
    rm_safety_host
)
target_compile_features(test_infantry_et08_input_profile PRIVATE cxx_std_14)
target_compile_options(test_infantry_et08_input_profile PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME infantry_et08_input_profile
         COMMAND test_infantry_et08_input_profile)

add_library(rm_infantry_gimbal_limits_host STATIC
    ../../applications/infantry/gimbal/infantry_gimbal_limits.c
)
target_include_directories(rm_infantry_gimbal_limits_host PUBLIC
    ../../applications/infantry/gimbal
)
target_compile_features(rm_infantry_gimbal_limits_host PUBLIC c_std_11)
target_compile_options(rm_infantry_gimbal_limits_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_infantry_gimbal_limits
    infantry_gimbal_limits_test.c
)
target_link_libraries(test_infantry_gimbal_limits PRIVATE
    rm_infantry_gimbal_limits_host
)
target_compile_features(test_infantry_gimbal_limits PRIVATE c_std_11)
target_compile_options(test_infantry_gimbal_limits PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME infantry_gimbal_limits COMMAND test_infantry_gimbal_limits)

add_library(rm_infantry_gimbal_axis_state_host STATIC
    ../../applications/infantry/gimbal/infantry_gimbal_axis_state.c
)
target_include_directories(rm_infantry_gimbal_axis_state_host PUBLIC
    ../../applications/infantry/gimbal
    ../../applications/infantry/include
    ../../applications/infantry/referee
)
target_compile_features(rm_infantry_gimbal_axis_state_host PUBLIC c_std_11)
target_compile_options(rm_infantry_gimbal_axis_state_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_infantry_gimbal_axis_state
    infantry_gimbal_axis_state_test.c
)
target_link_libraries(test_infantry_gimbal_axis_state PRIVATE
    rm_infantry_gimbal_axis_state_host
    m
)
target_compile_features(test_infantry_gimbal_axis_state PRIVATE c_std_11)
target_compile_options(test_infantry_gimbal_axis_state PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME infantry_gimbal_axis_state
         COMMAND test_infantry_gimbal_axis_state)

add_executable(test_infantry_gimbal_units
    infantry_gimbal_units_test.c
)
target_include_directories(test_infantry_gimbal_units PRIVATE
    ../../applications/infantry/gimbal
)
target_compile_features(test_infantry_gimbal_units PRIVATE c_std_11)
target_compile_options(test_infantry_gimbal_units PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
target_link_libraries(test_infantry_gimbal_units PRIVATE m)
add_test(NAME infantry_gimbal_units COMMAND test_infantry_gimbal_units)

add_executable(test_infantry_shoot_config
    infantry_shoot_config_test.c
)
target_include_directories(test_infantry_shoot_config PRIVATE
    stubs
    ../../applications/infantry/config
)
target_compile_features(test_infantry_shoot_config PRIVATE c_std_11)
target_compile_options(test_infantry_shoot_config PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
target_link_libraries(test_infantry_shoot_config PRIVATE m)
add_test(NAME infantry_shoot_config COMMAND test_infantry_shoot_config)

add_library(rm_infantry_shoot_trigger_host STATIC
    ../../applications/infantry/shoot/infantry_shoot_trigger.c
)
target_include_directories(rm_infantry_shoot_trigger_host PUBLIC
    ../../applications/infantry/shoot
)
target_compile_features(rm_infantry_shoot_trigger_host PUBLIC c_std_11)
target_compile_options(rm_infantry_shoot_trigger_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_infantry_shoot_trigger
    infantry_shoot_trigger_test.c
)
target_link_libraries(test_infantry_shoot_trigger PRIVATE
    rm_infantry_shoot_trigger_host
)
target_compile_features(test_infantry_shoot_trigger PRIVATE c_std_11)
target_compile_options(test_infantry_shoot_trigger PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME infantry_shoot_trigger COMMAND test_infantry_shoot_trigger)

add_library(rm_infantry_chassis_kinematics_host STATIC
    ../../applications/infantry/chassis/infantry_chassis_follow.c
    ../../applications/infantry/chassis/infantry_chassis_kinematics.c
)
target_include_directories(rm_infantry_chassis_kinematics_host PUBLIC
    ../../applications/infantry/chassis
)
target_compile_features(rm_infantry_chassis_kinematics_host PUBLIC c_std_11)
target_compile_options(rm_infantry_chassis_kinematics_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_infantry_chassis_kinematics
    infantry_chassis_kinematics_test.c
)
target_link_libraries(test_infantry_chassis_kinematics PRIVATE
    rm_infantry_chassis_kinematics_host
    m
)
target_compile_features(test_infantry_chassis_kinematics PRIVATE c_std_11)
target_compile_options(test_infantry_chassis_kinematics PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
add_test(NAME infantry_chassis_kinematics
         COMMAND test_infantry_chassis_kinematics)

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

add_library(rm_pid_host STATIC
    ../../components/algorithms/pid/controller.c
)
target_include_directories(rm_pid_host PUBLIC
    stubs
    ../../components/algorithms/pid
)
target_compile_features(rm_pid_host PUBLIC c_std_11)
target_compile_options(rm_pid_host PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(test_pid_reset
    pid_reset_test.c
)
target_link_libraries(test_pid_reset PRIVATE rm_pid_host m)
target_compile_features(test_pid_reset PRIVATE c_std_11)
target_compile_options(test_pid_reset PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_test(NAME pid_reset COMMAND test_pid_reset)
