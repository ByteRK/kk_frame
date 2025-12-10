# cmake最低版本以及项目名设置
cmake_minimum_required(VERSION 3.8)
project(kk_frame)
message("PROJECT_NAME=${PROJECT_NAME}")

# 生成X64下的字体文件位置
execute_process(COMMAND bash script/fonts.sh  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 目标属性设置
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包查找
find_package(jsoncpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
# find_package(Pixman)

# 源文件收集 [./*.c和./*.cc]会意外导致匹配到非src目录下的源文件
file(GLOB_RECURSE PROJECT_SOURCES
    # "./*.c"
    # "./*.cc"
    "./main.cc"
    "./src/*/*.c"
    "./src/*/*.cc"
)

# 可执行文件配置
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 设置版本依赖，使每次构建前运行 date2ver 生成版本号
add_custom_target(${PROJECT_NAME}_version 
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/date2ver > /dev/null
    COMMENT "Running date2ver script for version generation")
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_version)

# 包含目录配置
target_include_directories(${PROJECT_NAME} PRIVATE
    # 项目头文件
    .
    config
    src/buzzer
    src/common
    src/data
    src/project
    src/function
    src/net
    src/uart
    src/i2c
    src/protocol
    src/viewlibs
    src/windows

    # Cdroid头文件
    ${CDROID_INCLUDE_DIRS}
    ${CDROID_DEPINCLUDES}

    # CMAKE头文件
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}/include/gui
    ${CMAKE_BINARY_DIR}/include/porting

    # PIXMAN头文件
    # ${PIXMAN_INCLUDE_DIRS}
)

# 公共编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CDROID_RUNNING
    $<$<STREQUAL:${CDROID_CHIPSET},x64>:CDROID_X64>
    $<$<STREQUAL:${CDROID_CHIPSET},sigma>:CDROID_SIGMA>
    $<$<STREQUAL:${CDROID_CHIPSET},tinat113>:CDROID_T113>
    $<$<STREQUAL:${CDROID_CHIPSET},rk3506>:CDROID_RK3506>

    KEYBOARD_DISABLE # 禁用键盘
)

# 公共链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    cdroid
    JsonCpp::JsonCpp
    CURL::libcurl # ${CURL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto # ${OPENSSL_LIBRARIES}
    # ${PIXMAN_LIBRARIES}
)

# 平台特定配置 (以下例子为为x64增加视频支持)
if(CDROID_CHIPSET STREQUAL "x64")

    # 增加视频支持
    find_package(FFMPEG REQUIRED)
    find_library(AVSWSCALE_LIBRARIES
        NAMES swscale
        HINTS ${PC_FFMPEG_LIBDIR}
        ${PC_FFMPEG_LIBRARY_DIRS}
    )
    find_library(AVSWRESAMPLE_LIBRARIES
        NAMES swresample
        HINTS ${PC_FFMPEG_LIBDIR}
        ${PC_FFMPEG_LIBRARY_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFMPEG_LIBRARIES} ${AVSWSCALE_LIBRARIES} ${AVSWRESAMPLE_LIBRARIES}
    )
elseif(CDROID_CHIPSET STREQUAL "sigma")
elseif(CDROID_CHIPSET STREQUAL "t113")
elseif(CDROID_CHIPSET STREQUAL "rk3506")
else()
    message("Unsupported chipset: ${CDROID_CHIPSET}")
endif()


# PAK打包
CreatePAK(
    ${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pak
    ${PROJECT_SOURCE_DIR}/R.h
)

# 安装配置 进行编译
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

