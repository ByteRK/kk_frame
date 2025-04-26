/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2025-01-18 17:57:15
 * @FilePath: /cy_frame/config/series_config.h
 * @Description: 项目硬件参数
 * @BugList: 
 * 
 * Copyright (c) 2025 by Cy, All Rights Reserved. 
 * 
 */

#ifndef __SERIES_CONFIG_H__
#define __SERIES_CONFIG_H__

#define SERIES_NAME	"CDROID"

#define FLASH_SIZE	"128NR"
#define FLASH_16NR	1

//cpu config
#define CPU_NAME     "SSD212"
#define CPU_BRAND    "Sigmstar"

#define FUNCTION_WIRE    1
#ifdef DEBUG
    #define FUNCTION_WIFI    0
#else
    #define FUNCTION_WIFI    1
#endif

/***********************************************/

#define WLAN_NAME     "wlan0"
#define WIRE_NAME     ""

#define SYS_SCREEN_BEIGHTNESS_PWM "pwm2"   // 屏幕亮度控制PWM
#define SYS_SCREEN_BRIGHTNESS_MIN 0        // 屏幕亮度最小可设置值
#define SYS_SCREEN_BRIGHTNESS_MAX 100000   // 屏幕亮度最大可设置值

#endif /*__SERIES_CONFIG_H__*/
