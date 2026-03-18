# ========================================
# 子库模板：<library_keyboard>
# ========================================

# 1. 设置库信息
set(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(LIB_NAME ${LIB_NAME} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)

message(STATUS "Configuring library: ${LIB_NAME}")

# 2. 推导项目根路径（若需要）
set(APP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# 3. 库目标名
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")

# 4. 源文件收集
file(GLOB_RECURSE LIB_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/childs/*.cc"
)

# 5. 导出头文件目录到父作用域
set(${LIB_NAME}_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PARENT_SCOPE
)

# 6. 导出依赖库到父作用域（非必选）
#set(${LIB_NAME}_LIBRARIES
#    PARENT_SCOPE
#)

# 7. 导出依赖库搜索目录到父作用域（非必选）
#set(${LIB_NAME}_LIBRARY_DIRS
#    PARENT_SCOPE
#)

# 8. 导出目标名到父作用域
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

# 9. 创建库（子库编译设定）
if(LIB_SOURCES)
    # CDROID头文件目录
    set(LIB_CDROID_INCLUDE_PATHS
        ${CDROID_INCLUDE_DIRS}
        ${CDROID_DEPINCLUDES}
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/include/gui
        ${CMAKE_BINARY_DIR}/include/porting
    )

    # 静态库
    add_library(${TARGET_NAME} STATIC ${LIB_SOURCES})

    # Pak 打包    
    CreatePAK(
        ${TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${PROJECT_BINARY_DIR}/${LIB_NAME_LOWER}.pak
        ${CMAKE_CURRENT_SOURCE_DIR}/R.h
    )

    # 生成库私有头
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_private_config.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_private_config.h
    )

    # PRODUCT 定义
    target_compile_definitions(${TARGET_NAME} PRIVATE
        PRODUCT_${CDROID_CHIPSET_UPPER}
    )
    
    # 设置头文件目录
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/childs

        ${LIB_CDROID_INCLUDE_PATHS}   # 为了获取CDROID组件
        
        ${APP_ROOT_DIR}/config        # 为了获取文件路径LOCAL_DATA_DIR
        ${APP_ROOT_DIR}/src/utils     # 为了获取utils
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
