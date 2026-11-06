/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-05 17:00:00
 * @LastEditTime: 2026-11-05 18:10:00
 * @FilePath: /kk_frame/library/macro_define.h
 * @Description: 应用与库共用的基础宏/小工具（不依赖任何框架头文件）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __LIBRARY_MACRO_DEFINE_H__
#define __LIBRARY_MACRO_DEFINE_H__

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

/// @brief 动态转换（失败返回 nullptr）
#ifndef __dc
#define __dc(T, p) dynamic_cast<T*>(p)
#endif

/// @brief 条件成立时记录错误并立即终止进程
#ifndef FailFast
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

/// @brief 释放内存并将指针置空
/// @tparam T 指针所指类型
/// @param p 指针引用
template<typename T>
void __delete(T*& p) {
    if (p) { delete p; p = nullptr; }
}

/// @brief 功能开关（CMake 注入 ENABLE_XXX=1）
/// @note 未定义时标识符在 #if 中按 0 处理，等价于"关闭"；
///       这里不用 defined()，避免 -Wexpansion-to-defined 告警。
#ifndef ENABLED
#define ENABLED(NAME)  (ENABLE_##NAME == 1)
#endif
#ifndef DISABLED
#define DISABLED(NAME) (!ENABLED(NAME))
#endif

/// @brief 文件控制开关宏（存在 <NAME>_1 文件视为开启）
#ifndef FILE_SWITCH_ON
#define FILE_SWITCH_ON(D)  (::access(#D "_1", F_OK) == 0)
#endif
/// @brief 文件控制开关宏（存在 <NAME>_0 文件视为关闭）
#ifndef FILE_SWITCH_OFF
#define FILE_SWITCH_OFF(D) (::access(#D "_0", F_OK) == 0)
#endif

#endif // !__LIBRARY_MACRO_DEFINE_H__
