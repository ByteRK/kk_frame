/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-05 09:22:24
 * @LastEditTime: 2026-02-05 17:43:09
 * @FilePath: /kk_frame/src/comm/tcp/transport.cc
 * @Description: Transport 抽象基类
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "transport.h"

Transport::Transport() : mEvents() {}

Transport::~Transport() {}

void Transport::postEvent(const TransportEvent& ev) {
    std::lock_guard<std::mutex> lock(mEventLock);
    mEvents.push(ev);
}

int Transport::checkEvents() {
    std::lock_guard<std::mutex> lock(mEventLock);
    return mEvents.empty() ? 0 : 1;
}

int Transport::handleEvents() {
    for (;;) {
        TransportEvent ev;
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
