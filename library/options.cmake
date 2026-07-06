# 控制子库启用与否

# Wi-Fi 相关库（跟随 ENABLED_WIFI）
set(USE_WPACTRL ${ENABLED_WIFI})
set(USE_NTPCLIENT ${ENABLED_WIFI})

# 键盘库（跟随 ENABLED_KEYBOARD）
set(USE_KEYBOARD ${ENABLED_KEYBOARD})

# 可独立配置的工具库
set(USE_HVTOOLS          OFF)       # hvTools 网络工具库
set(USE_FASTGAUSSIANBLUR ON)        # 快速高斯模糊库
set(USE_GAUSSFILTER      ON)        # 高斯滤波库
set(USE_GAUSSIANBLUR     ON)        # 高斯模糊库
set(USE_MD5              OFF)       # MD5 工具库
