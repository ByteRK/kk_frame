/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-05 09:57:19
 * @FilePath: /kk_frame/src/class/event_fd.h
 * @Description: 跨线程唤醒使用的事件 FD（RAII 封装）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __EVENT_FD_H__
#define __EVENT_FD_H__

#include "non_copyable.h"

/// @brief 事件 FD（eventfd + NONBLOCK + CLOEXEC）的 RAII 封装
/// @note FD 的生命周期与对象生命周期绑定
///
/// 典型用法：
///   EventFd fd;
///   if (!fd.create()) return false;
///   looper->addFd(fd.get(), 0, EVENT_INPUT, onWake, this);
///   fd.notify();   // 唤醒
///   fd.drain();    // 读空
///   // 析构即关闭；或 looper->removeFd(fd.get()); fd.close();
class EventFd : private NonCopyable {
public:
    EventFd();
    ~EventFd();

    EventFd(EventFd&& other) noexcept;
    EventFd& operator=(EventFd&& other) noexcept;

public:
    /// @brief 创建事件 FD
    /// @return true 成功
    bool create();

    /// @brief 是否已持有有效 FD
    bool valid() const;

    /// @brief 获取裸 FD
    /// @return 有效 FD；未创建时返回 -1
    int  get() const;

    /// @brief 向事件 FD 写入一次通知
    /// @return true 成功（计数器已满说明通知已待处理，同样视为成功）
    bool notify() const;

    /// @brief 清空事件 FD 中所有待处理通知
    /// @return true 成功（无待处理数据也视为成功）
    bool drain() const;

    /// @brief 关闭 FD 并复位为无效
    void close();

public:
    /// @brief 向指定 fd 写入一次通知
    static bool notifyFd(int fd);

    /// @brief 清空指定 fd 中的所有待处理通知
    static bool drainFd(int fd);

private:
    int mFd;
};

#endif // !__EVENT_FD_H__
