/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/core/transport_handler.h
 * @Description: Raw transport callback interface
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TRANSPORT_HANDLER_H__
#define __TRANSPORT_HANDLER_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 原始通讯事件回调接口。
 *
 * 回调由具体 Transport 分发。TCP 实现通常会将工作线程事件切换到 init() 所在线程的
 * Looper 中再回调；显式 stop() 期间为避免生命周期事件被清空，可能在 stop() 调用线程
 * 同步排空已投递事件。UART 实现则在调用 start()/stop()/onTick() 的线程中同步回调。
 * 回调参数中的数据只在本次 onRecv() 调用期间有效，如需异步使用必须自行复制。
 * 回调中允许调用通道的 stop()，当前回调返回后剩余待分发事件会被取消。禁止在
 * 回调中直接销毁 Transport、PacketChannel 或当前 handler；销毁必须延迟到回调返回后。
 */
class TransportHandler {
public:
    virtual ~TransportHandler() { }

    /** @brief 通讯通道已建立；服务端的 id 为客户端标识，其他通道通常为 -1。 */
    virtual void onConnected(int id = -1) { }
    /** @brief 通讯通道已断开；服务端的 id 为客户端标识，其他通道通常为 -1。 */
    virtual void onDisconnected(int id = -1) { }
    /** @brief 收到原始字节流，数据尚未进行粘包、拆包或协议校验。 */
    virtual void onRecv(const uint8_t* data, size_t len, int id = -1) = 0;
    /** @brief 通讯过程中发生错误，err 通常为系统 errno。 */
    virtual void onError(int err) { }
};

#endif // !__TRANSPORT_HANDLER_H__
