# =============================================================================
# RoboMaster 嵌入式框架 - 通用CMake函数
# =============================================================================
# 项目: cmake_functions
# 描述: 定义项目中使用的通用CMake函数
# 作者: YZ-Control/myself
# 版本: 1.0.0
# 日期: 2025-12-07
# =============================================================================

# 定义彩色输出函数
function(color_message color message)
    if(${color} STREQUAL "RED")
        message(STATUS "\033[1;31m${message}\033[0m")
    elseif(${color} STREQUAL "GREEN")
        message(STATUS "\033[1;32m${message}\033[0m")
    elseif(${color} STREQUAL "YELLOW")
        message(STATUS "\033[1;33m${message}\033[0m")
    elseif(${color} STREQUAL "BLUE")
        message(STATUS "\033[1;34m${message}\033[0m")
    elseif(${color} STREQUAL "MAGENTA")
        message(STATUS "\033[1;35m${message}\033[0m")
    elseif(${color} STREQUAL "CYAN")
        message(STATUS "\033[1;36m${message}\033[0m")
    else()
        message(STATUS "${message}")
    endif()
endfunction()

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

# 打印编译信息的函数
function(print_build_info target)
    color_message(CYAN "=== Build Information for ${target} ===")
    get_target_property(TARGET_TYPE ${target} TYPE)
    color_message(BLUE "Target Type: ${TARGET_TYPE}")
    
    get_target_property(TARGET_SOURCES ${target} SOURCES)
    color_message(BLUE "Source Files:")
    foreach(src ${TARGET_SOURCES})
        color_message(YELLOW "  - ${src}")
    endforeach()
    
    get_target_property(TARGET_COMPILE_OPTIONS ${target} COMPILE_OPTIONS)
    if(TARGET_COMPILE_OPTIONS)
        color_message(BLUE "Compile Options:")
        foreach(opt ${TARGET_COMPILE_OPTIONS})
            color_message(YELLOW "  - ${opt}")
        endforeach()
    endif()
    
    get_target_property(TARGET_LINK_OPTIONS ${target} LINK_OPTIONS)
    if(TARGET_LINK_OPTIONS)
        color_message(BLUE "Link Options:")
        foreach(opt ${TARGET_LINK_OPTIONS})
            color_message(YELLOW "  - ${opt}")
        endforeach()
    endif()
    
    color_message(GREEN "=== End Build Information ===")
endfunction()