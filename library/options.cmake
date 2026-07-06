# 控制子库启用与否
# 与项目功能绑定的库直接继承对应开关；独立库使用项目级 option

if(NOT COMMAND project_option)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/option_helpers.cmake)
endif()

# Wi-Fi 相关库（跟随 ENABLED_WIFI）
set(USE_WPACTRL ${ENABLED_WIFI})
set(USE_NTPCLIENT ${ENABLED_WIFI})

# 键盘库（跟随 ENABLED_KEYBOARD）
set(USE_KEYBOARD ${ENABLED_KEYBOARD})

# 可独立配置的工具库
project_option(USE_HVTOOLS          OFF   "启用 hvTools 网络工具库")
project_option(USE_FASTGAUSSIANBLUR ON    "启用快速高斯模糊库")
project_option(USE_GAUSSFILTER      ON    "启用高斯滤波库")
project_option(USE_GAUSSIANBLUR     ON    "启用高斯模糊库")
project_option(USE_MD5              OFF   "启用 MD5 工具库")
