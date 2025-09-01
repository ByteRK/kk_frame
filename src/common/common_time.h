/*
 * @Author: hanakami
 * @Date: 2025-06-18 08:43:03
 * @email: hanakami@163.com
 * @LastEditTime: 2025-09-01 16:43:40
 * @FilePath: /hana_frame/src/common/common_time.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */


#ifndef COMMON_TIME_H
#define COMMON_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>
#include <sys/time.h>

/// @brief 获取当前UTC时间 (微秒精度)
/// @return 
struct timeval get_utc_time();

/// @brief 获取本地时间 (微秒精度，使用时区)
/// @return
struct timeval get_local_time();

/// @brief 获取单调递增时间 (不受系统时间修改影响)
/// @return
struct timeval get_monotonic_time();

/// @brief 时间相加
/// @param a
/// @param b
/// @return
struct timeval timeval_add(struct timeval a, struct timeval b);

/// @brief 时间相减 (a - b)
/// @param a
/// @param b
/// @return
struct timeval timeval_sub(struct timeval a, struct timeval b);

/// @brief 时间比较 (a - b)，返回毫秒差值
/// @param a 
/// @param b 
/// @return 
int64_t timeval_diff_ms(struct timeval a, struct timeval b);

/// @brief 将timeval转换为毫秒
/// @param tv 
/// @return 
int64_t timeval_to_ms(struct timeval tv);

/// @brief 将毫秒转换为timeval
/// @param milliseconds
/// @return
struct timeval ms_to_timeval(int64_t milliseconds);

/// @brief 获取耗时测量开始点
/// @return
struct timeval start_timer();

/// @brief 计算从开始点到现在经过的时间 (毫秒)
/// @param start 
/// @return 
int64_t elapsed_ms(struct timeval start);

#ifdef __cplusplus
}
#endif

#endif // COMMON_TIME_H