/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-15 00:00:00
 * @LastEditTime: 2026-10-15 10:45:52
 * @FilePath: /kk_frame/src/comm/packet/ask_ack.h
 * @Description: 线性重发器实现类（stop-and-wait）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

/**
 * // McuMgr 构造
 * mAskAck = new AskMustAck(mPacket, [this](BuffData* b){ return mChannel->send(b); });
 * 
 * // init 中 channel 就绪后
 * mAskAck->start();
 * 
 * // send2Mcu 内：编码完成后
 * mAskAck->add(&snd); // 接管Ask Buf所有权
 * BuffData* out = mAskAck->get();
 * mChannel->send(out);
 * 
 * // onCommDeal 内
 * mAskAck->del(ack);
 * mAskAck->clearTimeout();
**/

#ifndef __ASK_ACK_H__
#define __ASK_ACK_H__

#include "packet_base.h"
#include "packet_buffer.h"
#include "tick_mgr.h"

#include <functional>
#include <stdint.h>
#include <vector>

/// @brief 线性重发器（stop-and-wait）
/// @note 维护"待确认发送包"队列，超时自动重发，直到收到匹配 ack
class AskMustAck : public TickMgr::ITickClass {
public:
    /// @brief 发送回调：上层通道提供发送接口
    /// @param bd 待发送数据包（发送后由通道自动回收，回调内无需 recycle）
    /// @return 0 成功，非 0 失败
    using SendFunc = std::function<int(BuffData*)>;

private:
    /// @brief 待确认包
    struct Packet {
        int       ackCmd{ 0 };        // 期望收到的 ack 命令（匹配与去重键）
        int64_t   addTime{ 0 };       // 入队时间（毫秒）
        BuffData* bd{ nullptr };      // 待确认数据
    };

private:
    PacketBufferPool*   mPool{ nullptr };    // 数据包缓存池
    SendFunc            mSender{ nullptr };  // 发送回调
    std::vector<Packet> mPackets;            // 待确认队列
    int64_t             mTimeoutMs{ 1000 };  // 超时时间（毫秒）
    int                 mTimeout{ 0 };       // 累计超时次数
    bool                mStarted{ false };   // tick 启动状态

public:
    AskMustAck(PacketBufferPool* pool, SendFunc sender);
    ~AskMustAck();

    /// @brief 启动重发 tick
    /// @param tickMs tick 周期（毫秒），默认 100
    void start(int64_t tickMs = 100);

    /// @brief 停止重发 tick（幂等，可重复调用）
    void stop();

    /// @brief 设置超时时间（毫秒），默认 1000
    void setTimeout(int64_t ms) { mTimeoutMs = ms; }

    /// @brief 入队待确认包（接管 bd 所有权，成功后 *ppbd 置空）
    /// @param ppbd 数据包指针的指针
    /// @param ackCmd 期望收到的 ack 命令（用于匹配与去重），默认 0
    /// @return 0 成功，-1 失败
    int add(BuffData** ppbd, int ackCmd = 0);

    /// @brief 入队待确认包（从发送编码器接管数据包所有权）
    /// @param ask 发送编码器，ackCmd 自动取 ask->getCMD()
    /// @return 0 成功，-1 失败
    /// @note 成功后 ask 内部缓存归本类管理，ask 不应再使用
    int add(IAsk* ask);

    /// @brief 收到 ack 后移除队首匹配包
    /// @param ack 接收包（以 getCMD 匹配队首）
    /// @return 0 成功，-1 失败（队列空或命令不匹配）
    int del(const IAck* ack);

    /// @brief 取得队首数据包副本用于发送
    /// @return 副本（需由发送方/通道回收），队列空返回 nullptr
    BuffData* get();

    /// @brief 累计超时次数
    int timeout() const { return mTimeout; }

    /// @brief 清除超时计数
    void clearTimeout() { mTimeout = 0; }

protected:
    void onTick(int64_t nowMs) override;
};

#endif // !__ASK_ACK_H__
