
#include "cmd_handler.h"
#include "proto.h"

#include "conn_mgr.h"
#include "manage.h"

#define RECV_TIMEOUT_TIME 3000 // 接收数据超时警告

CHandlerManager *g_objHandler = CHandlerManager::ins();

CHandlerManager::CHandlerManager() {
    mRecvTimeout  = 0;
    mLastRecvTime = SystemClock::uptimeMillis();    
}

void CHandlerManager::onCommand(IAck *ack) {
    LOG(VERBOSE) << "cmd=" << std::hex << ack->getCMD();

    mRecvTimeout  = 0;
    mLastRecvTime = SystemClock::uptimeMillis();

    auto it = mHandlers.find(ack->getCMD());
    if (it == mHandlers.end() || it->second.empty()) {
        LOGW("command not deal. cmd(0x%x)", ack->getCMD());
        return;
    }
    for (IHandler *hd : it->second) {
        hd->onRecv(ack);
    }
}

void CHandlerManager::onTick() {
    int64_t now_tick = SystemClock::uptimeMillis();

    // 接收数据超时提示1次
    if (mRecvTimeout == 0 && now_tick - mLastRecvTime >= RECV_TIMEOUT_TIME) { mRecvTimeout++; }
}

bool CHandlerManager::addHandler(int cmd, IHandler *hd) {
    auto it = mHandlers.find(cmd);
    if (it == mHandlers.end()) {
        mHandlers[cmd].push_back(hd);
        return true;
    }
    auto it_find = std::find(it->second.begin(), it->second.end(), hd);
    if (it_find != it->second.end()){    
        LOGE("handler already in cmd. cmd=%d handler=%p", cmd, hd);
        return false;
    }
    mHandlers[cmd].push_back(hd);
    return true;
}

void CHandlerManager::removeHandler(IHandler *hd) {
    for (auto itm = mHandlers.begin(); itm != mHandlers.end();){
        auto it_find = std::find(itm->second.begin(), itm->second.end(), hd);
        if (it_find != itm->second.end()) {
            LOGD("cmd handler rem. cmd=%d handle=%p", itm->first, *it_find);
            itm->second.erase(it_find);
            if (itm->second.empty()) {
                itm = mHandlers.erase(itm);
                continue;
            }
        }
        itm++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
