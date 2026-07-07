# Sub-library: keyboard

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")
set(APP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

if(NOT LIB_SOURCES)
    message(WARNING "No source files found for library: ${LIB_NAME}")
    return()
endif()

add_library(${TARGET_NAME} STATIC ${LIB_SOURCES})
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

CreatePAK(
    ${TARGET_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_keyboard.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/R.h
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib_private_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/lib_private_config.h
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    PRODUCT_${CDROID_CHIPSET_UPPER}
)
target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/childs
        ${CDROID_INCLUDE_DIRS}
        ${CDROID_DEPINCLUDES}
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/include/gui
        ${CMAKE_BINARY_DIR}/include/porting
        ${APP_ROOT_DIR}/src
        ${APP_ROOT_DIR}/config
        ${APP_ROOT_DIR}/src/utils
        ${APP_ROOT_DIR}/src/class
)
set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ${LIB_NAME_LOWER}
)

list(LENGTH LIB_SOURCES LIB_SOURCE_COUNT)
message(STATUS "  Built library: ${TARGET_NAME} (${LIB_SOURCE_COUNT} sources)")
