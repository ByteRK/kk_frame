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

class PacketStreamDecoder {
public:
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

    int onBytes(const uint8_t* data, size_t len) {
        if (mPacketBuff == nullptr || mCurrRecv == nullptr || data == nullptr || len == 0) {
            return 0;
        }

        int offset = 0;
        const int totalLen = static_cast<int>(len);
        LOG(VERBOSE) << "packet channel recv len:" << totalLen
            << " hex:" << StringUtils::hexStr(data, totalLen);

        while (offset < totalLen) {
            int consumed = mPacketBuff->add(
                mCurrRecv,
                const_cast<uint8_t*>(data) + offset,
                totalLen - offset);
            if (consumed <= 0) {
                break;
            }
            offset += consumed;

            if (!mPacketBuff->complete(mCurrRecv)) {
                break;
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
        }

        return offset;
    }

    int64_t recvCount() const {
        return mRecvCount;
    }

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

template <typename TransportType>
class PacketChannel : public TransportHandler {
public:
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

    int init() {
        int rc = mTransport.init();
        if (rc != 0) {
            return rc;
        }
        return mTransport.start() ? 0 : -1;
    }

    bool start() {
        return mTransport.start();
    }

    void stop() {
        mTransport.stop();
    }

    void onTick() {
        mTransport.onTick();
    }

    bool isOk() const {
        return mTransport.isConnected();
    }

    bool isConnected() const {
        return mTransport.isConnected();
    }

    ssize_t send(const uint8_t* data, size_t len, int id = -1) {
        return mTransport.send(data, len, id);
    }

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

    void onConnected(int id = -1) override {
        LOGI("PacketChannel connected. id=%d", id);
    }

    void onDisconnected(int id = -1) override {
        LOGW("PacketChannel disconnected. id=%d", id);
    }

    void onRecv(const uint8_t* data, size_t len, int /*id*/ = -1) override {
        mDecoder.onBytes(data, len);
    }

    void onError(int err) override {
        mLastError = err;
        LOGE("PacketChannel error. err=%d", err);
    }

    int64_t sendCount() const {
        return mSendCount;
    }

    int64_t recvCount() const {
        return mDecoder.recvCount();
    }

    int lastError() const {
        return mLastError;
    }

private:
    IPacketBuffer* mPacketBuff;
    TransportType mTransport;
    PacketStreamDecoder mDecoder;
    int64_t mSendCount;
    int mLastError;
};

#endif // !__PACKET_CHANNEL_H__
