/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 17:00:00
 * @LastEditTime: 2026-11-06 09:34:34
 * @FilePath: /kk_frame/src/template/non_copyable.h
 * @Description: 禁止拷贝的基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __NON_COPYABLE_H__
#define __NON_COPYABLE_H__

/// @brief 禁止拷贝（可选禁止移动）的基类
/// @note 用于持有独占资源（fd、句柄、锁）的类型，避免逐类手写 = delete。
///
/// 用法：
///   class Socket : private NonCopyable { ... };            // 仅禁止拷贝
///   class EventFd : private NonCopyable { ... };           // 需自定义移动时，
///                                                          // 在派生类显式写 move 即可
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

/// @brief 既禁止拷贝也禁止移动
/// @note 适用于地址必须稳定的对象（如注册进回调表的监听器）
class NonCopyableNonMovable {
protected:
    NonCopyableNonMovable() = default;
    ~NonCopyableNonMovable() = default;

    NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
    NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
};

#endif // !__NON_COPYABLE_H__
