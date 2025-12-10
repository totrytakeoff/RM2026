# =============================================================================
# @file test/CMakeLists.txt
# @brief 嵌入式测试项目聚合配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 自动遍历 test 子目录并调用各自的 CMakeLists，以便统一创建 test_<name> 目标和输出目录。
# =============================================================================

# 寻找所有直接子目录
file(GLOB TEST_SUBDIRS LIST_DIRECTORIES true ./*)

# 遍历所有找到的路径
foreach(SUBDIR ${TEST_SUBDIRS})
    # 确保它是一个目录并且包含一个 CMakeLists.txt
    if(IS_DIRECTORY ${SUBDIR} AND EXISTS "${SUBDIR}/CMakeLists.txt")
        # 提取目录名用于打印消息
        get_filename_component(TEST_NAME ${SUBDIR} NAME)
        # 将子目录添加到构建中
        add_subdirectory(${SUBDIR})
        message(STATUS "Added Test Project: ${TEST_NAME}")
    endif()
endforeach()

# Force re-evaluation - v1
