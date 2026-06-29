/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-30 00:51:44
 * @LastEditTime: 2026-06-30 01:11:06
 * @FilePath: /kk_frame/src/quick_define.h
 * @Description: 一些快速操作定义
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __QUICK_DEFINE_H__
#define __QUICK_DEFINE_H__

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

/// @brief 释放内存
/// @tparam T
/// @param p
template<typename T>
void __delete(T*& p) {
    if (p) { delete p; p = nullptr; }
}

#ifndef __dc
/// @brief 动态转换
#define __dc(T, p) dynamic_cast<T*>(p)
#endif

#ifndef FailFast
/// @brief 条件成立时记录错误并立即终止进程
#define FailFast(condition, fmt, ...)                                               \
    do {                                                                            \
        if (condition) {                                                            \
            std::fprintf(stderr, "[FailFast][%s:%d] " fmt "\n",                     \
                __FILE__, __LINE__, ##__VA_ARGS__);                                 \
            std::fflush(stderr);                                                    \
            std::abort();                                                           \
        }                                                                           \
    } while (0)
#endif

#ifndef ENABLED
/// @brief 宏定义开关
#define ENABLED(NAME)  (defined(ENABLE_##NAME) && ENABLE_##NAME == 1)
#endif
#ifndef DISABLED
/// @brief 宏定义开关
#define DISABLED(NAME) (!defined(ENABLE_##NAME) || ENABLE_##NAME == 0)
#endif

#ifndef FILE_SWITCH_ON
/// @brief 文件控制开关宏
#define FILE_SWITCH_ON(D)  (::access(#D "_1", F_OK) == 0)
#endif
#ifndef FILE_SWITCH_OFF
/// @brief 文件控制开关宏
#define FILE_SWITCH_OFF(D) (::access(#D "_0", F_OK) == 0)
#endif

#endif // __QUICK_DEFINE_H__
