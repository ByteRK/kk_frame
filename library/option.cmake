# 控制子库启用与否

if(ENABLED_WIFI)
    set(USE_WPACTRL ON)           # Enable wpactrl library
    set(USE_HVTOOLS ON)           # Enable hvTools library
else()
    set(USE_WPACTRL OFF)          # Disable wpactrl library
    set(USE_HVTOOLS OFF)          # Enable hvTools library
endif()

set(USE_FASTGAUSSIANBLUR ON)  # Enable fastGaussianBlur library
set(USE_GAUSSFILTER ON)       # Enable gaussFilter library
set(USE_GAUSSIANBLUR ON)      # Enable gaussianblur library