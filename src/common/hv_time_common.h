#ifndef __HV_TIME_H__
#define __HV_TIME_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define HV_TIMET time_t
#define HV_TIMEV struct timeval

HV_TIMEV HV_TIME_GetLocalRealtime();
HV_TIMEV HV_TIME_GetUTCRealtime();
HV_TIMEV HV_TIME_GetMonotonic();

HV_TIMEV HV_TIME_TimevAdd(HV_TIMEV _stTimeEnd, HV_TIMEV _stTimeBegin);
HV_TIMEV HV_TIME_TimevSub(HV_TIMEV _stTimeEnd, HV_TIMEV _stTimeBegin);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


