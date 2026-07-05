/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-01 22:42:41
 * @LastEditTime: 2026-07-05 22:20:22
 * @FilePath: /kk_frame/src/comm/packet/packet_channel.h
 * @Description: 通讯数据包处理管道
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_CHANNEL_H__
#define __PACKET_CHANNEL_H__

/******************* 模式定义开始 *******************/

#ifndef PACKET_CHANNEL_ENABLE_MULTI_ID
/// @brief 多客户端处理机制开关
/// @note 开启后，onRecv 中的 id 参数将有效，且激活独立解码器机制
/// @note 关闭后所有来源共享默认解码器，并通过 id 限制同一时间只接受一个客户端
#define PACKET_CHANNEL_ENABLE_MULTI_ID 0
#endif

/******************* 模式定义结束 *******************/

#if PACKET_CHANNEL_ENABLE_MULTI_ID
#include <map>
#include <memory>
#endif
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <utility>

#include "packet_buffer.h"
#include "transport.h"
#include <cdlog.h>

/// @brief 通讯数据流解码工具
class PacketStreamDecoder {
private:
    PacketBufferPool*     mPacketPool;      // 数据包缓存池
    std::unique_ptr<IAck> mAck;             // 当前流独立的接收解码器
    BuffData*      mLastRecv{ nullptr };    // 上次数据包
    BuffData*      mCurrRecv{ nullptr };    // 当前数据包
    bool           mEnableRepeatAccept;     // 是否允许连续重复包
    int64_t        mRecvCount{ 0 };         // 接收整包计数
    int            mCheckErrorCount{ 0 };   // 当前连续校验失败包数量

public:
    PacketStreamDecoder(PacketBufferPool* packetPool, bool enableRepeatAccept);
    ~PacketStreamDecoder();
    int     onBytes(const uint8_t* data, size_t len, int id = -1);
    void    reset();
    int64_t recvCount() const;
    int     checkErrorCount() const;

private:
    static bool isSamePacket(const BuffData* lhs, const BuffData* rhs);
};

/// @brief 通讯数据管道 Transport -> PacketStreamDecoder
/// @tparam 数据源类型(Transport派生)
template <typename TransportType>
class PacketChannel : public TransportHandler {
private:
    TransportType                mTransport;              // 通讯数据通道
    PacketBufferPool*            mPacketPool;             // 通讯数据包缓存池
    PacketStreamDecoder          mDecoder;                // 默认数据流解码工具
    int64_t                      mSendCount{ 0 };         // 发送统计
    int64_t                      mRecvCount{ 0 };         // 接收统计
    int                          mLastError{ 0 };         // 最后一次错误码
#if PACKET_CHANNEL_ENABLE_MULTI_ID
    typedef std::unique_ptr<PacketStreamDecoder> ClientDecoder;           // 客户端数据流解码类
    bool                                         mEnableRepeatAccept;     // 是否允许连续重复包
    std::map<int, ClientDecoder>                 mClientDecoders;         // 分客户端数据流解码工具
#else
    int                                          mSingleClientId{ -1 };   // 唯一客户端 ID
#endif

public:
    /// @brief 构造数据管道，并将剩余参数转发给 TransportType 构造函数
    /// @tparam ...Args 
    /// @param packetBuff 数据包缓存管理器
    /// @param enableRepeatAccept 是否允许连续重复包
    /// @param ...args TransportType 构造函数参数
    template <typename... Args>
    PacketChannel(PacketBufferPool* packetPool, bool enableRepeatAccept, Args&&... args)
        : mPacketPool(packetPool),
        mDecoder(packetPool, enableRepeatAccept),
#if PACKET_CHANNEL_ENABLE_MULTI_ID
        mEnableRepeatAccept(enableRepeatAccept),
#endif
        mTransport(std::forward<Args>(args)...) {
        mTransport.setHandler(this);
    }

    /// @brief 析构数据管道，并停止底层通道
    ~PacketChannel() {
        mTransport.stop();
    }

    /// @brief 初始化并启动底层通道
    /// @return 0 成功
    int init() {
        int rc = mTransport.init();
        if (rc != 0) return rc;
        return mTransport.start() ? 0 : -1;
    }

    /// @brief 启动底层通道
    /// @return true 成功
    bool start() {
        return mTransport.start();
    }

    /// @brief 停止底层通道
    void stop() {
        mTransport.stop();
    }

    /// @brief 可用状态返回
    /// @return true 可用
    bool isOk() const {
        return mTransport.isConnected();
    }

    /// @brief 获取底层通道连接状态
    /// @return true 已连接
    bool isConnected() const {
        return mTransport.isConnected();
    }

    /// @brief 原始数据发送
    /// @param data 数据
    /// @param len 数据长度
    /// @param id 客户端 id
    /// @return 实际发送长度
    ssize_t send(const uint8_t* data, size_t len, int id = -1) {
        return mTransport.send(data, len, id);
    }

    /// @brief 发送已编码数据包
    /// @param ask 数据包
    /// @param id 客户端 id
    /// @return 全部字节发送成功返回 0，否则返回 -1。
    /// @note 调用后，数据包自动回收，无需手动释放
    int send(BuffData* ask, int id = -1) {
        if (ask == nullptr || mPacketPool == nullptr) {
            return -1;
        }

        if (!mTransport.isConnected()) {
            mPacketPool->recycle(ask);
            return -1;
        }

        const ssize_t rc = mTransport.send(ask->buf, ask->len, id);
        const int result = (rc == ask->len) ? 0 : -1;
        mPacketPool->recycle(ask);

        if (result == 0) {
            ++mSendCount;
        }
        return result;
    }

    /// @brief 设备连接事件处理
    /// @param id 客户端 id
    void onConnected(int id = -1) override {
#if PACKET_CHANNEL_ENABLE_MULTI_ID
        if (id >= 0) {
            ensureDecoder(id);
        }
#else
        if (!acceptSingleId(id, "connected")) {
            return;
        }
#endif
        LOGI("PacketChannel connected. id=%d", id);
    }

    /// @brief 设备断开事件处理
    /// @param id 客户端 id
    void onDisconnected(int id = -1) override {
#if PACKET_CHANNEL_ENABLE_MULTI_ID
        if (id >= 0) {
            mClientDecoders.erase(id);
        }
#else
        if (!acceptSingleId(id, "disconnected")) {
            return;
        }
        if (id >= 0) {
            mSingleClientId = -1;
        }
        mDecoder.reset();
#endif
        LOGW("PacketChannel disconnected. id=%d", id);
    }

    /// @brief 原始数据接收事件处理
    /// @param data 数据
    /// @param len 数据长度
    /// @param id 客户端 id
    void onRecv(const uint8_t* data, size_t len, int id = -1) override {
#if PACKET_CHANNEL_ENABLE_MULTI_ID
        PacketStreamDecoder* decoder = id >= 0 ? ensureDecoder(id) : &mDecoder;
#else
        if (!acceptSingleId(id, "data")) {
            return;
        }
        PacketStreamDecoder* decoder = &mDecoder;
#endif
        if (decoder == nullptr) {
            return;
        }

        size_t offset = 0;
        while (offset < len) {
            const size_t remaining = len - offset;
            const int64_t recvCountBefore = decoder->recvCount();
            const int consumed = decoder->onBytes(data + offset, remaining, id);
            mRecvCount += decoder->recvCount() - recvCountBefore;
            if (consumed <= 0 || static_cast<size_t>(consumed) > remaining) {
                LOGE("PacketChannel decoder invalid consumption. id=%d consumed=%d remain=%zu",
                    id, consumed, remaining);
                break;
            }
            offset += static_cast<size_t>(consumed);
        }
    }

    /// @brief 错误事件处理
    /// @param err 错误码
    void onError(int err) override {
        mLastError = err;
        LOGE("PacketChannel error. err=%d", err);
    }

    /// @brief 返回累计发送包数量
    /// @return 累计发送包数量
    int64_t sendCount() const {
        return mSendCount;
    }

    /// @brief 返回累计接收完整包数量
    /// @return 累计接收完整包数量
    int64_t recvCount() const {
        return mRecvCount;
    }

    /// @brief 返回最近一次通道错误
    /// @return 最近一次通道错误
    int lastError() const {
        return mLastError;
    }

private:
#if PACKET_CHANNEL_ENABLE_MULTI_ID
    /// @brief 根据客户端ID获取解码器
    /// @param id 客户端ID
    /// @return 解码器
    PacketStreamDecoder* ensureDecoder(int id) {
        auto it = mClientDecoders.find(id);
        if (it != mClientDecoders.end()) {
            return it->second.get();
        }

        std::unique_ptr<PacketStreamDecoder> decoder(
            new PacketStreamDecoder(mPacketPool, mEnableRepeatAccept));
        PacketStreamDecoder* result = decoder.get();
        mClientDecoders[id] = std::move(decoder);
        return result;
    }
#else
    /// @brief 检查来源是否单客户端
    /// @param id 客户端 id
    /// @param event 事件名称
    /// @return true 单ID
    bool acceptSingleId(int id, const char* event) {
        if (id < 0) {
            return true;
        }
        if (mSingleClientId < 0) {
            mSingleClientId = id;
            return true;
        }
        if (mSingleClientId == id) {
            return true;
        }

        LOGE("PacketChannel unexpected client id in single-id mode. event=%s expected=%d actual=%d",
            event, mSingleClientId, id);
        return false;
    }
#endif
};

#endif // !__PACKET_CHANNEL_H__
