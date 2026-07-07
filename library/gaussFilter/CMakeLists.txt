# Sub-library: gaussFilter

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TOLOWER "${LIB_NAME}" LIB_NAME_LOWER)
set(TARGET_NAME "${PROJECT_NAME}_${LIB_NAME_LOWER}_lib")

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)

if(NOT LIB_SOURCES)
    message(WARNING "No source files found for library: ${LIB_NAME}")
    return()
endif()

add_library(${TARGET_NAME} STATIC ${LIB_SOURCES})
set(${LIB_NAME}_TARGET_NAME ${TARGET_NAME} PARENT_SCOPE)

target_compile_definitions(${TARGET_NAME} PRIVATE
    PRODUCT_${CDROID_CHIPSET_UPPER}
)
target_include_directories(${TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_BINARY_DIR}/include/porting
)

if(CDROID_CHIPSET STREQUAL "x64")
    target_compile_options(${TARGET_NAME} PRIVATE -mavx -msse)
else()
    find_package(OpenMP QUIET)
    if(OpenMP_C_FOUND)
        target_compile_definitions(${TARGET_NAME} PRIVATE ENABLE_OMP=1)
        target_link_libraries(${TARGET_NAME} PRIVATE OpenMP::OpenMP_C)
    endif()
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ${LIB_NAME_LOWER}
)

list(LENGTH LIB_SOURCES LIB_SOURCE_COUNT)
message(STATUS "  Built library: ${TARGET_NAME} (${LIB_SOURCE_COUNT} sources)")
