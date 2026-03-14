# =============================================================================
# @file test/CMakeLists.txt
# @brief 嵌入式测试项目聚合配置
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 自动遍历 test 子目录并调用各自的 CMakeLists，以便统一创建 test_<name> 目标和输出目录。
# =============================================================================

# 递归查找 test/ 下所有子目录中的 CMakeLists.txt（不包含当前文件自身）
file(GLOB_RECURSE TEST_CMAKELIST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*/CMakeLists.txt")

# 排序保证输出稳定，便于排查问题
list(SORT TEST_CMAKELIST_FILES)

set(TEST_SUBDIRS "")

foreach(CMAKELIST_FILE ${TEST_CMAKELIST_FILES})
    # 从 "xxx/CMakeLists.txt" 提取目录 "xxx"
    get_filename_component(SUBDIR ${CMAKELIST_FILE} DIRECTORY)

    # 跳过空目录（理论不会出现）
    if(SUBDIR STREQUAL "")
        continue()
    endif()

    # 过滤掉常见构建目录，避免误把临时构建产物中的 CMakeLists 加入工程
    if(SUBDIR MATCHES "(^|/)(build|cmake-build[^/]*)($|/)")
        message(STATUS "Skip build directory in test/: ${SUBDIR}")
        continue()
    endif()

    list(APPEND TEST_SUBDIRS ${SUBDIR})
endforeach()

list(REMOVE_DUPLICATES TEST_SUBDIRS)

# 将所有测试子目录加入构建，支持多级分类目录
foreach(SUBDIR ${TEST_SUBDIRS})
    add_subdirectory(${SUBDIR})
    message(STATUS "Added Test Project Dir: ${SUBDIR}")
endforeach()
