# ========================================
# 子库模板：<ntpClient>
# ========================================

# 设置库信息
set(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(LIB_NAME ${LIB_NAME} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)

message(STATUS "Configuring library: ${LIB_NAME}")

# 库目标名
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")

# 源文件收集
file(GLOB_RECURSE LIB_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 导出头文件目录到父作用域
set(${LIB_NAME}_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    PARENT_SCOPE
)

# 导出目标名到父作用域
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

# 创建库（子库编译设定）
if(LIB_SOURCES)
    # 静态库
    add_library(${TARGET_NAME} STATIC ${LIB_SOURCES})
    
    # 设置头文件目录
    target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # 设置PIC以及输出目录
    set_target_properties(${TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME ${LIB_NAME_LOWER}
    )
    
    message(STATUS "  Built library: ${TARGET_NAME}")
    message(STATUS "  Sources count: ${LIB_SOURCES}")
else()
    message(WARNING "No source files found for library: ${LIB_NAME}")
endif()
