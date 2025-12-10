# =============================================================================
# RoboMaster 嵌入式框架 - /test 目录 CMakeLists
# =============================================================================
# 描述: 自动发现并构建所有测试子项目。
#       每个子目录都应该包含一个独立的 CMakeLists.txt，
#       它使用根目录 cmake/Functions.cmake 中定义的 create_test_project() 函数。
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
