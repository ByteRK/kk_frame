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

/** @brief 业务命令处理接口。 */
class IHandler {
public:
    virtual ~IHandler() { }
    /** @brief 处理已完成校验和解析的接收包；不接管 ack 生命周期。 */
    virtual void onCommDeal(IAck* ack) = 0;
    /**
     * @brief 处理带来源标识的接收包；默认兼容既有单连接处理器。
     * @param id TCP 服务端客户端标识，其他通道为 -1。
     */
    virtual void onCommDeal(IAck* ack, int id) {
        (void)id;
        onCommDeal(ack);
    }
};

#define g_packetMgr IHandlerManager::ins()

/**
 * @brief 按命令类型管理并分发业务处理器的进程内单例。
 *
 * 同一命令可注册多个不同处理器，同一处理器不能对同一命令重复注册。
 * 当前实现不提供内部并发保护，注册、移除和分发应在同一业务线程执行。
 */
class IHandlerManager {
    typedef std::vector<IHandler*> handlers;

protected:
    IHandlerManager();

public:
    /** @brief 返回全局唯一的命令处理器管理器。 */
    static IHandlerManager* ins() {
        static IHandlerManager stIns;
        return &stIns;
    }

    /** @brief 为 cmd 注册处理器，成功返回 true，空指针或重复注册返回 false。 */
    bool addHandler(int cmd, IHandler* hd);
    /** @brief 从所有命令下移除指定处理器。 */
    void removeHandler(IHandler* hd);
    /** @brief 根据 ack->getType() 将数据包及来源标识同步分发给已注册处理器。 */
    void onCommand(IAck* ack, int id = -1);

private:
    std::map<int, handlers> mHandlers;
};

#endif // !__PACKET_HANDLER_H__
