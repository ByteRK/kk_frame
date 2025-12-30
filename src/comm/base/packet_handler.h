/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-01-18 11:31:51
 * @LastEditTime: 2025-12-29 14:34:54
 * @FilePath: /kk_frame/src/comm/base/packet_handler.h
 * @Description: 消息处理器
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __cmd_handler_h__
#define __cmd_handler_h__

#include "packet_base.h"
#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////////////

/// @brief 消息处理类
class IHandler {
public:
    virtual void onCommDeal(IAck* ack) = 0;
};

///////////////////////////////////////////////////////////////////////////////

#define g_packetMgr IHandlerManager::ins()

/// @brief 消息处理类管理器
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

#endif
