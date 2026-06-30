/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/packet/packet_channel.h
 * @Description: Application packet channel adapter
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PACKET_CHANNEL_H__
#define __PACKET_CHANNEL_H__

#include "packet_buffer.h"
#include "packet_handler.h"
#include "transport_handler.h"

#include <cdlog.h>

#include <stddef.h>
#include <stdint.h>
#include "string_utils.h"
#include <utility>

/**
 * @brief 将任意分段的原始字节流还原为业务数据包。
 *
 * 解码器通过 IPacketBuffer 调用具体协议解析器，完成包判定、校验、可选去重，并将
 * 合法数据包交给全局 IHandlerManager。packetBuff 的生命周期必须长于本对象。
 */
class PacketStreamDecoder {
public:
    /**
     * @param packetBuff 具体协议的数据包缓存和解析器，不接管其生命周期。
     * @param enableRepeatAccept true 允许连续分发内容完全相同的数据包。
     */
    PacketStreamDecoder(IPacketBuffer* packetBuff, bool enableRepeatAccept)
        : mPacketBuff(packetBuff),
          mLastRecv(nullptr),
          mCurrRecv(nullptr),
          mEnableRepeatAccept(enableRepeatAccept),
          mRecvCount(0),
          mCheckErrorCount(0) {
        if (mPacketBuff != nullptr) {
            mLastRecv = mPacketBuff->obtain(true);
            mCurrRecv = mPacketBuff->obtain(true);
        }
    }

    ~PacketStreamDecoder() {
        if (mPacketBuff != nullptr) {
            mPacketBuff->recycle(mLastRecv);
            mPacketBuff->recycle(mCurrRecv);
        }
        mLastRecv = nullptr;
        mCurrRecv = nullptr;
    }

    /** @brief 推进一次数据包解析，返回本次从输入中消费的字节数。 */
    int onBytes(const uint8_t* data, size_t len) {
        if (mPacketBuff == nullptr || mCurrRecv == nullptr || data == nullptr || len == 0) {
            return 0;
        }

        const int totalLen = static_cast<int>(len);
        LOG(VERBOSE) << "packet channel recv len:" << totalLen
            << " hex:" << StringUtils::hexStr(data, totalLen);

        const int consumed = mPacketBuff->add(mCurrRecv, data, totalLen);
        if (consumed <= 0 || !mPacketBuff->complete(mCurrRecv)) {
            return consumed;
        }

        LOG(VERBOSE) << "packet complete:" << StringUtils::hexStr(mCurrRecv->buf, mCurrRecv->len);
        ++mRecvCount;

        if (mPacketBuff->check(mCurrRecv)) {
            mCheckErrorCount = 0;
            if (mEnableRepeatAccept || !mPacketBuff->compare(mCurrRecv, mLastRecv)) {
                IAck* ack = mPacketBuff->ack(mCurrRecv);
                g_packetMgr->onCommand(ack);

                mLastRecv->len = mCurrRecv->len;
                memcpy(mLastRecv->buf, mCurrRecv->buf, mCurrRecv->len);
            }
        } else {
            ++mCheckErrorCount;
            LOGE("packet check failed. len=%d check_fail=%d",
                mCurrRecv->len, mCheckErrorCount);
            StringUtils::hexdump("packet channel check failed",
                mCurrRecv->buf, mCurrRecv->len, 100);
        }

        mCurrRecv->len = 0;
        return consumed;
    }

    /** @brief 返回累计识别出的完整数据包数量，包括校验失败的数据包。 */
    int64_t recvCount() const {
        return mRecvCount;
    }

    /** @brief 返回自最近一次校验成功后连续校验失败的数据包数量。 */
    int checkErrorCount() const {
        return mCheckErrorCount;
    }

private:
    IPacketBuffer* mPacketBuff;
    BuffData* mLastRecv;
    BuffData* mCurrRecv;
    bool mEnableRepeatAccept;
    int64_t mRecvCount;
    int mCheckErrorCount;
};

/**
 * @brief 将统一原始通讯接口适配为业务数据包通道。
 *
 * TransportType 只需符合 Transport 的公开接口即可，因此业务管理类可以通过替换
 * 模板类型和构造参数在 UART、TCP 客户端或 TCP 服务端之间切换。
 *
 * @tparam TransportType 实际使用的底层通讯类型。
 */
template <typename TransportType>
class PacketChannel : public TransportHandler {
public:
    /**
     * @brief 构造数据包通道，并将剩余参数转发给 TransportType 构造函数。
     * @param packetBuff 具体协议缓存，不接管其生命周期，必须长于本对象。
     */
    template <typename... Args>
    PacketChannel(IPacketBuffer* packetBuff, bool enableRepeatAccept, Args&&... args)
        : mPacketBuff(packetBuff),
          mTransport(std::forward<Args>(args)...),
          mDecoder(packetBuff, enableRepeatAccept),
          mSendCount(0),
          mLastError(0) {
        mTransport.setHandler(this);
    }

    ~PacketChannel() {
        mTransport.stop();
    }

    /** @brief 初始化并启动底层通道，成功返回 0。 */
    int init() {
        int rc = mTransport.init();
        if (rc != 0) {
            return rc;
        }
        return mTransport.start() ? 0 : -1;
    }

    /** @brief 启动底层通道，适用于已经单独完成初始化的场景。 */
    bool start() {
        return mTransport.start();
    }

    /** @brief 停止底层通道。 */
    void stop() {
        mTransport.stop();
    }

    /** @brief 驱动底层周期任务；UART 接收依赖上层周期调用此接口。 */
    void onTick() {
        mTransport.onTick();
    }

    /** @brief 返回底层通道当前是否可用，与 isConnected() 语义相同。 */
    bool isOk() const {
        return mTransport.isConnected();
    }

    /** @brief 返回底层通道当前是否可用。 */
    bool isConnected() const {
        return mTransport.isConnected();
    }

    /** @brief 直接透传原始字节，不执行数据包编码或回收操作。 */
    ssize_t send(const uint8_t* data, size_t len, int id = -1) {
        return mTransport.send(data, len, id);
    }

    /**
     * @brief 发送已编码数据包，并在发送尝试结束后自动回收 ask。
     * @return 全部字节发送成功返回 0，否则返回 -1。
     * @note packetBuff 有效时，无论连接或发送结果如何，调用后 ask 均不再归调用者所有。
     */
    int send(BuffData* ask, int id = -1) {
        if (ask == nullptr || mPacketBuff == nullptr) {
            return -1;
        }

        if (!mTransport.isConnected()) {
            mPacketBuff->recycle(ask);
            return -1;
        }

        const ssize_t rc = mTransport.send(ask->buf, ask->len, id);
        const int result = (rc == ask->len) ? 0 : -1;
        mPacketBuff->recycle(ask);

        if (result == 0) {
            ++mSendCount;
        }
        return result;
    }

    /** @brief 接收底层连接事件；服务端 id 表示具体客户端。 */
    void onConnected(int id = -1) override {
        LOGI("PacketChannel connected. id=%d", id);
    }

    /** @brief 接收底层断开事件；服务端 id 表示具体客户端。 */
    void onDisconnected(int id = -1) override {
        LOGW("PacketChannel disconnected. id=%d", id);
    }

    /** @brief 按解码器返回的消费长度持续处理原始字节；当前数据包分发不保留来源 id。 */
    void onRecv(const uint8_t* data, size_t len, int /*id*/ = -1) override {
        size_t offset = 0;
        while (offset < len) {
            const size_t remaining = len - offset;
            const int consumed = mDecoder.onBytes(data + offset, remaining);
            if (consumed <= 0 || static_cast<size_t>(consumed) > remaining) {
                LOGE("PacketChannel decoder invalid consumption. consumed=%d remain=%zu",
                    consumed, remaining);
                break;
            }
            offset += static_cast<size_t>(consumed);
        }
    }

    /** @brief 记录最近一次底层通讯错误。 */
    void onError(int err) override {
        mLastError = err;
        LOGE("PacketChannel error. err=%d", err);
    }

    /** @brief 返回累计完整发送成功的数据包数量。 */
    int64_t sendCount() const {
        return mSendCount;
    }

    /** @brief 返回累计识别出的完整接收包数量。 */
    int64_t recvCount() const {
        return mDecoder.recvCount();
    }

    /** @brief 返回最近一次底层通讯错误码，尚未出错时为 0。 */
    int lastError() const {
        return mLastError;
    }

private:
    IPacketBuffer*      mPacketBuff;  // 通讯数据缓存
    TransportType       mTransport;   // 通讯通道
    PacketStreamDecoder mDecoder;     // 解包器
    int64_t             mSendCount;   // 发送统计
    int                 mLastError;   // 最后一次错误码
};

#endif // !__PACKET_CHANNEL_H__
