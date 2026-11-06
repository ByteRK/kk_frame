/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-04 14:00:00
 * @FilePath: /kk_frame/src/class/event_fd.cc
 * @Description: 跨线程唤醒使用的事件 FD（RAII 封装）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "event_fd.h"

#include <cdlog.h>

#include <cerrno>
#include <cstdint>
#include <sys/eventfd.h>
#include <unistd.h>
#include <utility>

EventFd::EventFd() : mFd(-1) { }

EventFd::~EventFd() {
    close();
}

EventFd::EventFd(EventFd&& other) noexcept : mFd(other.mFd) {
    other.mFd = -1;
}

EventFd& EventFd::operator=(EventFd&& other) noexcept {
    if (this != &other) {
        close();
        mFd = other.mFd;
        other.mFd = -1;
    }
    return *this;
}

bool EventFd::create() {
    if (mFd >= 0) {
        return true;
    }
    mFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return mFd >= 0;
}

bool EventFd::valid() const {
    return mFd >= 0;
}

int EventFd::get() const {
    return mFd;
}

bool EventFd::notify() const {
    return notifyFd(mFd);
}

bool EventFd::drain() const {
    return drainFd(mFd);
}

bool EventFd::notifyFd(int fd) {
    if (fd < 0) {
        return false;
    }

    const uint64_t value = 1;
    ssize_t result = 0;
    do {
        result = write(fd, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);

    // 计数器已满只说明通知已处于待处理状态，无需视为失败
    return result >= 0 || errno == EAGAIN || errno == EWOULDBLOCK;
}

bool EventFd::drainFd(int fd) {
    if (fd < 0) {
        return false;
    }

    uint64_t value = 0;
    for (;;) {
        const ssize_t result = read(fd, &value, sizeof(value));
        if (result > 0) continue;
        if (result < 0) {
            if (errno == EINTR) continue;
            // 非阻塞读到无数据：说明已排空
            if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
            LOGE("drain eventfd failed. errno=%d", errno);
            return false;
        }
        // eventfd 读不会返回 0，出现 0 说明传入的不是 eventfd
        LOGE("drain eventfd returned 0, fd=%d is not an eventfd", fd);
        return false;
    }
}

void EventFd::close() {
    if (mFd < 0) {
        return;
    }

    // close 返回 EINTR 时 fd 状态不确定，按惯例不重试
    const int result = ::close(mFd);
    mFd = -1;
    if (result != 0) {
        LOGE("close eventfd failed. errno=%d", errno);
    }
}
