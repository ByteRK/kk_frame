# ============================================================================
# src/utils/tests —— utils 独立测试工程
#
# 本目录不属于主程序：最外层 CMakeLists 已把 /src/utils/tests/ 从源文件列表中排除。
# 单独构建方式（在仓库根目录执行）：
#
#   cmake -S src/utils/tests -B build-utils-tests
#   cmake --build build-utils-tests -j
#   ./build-utils-tests/kk_utils_tests
#
# 依赖：只依赖系统 C++ 编译器与 POSIX（iconv 为 glibc 自带），
#       不依赖 cdroid / vcpkg / ghc_filesystem。
# ============================================================================

cmake_minimum_required(VERSION 3.13)
project(kk_utils_tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(UTILS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PROJECT_DIR ${UTILS_DIR}/../..)

# ---------------------------------------------------------------------------
# 被测源码：直接引用 utils 各子目录，避免复制代码
# 说明：integration 层只有 hash_utils 不依赖 jsoncpp / 图像算法库，单独纳入测试；
#       其余 integration 源（json/image/brand/cdroid）需要三方库，不纳入本测试工程。
# ---------------------------------------------------------------------------
file(GLOB UTILS_BASE_SOURCES     CONFIGURE_DEPENDS "${UTILS_DIR}/base/*.cc")
file(GLOB UTILS_PLATFORM_SOURCES CONFIGURE_DEPENDS "${UTILS_DIR}/platform/*.cc")

set(UTILS_INTEGRATION_SOURCES
    ${UTILS_DIR}/integration/hash_utils.cc
)

# 测试自身源码
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
    ${UTILS_BASE_SOURCES}
    ${UTILS_PLATFORM_SOURCES}
    ${UTILS_INTEGRATION_SOURCES}
)

# ---------------------------------------------------------------------------
# 日志兼容层：顶替 cdroid 的 cdlog.h
#
# utils 源码里写的是 #include <cdlog.h>，测试时没有 cdroid。
# 这里在“构建目录”生成一个只做转发的 cdlog.h，把包含链指到源码树里的
# tests/cdlog_shim.h；源码树中不再存在名为 cdlog.h 的文件，
# 编辑器（IntelliSense）因此可以正常解析到框架的 cdlog.h。
# ---------------------------------------------------------------------------
set(CDLOG_SHIM_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${CDLOG_SHIM_INCLUDE_DIR})
file(GENERATE OUTPUT ${CDLOG_SHIM_INCLUDE_DIR}/cdlog.h CONTENT [=[
/* 由 src/utils/tests/CMakeLists.txt 自动生成，请勿手动修改。
 * 作用：让 utils 源码中的 #include <cdlog.h> 命中测试兼容层。 */
#include "cdlog_shim.h"
]=])

# ---------------------------------------------------------------------------
# 头文件路径
# 关键：上面生成的目录放在最前面，使 utils 源码中的 #include <cdlog.h>
#       命中兼容层，从而无需引入 cdroid。
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CDLOG_SHIM_INCLUDE_DIR}     # 必须在前：生成的 cdlog.h 转发头
    ${CMAKE_CURRENT_SOURCE_DIR}   # cdlog_shim.h / log_utils.h 兼容层
    ${UTILS_DIR}/base
    ${UTILS_DIR}/platform
    ${UTILS_DIR}/integration
    ${PROJECT_DIR}               # "library/library_config.h"
    ${PROJECT_DIR}/config
    ${PROJECT_DIR}/library       # macro_define.h / library_config.h
    ${PROJECT_DIR}/src/template
)

# ---------------------------------------------------------------------------
# 宏定义
# ENABLE_FILE_SYSTEM=0：让 FileUtils 走"失效实现"，测试不必引入 ghc_filesystem。
# PRODUCT_X64：让 process_utils 走 x64 分支（不真的 sync/reboot）。
# ---------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PRODUCT_X64
    ENABLE_FILE_SYSTEM=0
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

message(STATUS "Utils tests: base(${UTILS_BASE_SOURCES})")
message(STATUS "Utils tests: platform(${UTILS_PLATFORM_SOURCES})")
