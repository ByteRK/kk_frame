message(STATUS "========================================")
message(STATUS "Running Project.cmake")
message(STATUS "========================================")


################################## 开关 ##################################

set(ENABLED_KEYBOARD OFF)         # 键盘
set(ENABLED_KEYBOARD_PINYIN ON)   # 拼音

set(ENABLED_GAUSS_VIEW OFF)       # 高斯视图
set(ENABLED_GAUSS_DRAWABLE ON)    # 高斯绘图

set(ENABLED_FILE_SYSTEM ON)       # 文件系统
set(ENABLED_WIFI OFF)             # WIFI
set(ENABLED_OPENSSL OFF)          # OpenSSL
set(ENABLED_JSON ON)              # Json
set(ENABLED_PIXMAN ON)            # Pixman
set(ENABLED_VIDEO ON)             # 视频



################################## 配置 ##################################

list(APPEND PROJECT_LIBRARIES
    cdroid
)
list(APPEND PROJECT_INCLUDE_DIR 
    ${CDROID_INCLUDE_DIRS}
    ${CDROID_DEPINCLUDES}
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}/include/gui
    ${CMAKE_BINARY_DIR}/include/porting
)

if (ENABLED_KEYBOARD)
    add_definitions(-DENABLE_KEYBOARD=1)
    if(ENABLED_KEYBOARD_PINYIN)
        add_definitions(-DENABLE_KEYBOARD_PINYIN=1)
    endif()
endif()

if (ENABLED_GAUSS_VIEW)
    add_definitions(-DENABLE_GAUSS_VIEW)
endif()
if (ENABLED_GAUSS_DRAWABLE)
    if(NOT ENABLED_PIXMAN)
        message(FATAL_ERROR "ENABLED_PIXMAN must be enabled when ENABLED_GAUSS_DRAWABLE is enabled")
    endif()
    add_definitions(-DENABLE_GAUSS_DRAWABLE)
endif()

if (ENABLED_FILE_SYSTEM)
    add_definitions(-DENABLE_FILE_SYSTEM)
    find_package(ghc_filesystem CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES ghcFilesystem::ghc_filesystem)
endif()

if (ENABLED_WIFI)
    add_definitions(-DENABLE_WIFI=1)
    find_package(CURL CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES CURL::libcurl)
endif()

if (ENABLED_WIFI AND ENABLED_OPENSSL)
    find_package(OpenSSL REQUIRED)
    list(APPEND PROJECT_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

if (ENABLED_JSON)
    add_definitions(-DENABLE_JSON=1)
    find_package(jsoncpp CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES JsonCpp::JsonCpp)
endif()

if (ENABLED_PIXMAN)
    add_definitions(-DENABLE_PIXMAN=1)
    find_package(Pixman)
    list(APPEND PROJECT_INCLUDE_DIR ${PIXMAN_INCLUDE_DIR})
    list(APPEND PROJECT_LIBRARIES ${PIXMAN_LIBRARIES})
endif()

if (ENABLED_VIDEO)
    add_definitions(-DENABLE_VIDEO=1 -DENABLE_RGB_VIDEO=1)
    if(CDROID_CHIPSET STREQUAL x64)
        find_package(FFMPEG REQUIRED)
        find_library(SWSCALE_LIBRARIES swscale HINTS ${FFMPEG_LIBRARY_DIRS})
        find_library(SWRESAMPLE_LIBRARIES swresample HINTS ${FFMPEG_LIBRARY_DIRS})
        list(APPEND PROJECT_INCLUDE_DIR ${FFMPEG_INCLUDE_DIRS})
        list(APPEND PROJECT_LIBRARY_DIR ${FFMPEG_LIBRARY_DIRS})
        list(APPEND PROJECT_LIBRARIES ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES} ${SWRESAMPLE_LIBRARIES})
    endif()
endif()