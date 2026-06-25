/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_handler.h
 * @Description: Application packet handler manager
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_HANDLER_H__
#define __PACKET_HANDLER_H__

#include "packet_base.h"

#include <map>
#include <vector>

class IHandler {
public:
    virtual ~IHandler() { }
    virtual void onCommDeal(IAck* ack) = 0;
};

#define g_packetMgr IHandlerManager::ins()

class IHandlerManager {
    typedef std::vector<IHandler*> handlers;

protected:
    IHandlerManager();

public:
    static IHandlerManager* ins() {
        static IHandlerManager stIns;
        return &stIns;
    }

    bool addHandler(int cmd, IHandler* hd);
    void removeHandler(IHandler* hd);
    void onCommand(IAck* ack);

private:
    std::map<int, handlers> mHandlers;
};

#endif // !__PACKET_HANDLER_H__
