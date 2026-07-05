/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:33:02
 * @LastEditTime: 2026-07-05 02:19:17
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
#define CPU_NAME       "酷睿 Ultra 7 265K 二十核"
#define CPU_BRAND      "英特尔"

// memory config
#define MEMORY_NAME    "阿斯加特 DDR5 6000MHz"
#define MEMORY_SIZE    "64GB"

// flash config
#define FLASH_NAME     "ZHITAI TiPlus7100"
#define FLASH_SIZE     "8192GB"

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
