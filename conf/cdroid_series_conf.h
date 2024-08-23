#ifndef __HV_SERIES_CONFIG_H__
#define __HV_SERIES_CONFIG_H__

#define HV_SERIES_NAME	"CDROID"

#define HV_FLASH_SIZE	"128NR"
#define HV_FLASH_16NR	1

//cpu config
#define HV_CPU_NAME     "SSD202"
#define HV_CPU_BRAND    "Sigmstar"
#define HV_SIGM_SSD202   1

#define HV_FUNCTION_WIRE    1
#ifdef DEBUG
    #define HV_FUNCTION_WIFI    0
#else
    #define HV_FUNCTION_WIFI    1
#endif

/***********************************************/
#define HV_WLAN_NAME     "wlan0"

#define HV_WIRE_NAME     ""


#endif /*__HV_SERIES_CONFIG_H__*/
