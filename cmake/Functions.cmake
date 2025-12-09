# =============================================================================
# RoboMaster 嵌入式框架 - 通用CMake函数
# =============================================================================
# 项目: cmake_functions
# 描述: 定义项目中使用的通用CMake函数
# 作者: YZ-Control/myself
# 版本: 1.0.0
# 日期: 2025-12-07
# =============================================================================


# 递归寻找子目录并添加为包含路径的函数
# 用法: include_sub_directories_recursively(TARGET_NAME ROOT_DIR)
function(target_include_directories_recursively target dir)
    if (IS_DIRECTORY ${dir})
        target_include_directories(${target} PUBLIC ${dir})
    endif()

    file(GLOB ALL_SUB RELATIVE ${dir} ${dir}/*)
    foreach(sub ${ALL_SUB})
        if (IS_DIRECTORY ${dir}/${sub})
            target_include_directories_recursively(${target} ${dir}/${sub})
        endif()
    endforeach()
endfunction()