/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:57:20
 * @FilePath: /hana_frame/src/protocol/packet_handler.h
 * @Description: 消息处理器
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef __cmd_handler_h__
#define __cmd_handler_h__

#include "packet_base.h"

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
