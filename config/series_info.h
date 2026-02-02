/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2026-02-02 09:11:56
 * @FilePath: /kk_frame/config/series_info.h
 * @Description: 项目硬件参数
 * @BugList: 
 * 
 * Copyright (c) 2025 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __SERIES_INFO_H__
#define __SERIES_INFO_H__

// cpu config
#define CPU_NAME     "SSD212"
#define CPU_BRAND    "Sigmstar"

// screen config
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  480
#define SCREEN_SIZE    "800*480"

// net config
#if defined(PRODUCT_X64)
#   define NET_LINE_NAME        "eno3"
#   define NET_WLAN_NAME        NET_LINE_NAME
#   define NET_FUNCTION_WLAN    0
#else
#   define NET_LINE_NAME        "eth0"
#   define NET_WLAN_NAME        "wlan0"
#   define NET_FUNCTION_WLAN    1
#endif

/***********************************************/

#define SYS_SCREEN_BEIGHTNESS_PWM "pwm2"   // 屏幕亮度控制PWM
#define SYS_SCREEN_BRIGHTNESS_MIN 0        // 屏幕亮度最小可设置值
#define SYS_SCREEN_BRIGHTNESS_MAX 100000   // 屏幕亮度最大可设置值

#endif /*__SERIES_INFO_H__*/
