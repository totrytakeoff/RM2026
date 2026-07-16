# Embedded regression and bring-up images.  Keep this list explicit: adding a
# directory must not silently add a new firmware image to every build.

set(RM_EMBEDDED_TEST_DIRS
    basic_test/et08_sbus_demo
    basic_test/gimbal_demo
    basic_test/mecanum_chassis_demo
    basic_test/motor_test
    basic_test/omni_chassis_demo
    basic_test/pitch_comp_scan_demo
    basic_test/relay_test
    basic_test/remote_control_demo
    basic_test/shoot_test
    basic_test/test_dm_imu
    basic_test/test_imu
    basic_test/vt_link_monitor_demo
    hero/hero_shoot_test
    hero/integrated_demo
    hero/integrated_vt_km_demo
    infantry/infantry_chassis_et08_demo
    infantry/infantry_chassis_gimbal_et08_demo
    infantry/infantry_chassis_gimbal_et08_demo1
    infantry/infantry_gimbal_et08_demo
    infantry/infantry_minimal
    infantry/infantry_omni_demo
    infantry/infantry_shoot_et08_demo
    infantry/infantry_shoot_test
    leg/wheelleg_all_test
    leg/wheelleg_balance_test
    leg/wheelleg_basic_test
    leg/wheelleg_imu_test
    leg/wheelleg_joint_pos_test
    leg/wheelleg_joint_test
    leg/wheelleg_shoot_test
)

foreach(test_dir IN LISTS RM_EMBEDDED_TEST_DIRS)
    add_subdirectory(${test_dir})
endforeach()

# test/unit is intentionally a standalone host project and is never loaded by
# the ARM cross-compilation tree.
