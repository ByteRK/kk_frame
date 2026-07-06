# Define a project-scoped cache option while exposing a concise local name.
# This prevents option collisions when multiple apps share one CMake build tree.
string(MAKE_C_IDENTIFIER "${PROJECT_NAME}" PROJECT_OPTION_PREFIX)
string(TOUPPER "${PROJECT_OPTION_PREFIX}" PROJECT_OPTION_PREFIX)

macro(project_option OPTION_NAME DEFAULT_VALUE DESCRIPTION)
    set(_project_option_name "${PROJECT_OPTION_PREFIX}_${OPTION_NAME}")
    option(${_project_option_name} "${DESCRIPTION}" ${DEFAULT_VALUE})
    set(${OPTION_NAME} ${${_project_option_name}})
    unset(_project_option_name)
endmacro()
