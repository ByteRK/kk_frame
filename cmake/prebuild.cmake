message(STATUS "========================================")
message(STATUS "Running prebuild.cmake")
message(STATUS "========================================")

# 仅在脚本或输入资源变化时重新执行预构建，避免无条件触发重编译。
file(GLOB PREBUILD_FONT_INPUTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf
)
set(PREBUILD_STAMP ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_prebuild.stamp)
add_custom_command(
    OUTPUT ${PREBUILD_STAMP}
    COMMAND ${CMAKE_COMMAND} -E echo "Running pre-build tasks..."
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/script/prebuild.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${PREBUILD_STAMP}
    COMMAND ${CMAKE_COMMAND} -E echo "End pre-build tasks..."
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/script/prebuild.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/script/fonts.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/script/file_count.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/date2ver
        ${PREBUILD_FONT_INPUTS}
    VERBATIM
)
add_custom_target(${PROJECT_NAME}_prebuild DEPENDS ${PREBUILD_STAMP})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_prebuild)
