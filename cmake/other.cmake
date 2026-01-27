message(STATUS "========================================")
message(STATUS "Running Other.cmake")
message(STATUS "========================================")

# 设置编译前置依赖脚本，使每次构建前运行 prebuild.sh
add_custom_target(${PROJECT_NAME}_prebuild ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running pre-build tasks..."
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/script/prebuild.sh
    COMMAND ${CMAKE_COMMAND} -E echo "End pre-build tasks..."
    VERBATIM
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_prebuild)