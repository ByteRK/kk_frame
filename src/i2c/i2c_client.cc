
#include "i2c_client.h"
#include "cmd_handler.h"

#include <core/app.h>
#include <core/systemclock.h>


#define RECV_PACKET_TIME_SPACE 100  // epoll_wait 收包间隔ms
#define SEND_PACKET_TIME_SPACE 50   // 发包间隔ms

I2CClient::I2CClient(IPacketBuffer* ipacket, BufferType type, int recv_space)
    : mPacketBuff(ipacket), mBufType(type), mRecvSpace(recv_space) {

    mLastSendTime = 0; // 最后一次发包时间
    mSendCount = 0;
    mRecvCount = 0;
    mChkErrCount = 0;
    mLastRecvTime = SystemClock::uptimeMillis() + 5000;
    mLastRecv = mPacketBuff->obtain(type);
    mCurrRecv = mPacketBuff->obtain(type);
    mLastSndHeart = SystemClock::uptimeMillis();
    mSerialOk = 0;
    mLastDealDataTime = 0;
    mRSBuf = (IICBuf*)calloc(1, sizeof(IICBuf) + LEN_4K);
    mRSBuf->start = mRSBuf->buf;
    mRSBuf->pos = mRSBuf->start;
    mRSBuf->last = mRSBuf->pos;
    mRSBuf->end = mRSBuf->start + LEN_4K;
}

I2CClient::~I2CClient() {
    mPacketBuff->recycle(mLastRecv);
    mPacketBuff->recycle(mCurrRecv);

    for (BuffData* ask : mSendQueue) { mPacketBuff->recycle(ask); }
    mSendQueue.clear();
}

int I2CClient::init() {
    sw_i2c_init();

    // 读取缓冲区积累的数据丢弃
#if 0
    int read_len;
    while ((read_len = i2c_mcu_read2(I2C_ADDR_A, mRSBuf->pos, mRSBuf->end - mRSBuf->last)) >= mRSBuf->end - mRSBuf->last) {
        LOG(VERBOSE) << "fore-read data, give up. buf=" << hexstr(mRSBuf->pos, read_len);
    }
    while ((read_len = i2c_mcu_read2(I2C_ADDR_B, mRSBuf->pos, mRSBuf->end - mRSBuf->last)) >= mRSBuf->end - mRSBuf->last) {
        LOG(VERBOSE) << "fore-read data, give up. buf=" << hexstr(mRSBuf->pos, read_len);
    }
#endif

    App::getInstance().addEventHandler(this);
    return 0;
}


int I2CClient::readData() {
    int read_len = 0;
    // int64_t curr_tick = SystemClock::uptimeMillis();
    // if (curr_tick - mLastRecvTime < mRecvSpace) { return 1; }
    // mLastRecvTime = curr_tick;

    // read_len = i2c_mcu_read2(I2C_ADDR_A, mRSBuf->last, mRSBuf->end - mRSBuf->last);
    // if (read_len > 0) {
    //     mRSBuf->last += read_len;
    // } else {
    //     LOGE("i2c recv error. result=%d", read_len);
    //     return 0;
    // }
    return read_len;
}

int I2CClient::onRecvData() {
    int    count = 0;
    size_t data_len = mRSBuf->last - mRSBuf->pos;
    // iic数据包
    int64_t oldRecvCount = mRecvCount;
    onUartData(mRSBuf->pos, 9);
    mRSBuf->pos = mRSBuf->start;
    mRSBuf->last = mRSBuf->pos;
    count = mRecvCount - oldRecvCount;

    return count;
}

int I2CClient::onUartData(uchar* buf, int len) {
    int offset = 0;
    if (len > 0) { LOG(VERBOSE) << "len:" << len << " hex:" << hexstr(buf, len); }

    for (; offset < len;) {
        offset += mPacketBuff->add(mCurrRecv, buf, len);
        if (!mPacketBuff->complete(mCurrRecv)) { break; }
        mRecvCount++;

        if (mPacketBuff->check(mCurrRecv)) {
            mChkErrCount = 0;
            // 检查重复数据包
            if (!mPacketBuff->compare(mCurrRecv, mLastRecv) || checkDealData()) {
                //LOG(DEBUG) << "new packet:" << mPacketBuff->str(mCurrRecv);
                IAck* ack = mPacketBuff->ack(mCurrRecv);
                g_objHandler->onCommand(ack);

                // 保存本次数据包
                mLastRecv->len = mCurrRecv->len;

                mLastSendTime = 0;
            }
        } else {
            mChkErrCount++;
            hexdump("IIC recv data, check fail", mCurrRecv->buf, mCurrRecv->len);
        }

        mCurrRecv->len = 0;
    }

    return 0;
}

void I2CClient::onTick() {
    int64_t now_tick = SystemClock::uptimeMillis();

    static int64_t s_last_time = 0;
    if (now_tick - s_last_time >= 60000) {
        s_last_time = now_tick;
        LOG(DEBUG) << " packet info. send=" << mSendCount << " recv=" << mRecvCount
            << " check_fail=" << mChkErrCount;
    }

    g_objHandler->onTick();
    // 发送队列不为空
    if (now_tick - mLastSendTime >= SEND_PACKET_TIME_SPACE && !mSendQueue.empty()) {
        mLastSendTime = now_tick;
        BuffData* ask = mSendQueue.front();

        mSendQueue.pop_front();

        mPacketBuff->check_code(ask);

        sendTrans(ask);
        mPacketBuff->recycle(ask);
        mSendCount++;
    }
}

int I2CClient::send(BuffData* ask) {
    // 检查队列是否有相同的包还未发送
    for (auto it = mSendQueue.begin(); it != mSendQueue.end(); it++) {
        BuffData* dst = *it;
        if (mPacketBuff->compare(ask, dst)) {
            // LOG(WARN) << "same package exists! str=" << mPacketBuff->str(ask);
            mPacketBuff->recycle(ask);
            return 1;
        }
    }
    mSendQueue.push_back(ask);

    return 0;
}



void I2CClient::sendTrans(BuffData* ask) {
    if (ask->buf[2] == 0x0A) {
        LOG(VERBOSE) << std::to_string(I2C_ADDR_A) << " iic send:" << hexstr(ask->buf, ask->len);
        i2c_mcu_write2(I2C_ADDR_A, ask->buf, ask->len);
    } else {
        LOG(VERBOSE) << std::to_string(I2C_ADDR_B) << " iic send:" << hexstr(ask->buf, ask->len);
        i2c_mcu_write2(I2C_ADDR_B, ask->buf, ask->len);
    }
    // mLastRecvTime = SystemClock::uptimeMillis();
}

int I2CClient::checkEvents() {
    int64_t now_time_tick = SystemClock::uptimeMillis();
    if (now_time_tick - mLastRecvTime >= mRecvSpace) {
        mLastRecvTime = now_time_tick;
        return 1;
    }

    return 0;
}

int I2CClient::handleEvents() {
    int read_len = i2c_mcu_read2(I2C_ADDR_A, mRSBuf->last, 9);
    // hexdump("buf", mRSBuf->last, 9);
    if (read_len == 0) {
        mRSBuf->last += read_len;
        onRecvData();
    }

    read_len = i2c_mcu_read2(I2C_ADDR_B, mRSBuf->last, 9);
    // hexdump("buf", mRSBuf->last, 9);
    if (read_len == 0) {
        mRSBuf->last += read_len;
        onRecvData();
    }

    return 1;
}
bool I2CClient::checkDealData() {
    int64_t now_time_tick = SystemClock::uptimeMillis();
    if (now_time_tick - mLastDealDataTime >= 1000) {
        mLastDealDataTime = now_time_tick;
        return true;
    }
    return false;
}

int I2CClient::getRecvSpace() {
    return mRecvSpace;
}
