message(STATUS "========================================")
message(STATUS "Running features.cmake")
message(STATUS "========================================")


################################## 开关 ##################################

include(${CMAKE_CURRENT_LIST_DIR}/option_helpers.cmake)

if(CDROID_CHIPSET STREQUAL "x64")
    set(DEFAULT_ENABLED_WIFI ON)
else()
    set(DEFAULT_ENABLED_WIFI OFF)
endif()
project_option(ENABLED_WIFI ${DEFAULT_ENABLED_WIFI} "启用 Wi-Fi 支持[将附带CURL支持]")
project_option(ENABLED_OPENSSL         OFF          "启用 CURL 的显式 OpenSSL 链接")

project_option(ENABLED_KEYBOARD        ON   "启用键盘支持")
project_option(ENABLED_KEYBOARD_PINYIN OFF  "启用键盘拼音输入支持")
project_option(ENABLED_GAUSS_VIEW      OFF  "启用高斯模糊视图")
project_option(ENABLED_GAUSS_DRAWABLE  ON   "启用高斯模糊绘图")
project_option(ENABLED_FILE_SYSTEM     ON   "启用文件系统支持")
project_option(ENABLED_JSON            ON   "启用 JsonCpp 支持")
project_option(ENABLED_PIXMAN          ON   "启用 Pixman 支持")
project_option(ENABLED_VIDEO           ON   "启用视频支持")
project_option(ENABLED_AUDIO           OFF  "启用 ALSA 音频播放")


################################## 配置 ##################################

# CDROID 基础依赖（常驻配置）
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

# Wi-Fi 配置（ENABLED_WIFI）
if (ENABLED_WIFI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_WIFI=1)
    set(ENABLED_CURL ON)
else()
    set(ENABLED_CURL OFF)
endif()

# CURL 配置（由 ENABLED_WIFI 派生）
if (ENABLED_CURL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_CURL=1)
    find_package(CURL CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES CURL::libcurl)
endif()

# OpenSSL 配置（ENABLED_OPENSSL，仅在 CURL 启用时生效）
if (ENABLED_CURL AND ENABLED_OPENSSL)
    find_package(OpenSSL REQUIRED)
    list(APPEND PROJECT_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

# 键盘配置（ENABLED_KEYBOARD / ENABLED_KEYBOARD_PINYIN）
if (ENABLED_KEYBOARD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_KEYBOARD=1)
    if(ENABLED_KEYBOARD_PINYIN)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_KEYBOARD_PINYIN=1)
    endif()
endif()

# 高斯模糊视图配置（ENABLED_GAUSS_VIEW）
if (ENABLED_GAUSS_VIEW)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GAUSS_VIEW)
endif()

# 高斯模糊绘图配置（ENABLED_GAUSS_DRAWABLE）
if (ENABLED_GAUSS_DRAWABLE)
    if(NOT ENABLED_PIXMAN)
        message(FATAL_ERROR "ENABLED_PIXMAN must be enabled when ENABLED_GAUSS_DRAWABLE is enabled")
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GAUSS_DRAWABLE)
endif()

# 文件系统配置（ENABLED_FILE_SYSTEM）
if (ENABLED_FILE_SYSTEM)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_FILE_SYSTEM)
    find_package(ghc_filesystem CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES ghcFilesystem::ghc_filesystem)
endif()

# JSON 配置（ENABLED_JSON）
if (ENABLED_JSON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_JSON=1)
    find_package(jsoncpp CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES JsonCpp::JsonCpp)
endif()

# Pixman 配置（ENABLED_PIXMAN）
if (ENABLED_PIXMAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_PIXMAN=1)
    find_package(Pixman REQUIRED)
    list(APPEND PROJECT_INCLUDE_DIR ${PIXMAN_INCLUDE_DIR})
    list(APPEND PROJECT_LIBRARIES ${PIXMAN_LIBRARIES})
endif()

# 视频配置（ENABLED_VIDEO）
if (ENABLED_VIDEO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_VIDEO=1 ENABLE_RGB_VIDEO=1)
    if(CDROID_CHIPSET STREQUAL x64)
        find_package(FFMPEG REQUIRED)
        find_library(SWSCALE_LIBRARIES swscale HINTS ${FFMPEG_LIBRARY_DIRS})
        find_library(SWRESAMPLE_LIBRARIES swresample HINTS ${FFMPEG_LIBRARY_DIRS})
        if(NOT SWSCALE_LIBRARIES OR NOT SWRESAMPLE_LIBRARIES)
            message(FATAL_ERROR "FFmpeg swscale and swresample libraries are required")
        endif()
        list(APPEND PROJECT_INCLUDE_DIR ${FFMPEG_INCLUDE_DIRS})
        list(APPEND PROJECT_LIBRARY_DIR ${FFMPEG_LIBRARY_DIRS})
        list(APPEND PROJECT_LIBRARIES ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES} ${SWRESAMPLE_LIBRARIES})
    endif()
endif()

# 音频配置（ENABLED_AUDIO）
if (ENABLED_AUDIO)
    find_package(ALSA REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_AUDIO=1)
    list(APPEND PROJECT_INCLUDE_DIR ${ALSA_INCLUDE_DIRS})
    list(APPEND PROJECT_LIBRARIES ${ALSA_LIBRARIES})
endif()
