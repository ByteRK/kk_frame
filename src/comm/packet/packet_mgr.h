/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-01 22:42:41
 * @LastEditTime: 2026-07-05 20:55:54
 * @FilePath: /kk_frame/src/comm/packet/packet_mgr.h
 * @Description: 通讯数据包分发管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_HANDLER_H__
#define __PACKET_HANDLER_H__

#include "packet_base.h"
#include "singleton.h"

#include <map>
#include <vector>

#define g_packetMgr PacketManager::instance()

/// @brief 通讯数据包处理器
class PacketHandler {
public:
    virtual ~PacketHandler();
    virtual void onCommDeal(const IAck* ack);
    virtual void onCommDeal(const IAck* ack, int id);
};

/// @brief 通讯数据包分发管理器
/// @note 支持一对多以及多对一分发
/// @note 不支持并发，必须在同一线程调用
/// @note 分发过程新增处理器下包生效
class PacketManager : public Singleton<PacketManager> {
    friend  Singleton<PacketManager>;
    typedef std::vector<PacketHandler*> Handlers;

private:
    std::map<int, Handlers> mHandlers; // <数据包类型, 处理器列表>

protected:
    PacketManager();

public:
    bool addHandler(int type, PacketHandler* ph);
    void removeHandler(PacketHandler* ph);
    void onCommand(IAck* ack, int id = -1);
};

#endif // !__PACKET_HANDLER_H__
