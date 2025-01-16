#ifndef __SERIES_CONFIG_H__
#define __SERIES_CONFIG_H__

#define SERIES_NAME	"CDROID"

#define FLASH_SIZE	"128NR"
#define FLASH_16NR	1

//cpu config
#define CPU_NAME     "SSD202"
#define CPU_BRAND    "Sigmstar"
#define SIGM_SSD202   1

#define FUNCTION_WIRE    1
#ifdef DEBUG
    #define FUNCTION_WIFI    0
#else
    #define FUNCTION_WIFI    1
#endif

/***********************************************/
#define WLAN_NAME     "wlan0"

#define WIRE_NAME     ""


#endif /*__SERIES_CONFIG_H__*/
