/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 18:35:40
 * @FilePath: /hana_frame/config/series_config.h
 * @Description:
 * Copyright (c) 2025 by hanakami, All Rights Reserved.
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
