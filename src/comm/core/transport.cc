/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:22:24
 * @LastEditTime: 2026-02-07 16:36:50
 * @FilePath: /kk_frame/src/comm/core/transport.cc
 * @Description: 数据传输基类
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "transport.h"

Transport::Transport() : mEvents() {}

Transport::~Transport() {}

int Transport::checkEvents() {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mEvents.empty() ? 0 : 1;
}

int Transport::handleEvents() {
    for (;;) {
        Transport::Event ev;
        {
            std::lock_guard<std::mutex> lock(mEventLock);
            if (mEvents.empty())
                break;
            ev = mEvents.front();
            mEvents.pop();
        }
        dispatchEvent(ev);
    }
    return 0;
}

void Transport::postEvent(const Transport::Event& ev) {
    std::lock_guard<std::mutex> lock(mEventLock);
    mEvents.push(ev);
}

void Transport::sendEvent(const Transport::Event& ev) {
    dispatchEvent(ev);
}
