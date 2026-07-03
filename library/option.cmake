# 控制子库启用与否

if(ENABLED_WIFI)
    set(USE_WPACTRL ON)           # Enable wpactrl library
    set(USE_HVTOOLS OFF)          # Enable hvTools library
    set(USE_NTPCLIENT ON)         # Enable ntpClient library
else()
    set(USE_WPACTRL OFF)          # Disable wpactrl library
    set(USE_HVTOOLS OFF)          # Disable hvTools library
    set(USE_NTPCLIENT OFF)        # Disable ntpClient library
endif()

if(ENABLED_KEYBOARD)
    set(USE_KEYBOARD ON)          # Enable keyboard library
endif()

set(USE_FASTGAUSSIANBLUR ON)      # Enable fastGaussianBlur library
set(USE_GAUSSFILTER ON)           # Enable gaussFilter library
set(USE_GAUSSIANBLUR ON)          # Enable gaussianblur library

set(USE_MD5 OFF)                  # Enable md5 library
