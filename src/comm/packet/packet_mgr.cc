/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-01 22:42:41
 * @LastEditTime: 2026-07-02 01:09:35
 * @FilePath: /kk_frame/src/comm/packet/packet_mgr.cc
 * @Description: 通讯数据包管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_mgr.h"

#include <algorithm>
#include <cdlog.h>

/// @brief 包处理器析构函数
PacketHandler::~PacketHandler() {
    g_packetMgr->removeHandler(this);
}

/// @brief 数据包处理（不校验ID）
/// @param ack 数据包
void PacketHandler::onCommDeal(const IAck* ack) { }

/// @brief 数据包处理（校验ID）
/// @param ack 数据包
/// @param id 来源ID（适合多客户端的服务端使用）
void PacketHandler::onCommDeal(const IAck* ack, int id) {
    onCommDeal(ack);
}

/// @brief 包管理器构造函数
PacketManager::PacketManager() { }

/// @brief 注册包处理器
/// @param type 包类型
/// @param ph 包处理器
/// @return true 成功 false 失败/重复注册
bool PacketManager::addHandler(int type, PacketHandler* ph) {
    if (ph == nullptr) return false;

    auto it = mHandlers.find(type);
    if (it == mHandlers.end()) {
        mHandlers[type].push_back(ph);
        return true;
    }

    auto itFind = std::find(it->second.begin(), it->second.end(), ph);
    if (itFind != it->second.end()) {
        LOGE("[PacketManager] handler already in type. type=%d handler=%p", type, ph);
        return false;
    }

    it->second.push_back(ph);
    return true;
}

/// @brief 移除包处理器
/// @param ph 包处理器
void PacketManager::removeHandler(PacketHandler* ph) {
    if (ph == nullptr) return;

    for (auto itm = mHandlers.begin(); itm != mHandlers.end();) {
        auto itFind = std::find(itm->second.begin(), itm->second.end(), ph);
        if (itFind != itm->second.end()) {
            LOGD("[PacketManager] handler remove. type=%d handle=%p", itm->first, *itFind);
            itm->second.erase(itFind);
            if (itm->second.empty()) {
                itm = mHandlers.erase(itm);
                continue;
            }
        }
        ++itm;
    }
}

/// @brief 数据包分发
/// @param ack 数据包
/// @param id 来源ID（适合多客户端的服务端使用）
void PacketManager::onCommand(IAck* ack, int id) {
    if (ack == nullptr) return;

    // 获取类型
    const int type = ack->getType();
    LOGV("[PacketManager] onCommand. type=%d, id=%d", type, id);

    // 判断是否存在该类型的包处理器
    auto it = mHandlers.find(type);
    if (it == mHandlers.end() || it->second.empty()) {
        LOGW("[PacketManager] command not deal. type=%d, id=%d", type, id);
        return;
    }

    const handlers snapshot = it->second;
    for (PacketHandler* hd : snapshot) {
        if (hd == nullptr) continue;

        // 判断该类型是否还存在处理器，避免回调过程中被移除
        auto current = mHandlers.find(type);
        if (current == mHandlers.end()) break;

        // 判断该处理器是否还存在
        const handlers& currentHandlers = current->second;
        if (std::find(currentHandlers.begin(), currentHandlers.end(), hd) == currentHandlers.end()) {
            continue;
        }

        // 调用处理函数
        hd->onCommDeal(ack, id);
    }
}
