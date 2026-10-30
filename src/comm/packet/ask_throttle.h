/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-30 15:00:00
 * @LastEditTime: 2026-10-30 15:00:00
 * @FilePath: /kk_frame/src/comm/packet/ask_throttle.h
 * @Description: 发送限流器实现类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

/**
 * // 构造（与 AskMustAck 同风格，仅解耦通道类型）
 * mThrottle = new AskThrottle(mPacket, [this](BuffData* b){ return mChannel->send(b); });
 *
 * // init 中 channel 就绪后
 * mThrottle->start(100); // 每 100ms 最多发出一包
 *
 * // 发送处：编码完成后（可一次性连续 add 多包）
 * mThrottle->add(&bd); // 接管 Ask Buf 所有权，不立即发出
 *
 * // 断连 / 停用时
 * mThrottle->clear(); // 丢弃并回收积压包
 *
 * // 注意：与 AskMustAck 是并列组件，不要串联使用
 * // 否则超时重发会被排队推迟，破坏 stop-and-wait 的超时语义
**/

#ifndef __ASK_THROTTLE_H__
#define __ASK_THROTTLE_H__

#include "packet_base.h"
#include "packet_buffer.h"
#include "tick_mgr.h"

#include <deque>
#include <functional>
#include <stddef.h>
#include <stdint.h>

/// @brief 发送限流器（固定间隔逐包发送）
/// @note 应用层可一次性入队多包，实际按固定间隔逐包交给发送回调
/// @note 与上层 Tick 同线程使用，内部无锁
class AskThrottle : public TickMgr::ITickClass {
public:
    static constexpr size_t DEFAULT_MAX_QUEUE     = 32;   // 默认队列上限
    static constexpr int64_t DEFAULT_INTERVAL_MS  = 100;  // 默认发送间隔（毫秒）

public:
    /// @brief 发送回调：上层通道提供发送接口
    /// @param bd 待发送数据包（发送后由通道自动回收，回调内无需 recycle）
    /// @return 0 成功，非 0 失败
    using SendFunc = std::function<int(BuffData*)>;

private:
    /// @brief 待发送包
    struct Packet {
        int       cmd{ 0 };        // 发送命令（合并相同命令时的匹配键）
        BuffData* bd{ nullptr };   // 待发送数据
    };

private:
    PacketBufferPool*  mPool{ nullptr };                     // 数据包缓存池（不拥有）
    SendFunc           mSender{ nullptr };                   // 发送回调
    std::deque<Packet> mPackets;                             // 待发送队列
    int64_t            mIntervalMs{ DEFAULT_INTERVAL_MS };   // 两包之间的最小发送间隔
    int64_t            mLastSendMs{ 0 };                     // 上次实际发出时间（毫秒）
    size_t             mMaxQueue{ DEFAULT_MAX_QUEUE };       // 队列上限
    int64_t            mSentCount{ 0 };                      // 累计发出包数
    int64_t            mDropCount{ 0 };                      // 累计丢弃包数
    bool               mCoalesceByCmd{ false };              // 是否合并相同命令的包
    bool               mHasSent{ false };                    // 是否已发送过（首包不受间隔限制）
    bool               mStarted{ false };                    // tick 启动状态

public:
    AskThrottle(PacketBufferPool* pool, SendFunc sender);
    ~AskThrottle();

    /// @brief 启动发送 tick
    /// @param intervalMs 两包之间的最小发送间隔（毫秒），<=0 时忽略
    /// @note tick 周期自动取 intervalMs 的 1/4（限制在 10~50ms）以保证间隔精度
    void start(int64_t intervalMs = DEFAULT_INTERVAL_MS);

    /// @brief 停止发送 tick（幂等，可重复调用），不处理剩余队列
    void stop();

    /// @brief 设置发送间隔（毫秒），<=0 时忽略
    void setInterval(int64_t ms);

    /// @brief 获取发送间隔（毫秒）
    int64_t interval() const { return mIntervalMs; }

    /// @brief 设置队列上限
    /// @param count 队列上限；小于当前积压数量时不会丢弃已有包
    void setMaxQueue(size_t count);

    /// @brief 获取队列上限
    size_t maxQueue() const { return mMaxQueue; }

    /// @brief 设置是否按命令合并：队列中同 cmd 的包仅保留最新数据（原位置不变）
    /// @param enable true 合并，false 全部保留
    void setCoalesceByCmd(bool enable) { mCoalesceByCmd = enable; }

    /// @brief 入队待发送包（接管 bd 所有权，成功后 *ppbd 置空）
    /// @param ppbd 数据包指针的指针
    /// @param cmd 发送命令（合并相同命令时的匹配键），默认 0
    /// @return 0 成功，-1 失败
    /// @note 仅在 bd 为空或缓存池为空时不接管所有权；其余失败分支已回收并置空
    int add(BuffData** ppbd, int cmd = 0);

    /// @brief 入队待发送包（从发送编码器接管数据包所有权）
    /// @param ask 发送编码器，cmd 自动取 ask->getCMD()
    /// @return 0 成功，-1 失败
    /// @note 成功后 ask 内部缓存归本类管理，ask 不应再使用
    int add(IAsk* ask);

    /// @brief 清空队列并回收全部积压包（断连或停用时调用）
    void clear();

    /// @brief 当前积压包数量
    size_t size() const { return mPackets.size(); }

    /// @brief 队列是否为空
    bool empty() const { return mPackets.empty(); }

    /// @brief 累计发出包数
    int64_t sentCount() const { return mSentCount; }

    /// @brief 累计丢弃包数（队列满载）
    int64_t dropCount() const { return mDropCount; }

protected:
    void onTick(int64_t nowMs) override;

private:
    /// @brief 由发送间隔推导 tick 周期
    /// @param intervalMs 发送间隔（毫秒）
    /// @return tick 周期（10~50 毫秒）
    static int64_t tickOf(int64_t intervalMs);
};

#endif // !__ASK_THROTTLE_H__
