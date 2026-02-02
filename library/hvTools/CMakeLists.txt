# ========================================
# 子库模板：<hvTools>
# ========================================

# 1. 设置库信息
set(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(LIB_NAME ${LIB_NAME} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)

message(STATUS "Configuring library: ${LIB_NAME}")

# 2. 库目标名
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")

# 3. 源文件收集
file(GLOB_RECURSE LIB_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)

# 4. 导出头文件目录到父作用域
set(${LIB_NAME}_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    PARENT_SCOPE
)

# 5. 导出目标名到父作用域
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

# 6. 创建库
if(LIB_SOURCES)
    # 静态库
    add_library(${TARGET_NAME} STATIC ${LIB_SOURCES})

    # PRODUCT 定义
    target_compile_definitions(${TARGET_NAME} PRIVATE
        PRODUCT_${CDROID_CHIPSET_UPPER}
    )
    
    # 设置头文件目录
    target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
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