/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_handler.cc
 * @Description: Application packet handler manager
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_handler.h"

#include <algorithm>
#include <cdlog.h>

IHandlerManager::IHandlerManager() {
}

void IHandlerManager::onCommand(IAck* ack, int id) {
    if (ack == nullptr) {
        return;
    }

    LOG(VERBOSE) << "cmd=" << std::hex << ack->getType();

    auto it = mHandlers.find(ack->getType());
    if (it == mHandlers.end() || it->second.empty()) {
        LOGW("command not deal. type(0x%x)", ack->getType());
        return;
    }

    for (IHandler* hd : it->second) {
        if (hd != nullptr) {
            hd->onCommDeal(ack, id);
        }
    }
}

bool IHandlerManager::addHandler(int cmd, IHandler* hd) {
    if (hd == nullptr) {
        return false;
    }

    auto it = mHandlers.find(cmd);
    if (it == mHandlers.end()) {
        mHandlers[cmd].push_back(hd);
        return true;
    }

    auto itFind = std::find(it->second.begin(), it->second.end(), hd);
    if (itFind != it->second.end()) {
        LOGE("handler already in cmd. cmd=%d handler=%p", cmd, hd);
        return false;
    }

    it->second.push_back(hd);
    return true;
}

void IHandlerManager::removeHandler(IHandler* hd) {
    if (hd == nullptr) {
        return;
    }

    for (auto itm = mHandlers.begin(); itm != mHandlers.end();) {
        auto itFind = std::find(itm->second.begin(), itm->second.end(), hd);
        if (itFind != itm->second.end()) {
            LOGD("cmd handler rem. cmd=%d handle=%p", itm->first, *itFind);
            itm->second.erase(itFind);
            if (itm->second.empty()) {
                itm = mHandlers.erase(itm);
                continue;
            }
        }
        ++itm;
    }
}
