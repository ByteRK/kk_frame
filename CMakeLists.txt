# cmake最低版本以及项目名设置
cmake_minimum_required(VERSION 3.8)
project(kk_frame)
message("PROJECT_NAME=${PROJECT_NAME}")

# 版本生成命令
execute_process(COMMAND bash date2ver WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

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

# 芯片特定配置 (以下例子为为sigma添加海信IOT库)
# if(CDROID_CHIPSET STREQUAL sigma)
#     target_include_directories(${PROJECT_NAME} PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hisense_iot/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hisense_iot/include/hal
#     )
#     target_link_libraries(${PROJECT_NAME} PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hisense_iot/libsat_kit_ALL.a   # 海信IOT库
#         ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hisense_iot/libhs.so           # 海信IOT库必联适配层
#         rt                                                                  # 海信IOT库引用的基本库
#     )
#     target_compile_definitions(${PROJECT_NAME} PRIVATE HISENSE_IOT_ENABLE)
# else()
#     target_compile_definitions(${PROJECT_NAME} PRIVATE HISENSE_IOT_DISABLE)
# endif()

# Gauss相关的库
find_package(Pixman)
if(CDROID_CHIPSET STREQUAL sigma)
    find_package(OpenMP REQUIRED)
    if(OpenMP_C_FOUND)
        add_definitions(-DENABLE_OMP=1)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_C)
    endif()
else()
    include_directories(./3rdpart/ARM_NEON_2_x86_SSE)
    set(AVX_SSE -mavx -msse)
endif()

# 增加 3rdpart 的CMAKE 
add_subdirectory(./3rdpart)

# PAK打包
CreatePAK(
    ${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pak
    ${PROJECT_SOURCE_DIR}/R.h
)

# 安装配置 进行编译
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

