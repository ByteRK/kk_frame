/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-02 01:11:16
 * @LastEditTime: 2026-07-02 01:56:55
 * @FilePath: /kk_frame/src/comm/packet/packet_channel.cc
 * @Description: 数据包处理管道
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "packet_channel.h"

#include "packet_mgr.h"
#include "string_utils.h"

#include <string.h>

/// @brief 数据流解码工具构造
/// @param packetPool 数据包缓存池
/// @param enableRepeatAccept 是否允许重复接收
PacketStreamDecoder::PacketStreamDecoder(PacketBufferPool* packetPool, bool enableRepeatAccept)
    : mPacketPool(packetPool), mEnableRepeatAccept(enableRepeatAccept) {
    if (!mPacketPool) {
        LOGE("PacketStreamDecoder packet pool is null");
        return;
    }

    mAck = mPacketPool->createAck();
    mLastRecv = mPacketPool->obtainReceive();
    mCurrRecv = mPacketPool->obtainReceive();

    if (!mAck || mLastRecv == nullptr || mCurrRecv == nullptr) {
        LOGE("PacketStreamDecoder initialization failed");
        mPacketPool->recycle(mLastRecv);
        mPacketPool->recycle(mCurrRecv);
        mLastRecv = nullptr;
        mCurrRecv = nullptr;
    }
}

/// @brief 数据流解码工具析构
PacketStreamDecoder::~PacketStreamDecoder() {
    if (mPacketPool != nullptr) {
        mPacketPool->recycle(mLastRecv);
        mPacketPool->recycle(mCurrRecv);
    }
    mLastRecv = nullptr;
    mCurrRecv = nullptr;
}

/// @brief 原始数据流推入
/// @param data 数据流
/// @param len 数据长度
/// @param id 客户端ID
/// @return 已处理字节数
int PacketStreamDecoder::onBytes(const uint8_t* data, size_t len, int id) {
    // 数据初始校验
    if (mPacketPool == nullptr || !mAck || mCurrRecv == nullptr || data == nullptr || len == 0) {
        LOGW("onBytes failed. packetPool=%p ack=%p currRecv=%p data=%p len=%zu",
            mPacketPool, mAck.get(), mCurrRecv, data, len);
        return 0;
    }

    // 转换为底层解码接口使用的长度类型
    const int totalLen = static_cast<int>(len);
    LOGV("packet channel recv len: %zu hex: \n%s", totalLen, StringUtils::hexStr(data, totalLen).c_str());

    // 数据流推入协议器缓存
    mAck->parse(mCurrRecv);
    const int consumed = mAck->add(data, totalLen);
    if (consumed <= 0 || !mAck->complete()) {
        // 未处理完或数据包不完整
        return consumed;
    }

    // 数据包完整
    LOGV("packet complete:\n%s", StringUtils::hexStr(mCurrRecv->buf, mCurrRecv->len).c_str());
    ++mRecvCount;

    // 数据包校验
    if (mAck->check()) {
        mCheckErrorCount = 0;
        // 数据包重复校验
        if (mEnableRepeatAccept || !isSamePacket(mCurrRecv, mLastRecv)) {
            g_packetMgr->onCommand(mAck.get(), id);
            // 数据包缓存更新
            mLastRecv->len = mCurrRecv->len;
            memcpy(mLastRecv->buf, mCurrRecv->buf, mCurrRecv->len);
        }
    } else {
        ++mCheckErrorCount;
        LOGE("packet check failed. len=%d check_fail=%d", mCurrRecv->len, mCheckErrorCount);
        StringUtils::hexdump("packet channel check failed", mCurrRecv->buf, mCurrRecv->len, 100);
    }

    // 当前数据包复原并返回消费长度
    mCurrRecv->len = 0;
    return consumed;
}

/// @brief 清空当前流和去重状态，保留累计接收计数
void PacketStreamDecoder::reset() {
    if (mLastRecv != nullptr) {
        mLastRecv->len = 0;
    }
    if (mCurrRecv != nullptr) {
        mCurrRecv->len = 0;
    }
    mCheckErrorCount = 0;
}

/// @brief 获取已接收数据包数量
/// @return 已接收数据包数量
/// @note 包含校验失败的数量
int64_t PacketStreamDecoder::recvCount() const {
    return mRecvCount;
}

/// @brief 获取当前连续校验失败数据包数量
/// @return 当前连续校验失败数据包数量
int PacketStreamDecoder::checkErrorCount() const {
    return mCheckErrorCount;
}

/// @brief 比较两个数据包的类型和原始数据
/// @return true 数据包相同
bool PacketStreamDecoder::isSamePacket(const BuffData* lhs, const BuffData* rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        return false;
    }
    return lhs->type == rhs->type
        && lhs->len == rhs->len
        && memcmp(lhs->buf, rhs->buf, lhs->len) == 0;
}
