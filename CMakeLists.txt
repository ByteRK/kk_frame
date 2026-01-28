cmake_minimum_required(VERSION 3.8)
project(kk_frame)

message(STATUS "\n\n\n\n-- ========================================")
message(STATUS "Configuring project: ${PROJECT_NAME}")
message(STATUS "========================================")

# C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源文件收集
file(GLOB_RECURSE PROJECT_SOURCES
    "./main.cc" "./src/*/*.c" "./src/*/*.cc" "./src/*/*.cpp"
)

# Bin配置
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 项目环境定义
string(TOUPPER ${CDROID_CHIPSET} CDROID_CHIPSET_UPPER)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PRODUCT_${CDROID_CHIPSET_UPPER}
    $<$<STREQUAL:${CDROID_CHIPSET},x64>:CDROID_X64>
)

# 项目变量定义
set(PROJECT_LIBRARIES "")    # 项目库列表
set(PROJECT_LIBRARY_DIR "")  # 项目库搜索路径列表
set(PROJECT_INCLUDE_DIR "")  # 项目头文件目录列表

# 项目配置
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project.cmake)

# 项目库配置
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library)
list(APPEND PROJECT_LIBRARIES ${LIB_LIBRARIES})
list(APPEND PROJECT_LIBRARY_DIR ${LIB_LIBRARY_DIR})
list(APPEND PROJECT_INCLUDE_DIR ${LIB_INCLUDE_DIR})

# 其它配置
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/other.cmake)

# 头文件路径配置
target_include_directories(${PROJECT_NAME} PRIVATE
    # 配置头
    .
    src
    config

    # 项目头
    src/app
    src/app/data
    src/app/managers
    src/app/protocol
    src/app/page
    src/app/page/core
    src/app/page/view
    src/app/page/components

    # 通讯
    src/comm/base
    src/comm/i2c
    src/comm/net
    src/comm/uart

    # 组件及模板工具
    src/class
    src/template
    src/utils
    src/widgets

    # 项目库头文件
    ${PROJECT_INCLUDE_DIR}
)

# 库搜索路径配置
target_link_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_LIBRARY_DIR}
)

# 链接库配置
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_LIBRARIES}
)

# PAK打包
CreatePAK(
    ${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pak
    ${PROJECT_SOURCE_DIR}/R.h
)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 打印配置信息
message(STATUS "========================================")
message(STATUS "Configuration Summary")
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Library list: ${PROJECT_LIBRARIES}")
if(PROJECT_LIBRARIES)
    list(LENGTH PROJECT_LIBRARIES lib_count)
    message(STATUS "Total libraries to link: ${lib_count}")
else()
    message(STATUS "Total libraries to link: 0")
endif()
message(STATUS "Library dir: ${PROJECT_LIBRARY_DIR}")
message(STATUS "Include dir: ${PROJECT_INCLUDE_DIR}")
message(STATUS "========================================\n\n\n\n-- ")