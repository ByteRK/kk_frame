/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2026-07-04 02:55:28
 * @FilePath: /kk_frame/config/series_info.h
 * @Description: 项目硬件参数
 * @BugList: 
 * 
 * Copyright (c) 2025 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __SERIES_INFO_H__
#define __SERIES_INFO_H__

#define TO_STR_HELPER(x) #x
#define TO_STR(x) TO_STR_HELPER(x)

// cpu config
#define CPU_NAME       "SSD212"
#define CPU_BRAND      "Sigmstar"

// flash config
#define FLASH_NAME     "SPI NAND"
#define FLASH_SIZE     "128MB"

// screen config
#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  480
#define SCREEN_SIZE    TO_STR(SCREEN_WIDTH) "*" TO_STR(SCREEN_HEIGHT)

// net config
#if defined(PRODUCT_X64)
#   define NET_LINE_NAME        "eno3"
#   define NET_WLAN_NAME        "wlp1s0"
#else
#   define NET_LINE_NAME        "eth0"
#   define NET_WLAN_NAME        "wlan0"
#endif

#endif /*__SERIES_INFO_H__*/
