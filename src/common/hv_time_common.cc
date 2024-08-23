#include "hv_time_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

HV_TIMEV _ClockGettime(clockid_t _tclock_id)
{
	struct timespec stTime;
	clock_gettime(_tclock_id, &stTime);
	HV_TIMEV stRet;
	stRet.tv_sec = stTime.tv_sec;
	stRet.tv_usec = stTime.tv_nsec/1000;
	return stRet;
}


HV_TIMEV HV_TIME_GetLocalRealtime()
{
	HV_TIMEV stTimev = _ClockGettime(CLOCK_REALTIME);
    return stTimev;
}

HV_TIMEV HV_TIME_GetUTCRealtime()
{
	return _ClockGettime(CLOCK_REALTIME);
}

HV_TIMEV HV_TIME_GetMonotonic()
{
	return _ClockGettime(CLOCK_MONOTONIC);
}


HV_TIMEV HV_TIME_TimevAdd(HV_TIMEV _stTimeEnd, HV_TIMEV _stTimeBegin)
{
	HV_TIMEV stRet;
	
	timeradd(&_stTimeEnd, &_stTimeBegin, &stRet);
	return stRet;
}

HV_TIMEV HV_TIME_TimevSub(HV_TIMEV _stTimeEnd, HV_TIMEV _stTimeBegin)
{
	HV_TIMEV stRet;
	timersub(&_stTimeEnd, &_stTimeBegin, &stRet);
	return stRet;
}


/// @brief 时分秒转数字
/// @param ch 
/// @return 
// uint32_t HV_HMS_Char2Int(const char *ch)
// {
// 	int h = 0, m = 0, s = 0;
// 	sscanf(ch, "%d:%d:%d", &h, &m, &s);
// 	return h*3600+m*60+s;
// }

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

