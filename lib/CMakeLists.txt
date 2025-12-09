# 收集 HNUYueLuRM 下所有的源文件 (递归)
file(GLOB_RECURSE LIB_SOURCES "HNUYueLuRM/*.c" "HNUYueLuRM/*.cpp")

# 创建静态库
add_library(HNUYueLuRM_Framework STATIC ${LIB_SOURCES})

# 递归包含头文件路径 (PUBLIC 确保链接这个库的目标也能看到这些头文件)
target_include_directories_recursively(HNUYueLuRM_Framework ${CMAKE_CURRENT_SOURCE_DIR}/HNUYueLuRM)

# 还需要包含根目录的 Inc，因为官方驱动可能需要 stm32f4xx_hal_conf.h
target_include_directories(HNUYueLuRM_Framework PUBLIC ${CMAKE_SOURCE_DIR}/Inc)

# 添加编译选项以确保 FPU 设置一致
target_compile_options(HNUYueLuRM_Framework PRIVATE ${MCU_FLAGS})

message(STATUS "Library HNUYueLuRM_Framework Configured.")