# Sub-library: fastGaussianBlur

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
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/glm
)
set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ${LIB_NAME_LOWER}
)

list(LENGTH LIB_SOURCES LIB_SOURCE_COUNT)
message(STATUS "  Built library: ${TARGET_NAME} (${LIB_SOURCE_COUNT} sources)")
