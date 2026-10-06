# Sub-library: video

# ========================================
# 视频播放库
#   src/        —— 公共接口与公共逻辑（后端无关）
#   hal/<体系>  —— 各架构底层实现，按 CDROID_CHIPSET 选择性编译
#   hal/none    —— ENABLED_VIDEO 关闭时的占位实现
#   hal/generic —— 无专用实现的芯片走 cdroid 的 MP* 接口兜底
# ========================================

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")

# 1. 选择后端
if(NOT ENABLED_VIDEO)
    set(VIDEO_BACKEND "none")
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hal/${CDROID_CHIPSET})
    set(VIDEO_BACKEND "${CDROID_CHIPSET}")
else()
    set(VIDEO_BACKEND "generic")
endif()

# 2. 收集源码（公共逻辑 + 选定后端）
file(GLOB_RECURSE VIDEO_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
file(GLOB VIDEO_BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/${VIDEO_BACKEND}/*.cc"
)

if(NOT VIDEO_BACKEND_SOURCES)
    message(FATAL_ERROR "Video backend sources not found: hal/${VIDEO_BACKEND}")
endif()

add_library(${TARGET_NAME} STATIC ${VIDEO_COMMON_SOURCES} ${VIDEO_BACKEND_SOURCES})
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

target_compile_definitions(${TARGET_NAME} PRIVATE
    PRODUCT_${CDROID_CHIPSET_UPPER}
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/include/porting
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/include/gui
        ${CDROID_INCLUDE_DIRS}
        ${CDROID_DEPINCLUDES}
)

# 3. 后端专属依赖
if(VIDEO_BACKEND STREQUAL "x64")
    # FFmpeg 软解
    find_package(FFMPEG REQUIRED)
    find_library(SWSCALE_LIBRARIES swscale HINTS ${PC_FFMPEG_LIBDIR} ${PC_FFMPEG_LIBRARY_DIRS})
    if(NOT SWSCALE_LIBRARIES)
        message(FATAL_ERROR "FFmpeg swscale library is required for the x64 video backend")
    endif()
    find_package(Threads REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${FFMPEG_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        Threads::Threads
    )
elseif(VIDEO_BACKEND STREQUAL "rk3506")
    # Rockchip RKADK（交叉编译时由工具链 sysroot 提供）
    find_library(RKADK_LIBRARIES rkadk REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${RKADK_LIBRARIES}
        Threads::Threads
    )
elseif(VIDEO_BACKEND STREQUAL "generic")
    # 走 cdroid 的 MP* 接口，符号由 libtvhal.so 在运行期提供
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE
        Threads::Threads
    )
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ${LIB_NAME_LOWER}
)

list(LENGTH VIDEO_BACKEND_SOURCES VIDEO_BACKEND_SOURCE_COUNT)
message(STATUS "  Built library: ${TARGET_NAME} (backend: ${VIDEO_BACKEND}, ${VIDEO_BACKEND_SOURCE_COUNT} backend sources)")
