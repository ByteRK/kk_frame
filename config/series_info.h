/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-09-01 16:38:40
 * @FilePath: /hana_frame/config/series_config.h
 * @Description:
 * Copyright (c) 2025 by hanakami, All Rights Reserved.
 */

#ifndef __SERIES_INFO_H__
#define __SERIES_INFO_H__

#define SERIES_NAME	"CDROID"

#define FLASH_SIZE	"128NR"
#define FLASH_16NR	1

 //cpu config
#define CPU_NAME     "SSD212"
#define CPU_BRAND    "Sigmstar"

//screen config
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  480
#define SCREEN_SIZE    "800*480"

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
#define SYS_SCREEN_BRIGHTNESS_MAX 100      // 屏幕亮度最大可设置值

#define SYS_SCREEN_VOLUME_PWM "pwm3"   // 屏幕蜂鸣器控制PWM
#define SYS_SCREEN_VOLUME_MIN 10        // 屏幕蜂鸣器音量最小可设置值
#define SYS_SCREEN_VOLUME_MAX 90      // 屏幕蜂鸣器音量最大可设置值

#endif /*__SERIES_INFO_H__*/
