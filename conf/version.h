#ifndef __CONF_VERSION_H__
#define __CONF_VERSION_H__

#include <stdio.h>
#include "hv_series_conf.h"

#define HV_SOFT_VER_MAJ "HV_MAJ"
#define HV_SOFT_VER_MIN "HV_MIN"

#define HV_SOFT_VER_EXT "RevTag"

#define HV_SOFT_VER_TIME "RevTIME"

#define HV_SOFT_GIT_HARD "RevGITLAB"

#define HV_SOFT_VERSION "V" HV_CPU_NAME "." HV_SOFT_VER_MAJ "." HV_SOFT_VER_MIN "." HV_SOFT_VER_EXT "." HV_SOFT_VER_TIME
#define HV_SOFT_VERSION_NEW HV_SOFT_VER_MAJ "." HV_SOFT_VER_EXT "." HV_SOFT_VER_MIN "." HV_SOFT_VER_TIME


#define HV_OS_VER_S "Linux 4.4"

#define HV_HARD_VER_S "RevHardVer"

#define HV_HARD_VERSION "V" HV_CPU_NAME "." HV_HARD_VER_S


/// @brief 获取格式化后的软件版本，用于展示
/// @param _szBuf 
/// @return 
static inline char* HV_FORMAT_VERSION_STRING(char* _szBuf)
{
	sprintf(_szBuf, "%s_V%s",HV_SERIES_NAME,HV_SOFT_VER_EXT);
	return _szBuf;
}

/// @brief 获取当前版本构建时间
/// @return 
static inline const char* HV_FORMAT_VER_TIME_STRING()
{
	return HV_SOFT_VER_TIME;
}

/// @brief 获取当前版本号
/// @return 
static inline const char* HV_FORMAT_VER_EXT_STRING()
{
	return HV_SOFT_VER_EXT;
}

static inline const char* HV_FORMAT_GIT_HARD_STRING()
{
	return HV_SOFT_GIT_HARD;
}

#endif

