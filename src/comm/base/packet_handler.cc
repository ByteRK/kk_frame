
#include "packet_handler.h"
#include "proto.h"
#include <algorithm>
#include <cdlog.h>

IHandlerManager::IHandlerManager() {
}

/// @brief 分发信息
/// @param ack 
void IHandlerManager::onCommand(IAck* ack) {
    LOG(VERBOSE) << "cmd=" << std::hex << ack->getType();

    auto it = mHandlers.find(ack->getType());
    if (it == mHandlers.end() || it->second.empty()) {
        LOGW("command not deal. type(0x%x)", ack->getType());
        return;
    }
    for (IHandler* hd : it->second) {
        hd->onCommDeal(ack);
    }
}

/// @brief 注册命令处理
/// @param cmd 
/// @param hd 
/// @return 
bool IHandlerManager::addHandler(int cmd, IHandler* hd) {
    auto it = mHandlers.find(cmd);
    if (it == mHandlers.end()) {
        mHandlers[cmd].push_back(hd);
        return true;
    }
    auto it_find = std::find(it->second.begin(), it->second.end(), hd);
    if (it_find != it->second.end()) {
        LOGE("handler already in cmd. cmd=%d handler=%p", cmd, hd);
        return false;
    }
    mHandlers[cmd].push_back(hd);
    return true;
}

/// @brief 移除命令处理
/// @param hd 
void IHandlerManager::removeHandler(IHandler* hd) {
    for (auto itm = mHandlers.begin(); itm != mHandlers.end();) {
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
