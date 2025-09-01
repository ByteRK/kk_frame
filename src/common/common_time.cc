/*
 * @Author: hanakami
 * @Date: 2025-06-18 08:42:56
 * @email: hanakami@163.com
 * @LastEditTime: 2025-09-01 16:47:21
 * @FilePath: /hana_frame/src/common/common_time.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#include "common_time.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static struct timeval clock_to_timeval(clockid_t clk_id) {
    struct timespec ts;
    clock_gettime(clk_id, &ts);
    return (struct timeval){
        .tv_sec = ts.tv_sec,
        .tv_usec = ts.tv_nsec / 1000
    };
}

struct timeval get_utc_time() {
    return clock_to_timeval(CLOCK_REALTIME);
}

struct timeval get_local_time() {
    struct timeval utc = get_utc_time();
    time_t t = utc.tv_sec;
    struct tm local_tm;
    localtime_r(&t, &local_tm);
    
    // 转换回UTC时间表示，但已应用时区偏移
    return (struct timeval){
        .tv_sec = mktime(&local_tm),
        .tv_usec = utc.tv_usec
    };
}

struct timeval get_monotonic_time() {
    return clock_to_timeval(CLOCK_MONOTONIC);
}

struct timeval timeval_add(struct timeval a, struct timeval b) {
    struct timeval res;
    timeradd(&a, &b, &res);
    return res;
}

struct timeval timeval_sub(struct timeval a, struct timeval b) {
    struct timeval res;
    timersub(&a, &b, &res);
    return res;
}

int64_t timeval_diff_ms(struct timeval a, struct timeval b) {
    struct timeval diff = timeval_sub(a, b);
    return (diff.tv_sec * 1000LL) + (diff.tv_usec / 1000);
}

int64_t timeval_to_ms(struct timeval tv) {
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
}

struct timeval ms_to_timeval(int64_t milliseconds) {
    return (struct timeval){
        .tv_sec = milliseconds / 1000,
        .tv_usec = (milliseconds % 1000) * 1000
    };
}

struct timeval start_timer() {
    return get_monotonic_time();
}

int64_t elapsed_ms(struct timeval start) {
    struct timeval now = get_monotonic_time();
    return timeval_diff_ms(now, start);
}