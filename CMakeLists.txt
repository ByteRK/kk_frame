# cmake最低版本以及项目名设置
cmake_minimum_required(VERSION 3.8)
project(kk_frame)
message("PROJECT_NAME=${PROJECT_NAME}")

# 目标属性设置
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# 功能开关
option(ENABLED_KEYBOARD "Enable keyboard support" OFF)
option(ENABLED_KEYBOARD_PINYIN "Enable pinyin input" OFF)
option(ENABLED_WIFI "Enable WiFi support" ON)
option(ENABLED_OPENSSL "Enable OpenSSL" OFF)
option(ENABLED_JSON "Enable JSON support" ON)
option(ENABLED_VIDEO "Enable video playback" ON)
option(ENABLED_PIXMAN "Enable Pixman" OFF)
option(ENABLED_GLASS_VIEW "Enable glass view" ON)
option(ENABLED_GLASS_DRAWABLE "Enable glass drawable" OFF)



# 源文件收集 [./*.c和./*.cc]会意外导致匹配到非src目录下的源文件
file(GLOB_RECURSE PROJECT_SOURCES
    "./main.cc"
    "./src/*/*.c"
    "./src/*/*.cc"
    "./src/*/*.cpp"
)

# 设置项目特定的库输出目录
set(PROJECT_LIBRARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/apps/${PROJECT_NAME}/lib")
message(STATUS "Project library output directory: ${PROJECT_LIBRARY_OUTPUT_DIR}")

# 创建目录
file(MAKE_DIRECTORY ${PROJECT_LIBRARY_OUTPUT_DIR})

# 可执行文件配置
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 设置编译前置依赖脚本，使每次构建前运行 prebuild.sh
add_custom_target(${PROJECT_NAME}_prebuild ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running pre-build tasks..."
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/script/prebuild.sh
    COMMAND ${CMAKE_COMMAND} -E echo "End pre-build tasks..."
    VERBATIM
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_prebuild)

# 公共编译定义
string(TOUPPER ${CDROID_CHIPSET} CDROID_CHIPSET_UPPER)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CDROID_RUNNING
    PRODUCT_${CDROID_CHIPSET_UPPER}
    $<$<STREQUAL:${CDROID_CHIPSET},x64>:CDROID_X64>
    $<$<STREQUAL:${CDROID_CHIPSET},sigma>:CDROID_SIGMA>
    $<$<STREQUAL:${CDROID_CHIPSET},tinat113>:CDROID_T113>

    KEYBOARD_DISABLE # 禁用键盘
)

# 添加库目录
add_subdirectory(library)

# 键盘 && 拼音 #
if (ENABLED_KEYBOARD OR CDROID_CHIPSET STREQUAL x64)
    add_definitions(-DENABLE_KEYBOARD=1)
    
    if(ENABLED_KEYBOARD_PINYIN)
        add_definitions(-DENABLE_KEYBOARD_PINYIN=1)
    endif()
endif()

# FILE SYSTEM #
find_package(ghc_filesystem CONFIG REQUIRED)
list(APPEND UTILS_LIBS ghcFilesystem::ghc_filesystem)

# WIFI && OPENSSL #
if (ENABLED_WIFI)
    add_definitions(-DENABLE_WIFI=1)
    find_package(CURL CONFIG REQUIRED)
    list(APPEND UTILS_LIBS CURL::libcurl)

    if (ENABLED_OPENSSL)
        find_package(OpenSSL REQUIRED)
        list(APPEND UTILS_LIBS OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

# Json #
if (ENABLED_JSON)
  add_definitions(-DENABLE_JSON=1)
  find_package(jsoncpp CONFIG REQUIRED)
  list(APPEND UTILS_LIBS JsonCpp::JsonCpp)
endif()

# 视频播放 #
if (ENABLED_VIDEO)
    add_definitions(-DENABLE_VIDEO=1 -DENABLE_RGB_VIDEO=1)
endif()
if(ENABLED_VIDEO AND CDROID_CHIPSET STREQUAL x64)
    find_package(FFMPEG REQUIRED)
    find_library(SWSCALE_LIBRARIES swscale HINTS ${FFMPEG_LIBRARY_DIRS})
    find_library(SWRESAMPLE_LIBRARIES swresample HINTS ${FFMPEG_LIBRARY_DIRS})
    list(APPEND UTILS_INCLUDE_DIR ${FFMPEG_INCLUDE_DIRS})
    list(APPEND UTILS_LIBS_DIR ${FFMPEG_LIBRARY_DIRS})
    list(APPEND UTILS_LIBS ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES} ${SWRESAMPLE_LIBRARIES})
endif()

# Pixman #
if (ENABLED_PIXMAN)
    add_definitions(-DENABLED_PIXMAN=1)
    find_package(Pixman)
    list(APPEND UTILS_LIBS ${PIXMAN_LIBRARIES})
endif()

# 模糊控件 #
if (ENABLE_GLASS_VIEW)
    add_definitions(-DENABLE_GLASS_VIEW)
endif()

# 模糊Drawable #
if (ENABLE_GLASS_DRAWABLE)
    add_definitions(-DENABLE_GLASS_DRAWABLE)
endif()

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

    # Cdroid头
    ${CDROID_INCLUDE_DIRS}
    ${CDROID_DEPINCLUDES}

    # CMAKE头
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}/include/gui
    ${CMAKE_BINARY_DIR}/include/porting

    # 工具库
    ${UTILS_INCLUDE_DIR}

    # 项目库头文件
    ${PROJECT_LIBRARY_INCLUDE}
)

# 库搜索路径配置
target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}/lib
    ${UTILS_LIBS_DIR}
    ${PROJECT_LIBRARY_OUTPUT_DIR}
)

# 链接库配置
target_link_libraries(${PROJECT_NAME} PRIVATE
    cdroid
    ${UTILS_LIBS}
    ${PROJECT_LIBRARIES}
)

# PAK打包
CreatePAK(
    ${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pak
    ${PROJECT_SOURCE_DIR}/R.h
)

# 安装配置 进行编译
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 打印配置信息
message(STATUS "========================================")
message(STATUS "Configuration Summary")
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Libraries enabled: ${PROJECT_LIBRARIES}")
message(STATUS "Library count: ${PROJECT_LIBRARIES}")
if(PROJECT_LIBRARIES)
    list(LENGTH PROJECT_LIBRARIES lib_count)
    message(STATUS "Total libraries to link: ${lib_count}")
else()
    message(STATUS "Total libraries to link: 0")
endif()
message(STATUS "========================================")