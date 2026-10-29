message(STATUS "========================================")
message(STATUS "Running features.cmake")
message(STATUS "========================================")


################################## 开关 ##################################

if(CDROID_CHIPSET STREQUAL "x64")
    set(ENABLED_WIFI ON)              # Wi-Fi 支持（同时启用 CURL）
else()
    set(ENABLED_WIFI OFF)
endif()
set(ENABLED_OPENSSL         OFF)       # CURL 显式 OpenSSL 链接

set(ENABLED_KEYBOARD        ON)        # 键盘支持
set(ENABLED_KEYBOARD_PINYIN ON)        # 键盘拼音输入支持
set(ENABLED_GAUSS_VIEW      OFF)       # 高斯模糊视图
set(ENABLED_GAUSS_DRAWABLE  ON)        # 高斯模糊绘图
set(ENABLED_FILE_SYSTEM     ON)        # 文件系统支持（关闭后 FileUtils 全部接口失效）
set(ENABLED_JSON            ON)        # JsonCpp 支持（当前必须为 ON，见下方保护）
set(ENABLED_PIXMAN          ON)        # Pixman 支持
set(ENABLED_VIDEO           ON)        # 视频支持
set(ENABLED_ALSA            OFF)       # ALSA 音频播放


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
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GAUSS_VIEW=1)
endif()

# 高斯模糊绘图配置（ENABLED_GAUSS_DRAWABLE）
if (ENABLED_GAUSS_DRAWABLE)
    if(NOT ENABLED_PIXMAN)
        message(FATAL_ERROR "ENABLED_PIXMAN must be enabled when ENABLED_GAUSS_DRAWABLE is enabled")
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GAUSS_DRAWABLE=1)
endif()

# 文件系统配置（ENABLED_FILE_SYSTEM）
# 关闭后 FileUtils 全部接口转为失效实现（返回 false / 空操作），不再依赖 ghc_filesystem
if (ENABLED_FILE_SYSTEM)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_FILE_SYSTEM=1)
    find_package(ghc_filesystem CONFIG REQUIRED)
    list(APPEND PROJECT_LIBRARIES ghcFilesystem::ghc_filesystem)
endif()

# JSON 配置（ENABLED_JSON）
# JsonCpp 为常驻依赖：json_utils.h / history_store.h / statistics_store.h 等无条件引用，
# 关闭该开关必然导致编译失败，因此此处直接拦截，避免误改配置后出现难懂的报错。
if (NOT ENABLED_JSON)
    message(FATAL_ERROR "ENABLED_JSON must be enabled: JsonCpp is referenced unconditionally by json_utils/history_store/statistics_store")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_JSON=1)
find_package(jsoncpp CONFIG REQUIRED)
list(APPEND PROJECT_LIBRARIES JsonCpp::JsonCpp)

# Pixman 配置（ENABLED_PIXMAN）
if (ENABLED_PIXMAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_PIXMAN=1)
    find_package(Pixman REQUIRED)
    list(APPEND PROJECT_INCLUDE_DIR ${PIXMAN_INCLUDE_DIR})
    list(APPEND PROJECT_LIBRARIES ${PIXMAN_LIBRARIES})
endif()

# 视频配置（ENABLED_VIDEO）
if (ENABLED_VIDEO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_VIDEO=1)
endif()

# 音频配置（ENABLED_ALSA）
if (ENABLED_ALSA)
    find_package(ALSA REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_ALSA=1)
    list(APPEND PROJECT_INCLUDE_DIR ${ALSA_INCLUDE_DIRS})
    list(APPEND PROJECT_LIBRARIES ${ALSA_LIBRARIES})
endif()
