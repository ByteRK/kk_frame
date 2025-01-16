
#include "i2c_client.h"
#include "cmd_handler.h"

#include <core/app.h>
#include <core/systemclock.h>


#define RECV_PACKET_TIME_SPACE 100  // epoll_wait 收包间隔ms
#define SEND_PACKET_TIME_SPACE 50   // 发包间隔ms

I2CClient::I2CClient(IPacketBuffer* ipacket, BufferType type, I2COpenReq& i2cInfo
    , const std::string& ip, short port, int recv_space)
    : mPacketBuff(ipacket), mBufType(type), mI2cOpenReq(i2cInfo), mIp(ip), mPort(port), mRecvSpace(recv_space) {

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

#if CONN_TCP
#ifdef DEBUG
    SocketClient::init(mIp.c_str(), mPort);
#else
    SocketClient::init();
#endif
    if (mRecvSpace >= RECV_PACKET_TIME_SPACE) {
        App::getInstance().addEventHandler(this);
    }
#else
#ifndef DEBUG
    if (mI2cOpenReq.type == I2C_TYPE_SW)
        sw_i2c_init();
    else
        hw_i2c_init();

    // 读消息事件处理
    mFd = 0;
    mStatus = ST_CONNECTED;
    onStatusChange();
    App::getInstance().addEventHandler(this);
#endif
#endif
    return 0;
}


int I2CClient::readData() {
    int read_len = 0;

#if CONN_TCP
    read_len = Client::readData();
#else
    int64_t curr_tick = SystemClock::uptimeMillis();
    if (curr_tick - mLastRecvTime < mRecvSpace) { return 1; }
    mLastRecvTime = curr_tick;


    if (mI2cOpenReq.type == I2C_TYPE_SW)
        read_len = i2c_mcu_read_sw(mI2cOpenReq.addr, mRSBuf->last, mI2cOpenReq.len);
    else
        read_len = i2c_mcu_read_hw(mI2cOpenReq.addr, mI2cOpenReq.reg, mRSBuf->last, mI2cOpenReq.len);
    if (read_len > 0) {
        mRSBuf->last += read_len;
    } else {
        LOGE("i2c recv error. result=%d", read_len);
        return 0;
    }
#endif

    return read_len;
}

int I2CClient::onRecvData() {
    int    count = 0;
    size_t data_len = mRSBuf->last - mRSBuf->pos;

    // iic数据包
    int64_t oldRecvCount = mRecvCount;
    onI2cData(mRSBuf->pos, 9);
    mRSBuf->pos = mRSBuf->start;
    mRSBuf->last = mRSBuf->pos;
    count = mRecvCount - oldRecvCount;

    return count;
}

void I2CClient::onStatusChange() {
#if CONN_TCP
    SocketClient::onStatusChange();
    if (isConn()) { sendHeart(); }
#else
    LOGE_IF(!isConn(), "errno=%d errstr=%s", errno, strerror(errno));
#endif
}

bool I2CClient::isTimeout(int out_time) {
#if CONN_TCP
    return SocketClient::isTimeout(out_time);
#endif
    return false;
}

int I2CClient::onI2cData(uint8_t* buf, int len) {
    int offset = 0;
    if (len > 0) { LOG(VERBOSE) << "len:" << len << " hex:" << hexstr(buf, len); }

    for (; offset < len;) {
        offset += mPacketBuff->add(mCurrRecv, buf + offset, len - offset);
        if (!mPacketBuff->complete(mCurrRecv)) { break; }
        LOG(VERBOSE) << "complete:" << hexstr(mCurrRecv->buf, mCurrRecv->len);

        mRecvCount++;

        if (mPacketBuff->check(mCurrRecv)) {
            mChkErrCount = 0;
            // 检查重复数据包
            if (!mPacketBuff->compare(mCurrRecv, mLastRecv) || checkDealData()) {
                LOG(DEBUG) << "new packet:" << mPacketBuff->str(mCurrRecv);
                IAck* ack = mPacketBuff->ack(mCurrRecv);
                g_objHandler->onCommand(ack);

                // 保存本次数据包
                mLastRecv->len = mCurrRecv->len;
                memcpy(mLastRecv->buf, mCurrRecv->buf, mCurrRecv->len);

                mLastSendTime = 0;
            }
        } else {
            mChkErrCount++;
            // LOGE("data len:%d", mCurrRecv->len);
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

    SocketClient::onTick();
#if CONN_TCP
    if (now_tick - mLastSndHeart >= HEART_TIME && isTimeout(HEART_TIME)) {
        mLastSndHeart = now_tick;
        sendHeart();
    }
#endif

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

bool I2CClient::isOk() {
    bool is_ok = isConn();
#if CONN_TCP
    return is_ok && mSerialOk;
#endif
    return is_ok;
}

int I2CClient::send(BuffData* ask) {

    if (!isConn()) {
        mPacketBuff->recycle(ask);
        return -1;
    }

    // 检查队列是否有相同的包还未发送
    for (auto it = mSendQueue.begin(); it != mSendQueue.end(); it++) {
        BuffData* dst = *it;
        if (mPacketBuff->compare(ask, dst)) {
            LOG(WARN) << "same package exists! str=" << mPacketBuff->str(ask);
            mPacketBuff->recycle(ask);
            return 1;
        }
    }

    mSendQueue.push_back(ask);

    return 0;
}

void I2CClient::sendTrans(BuffData* ask) {
#if CONN_TCP
    sendData(ask->buf, ask->len);
#else
    LOG(VERBOSE) << std::to_string(mI2cOpenReq.addr) << " iic send:" << hexstr(ask->buf, ask->len);
    if (mI2cOpenReq.type == I2C_TYPE_SW)
        i2c_mcu_write_sw(mI2cOpenReq.addr, ask->buf, ask->len);
    else
        i2c_mcu_write_hw(mI2cOpenReq.addr, mI2cOpenReq.reg, ask->buf, ask->len);
    mLastRecvTime = SystemClock::uptimeMillis();
#endif
}

void I2CClient::sendHeart() {
#if CONN_TCP
    uchar size = 20;
    char  buf[size];
    for (size_t i = 0; i < size; i++)
        buf[i] = i;
    sendData(buf, size);
    LOG(INFO) << "connect i2c. name=";
#endif
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
#if CONN_TCP
    if (!isConn()) return 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(mFd, &read_fds);

    struct timeval vtime;
    vtime.tv_sec = 0;
    vtime.tv_usec = 1000;
    int ready = select(mFd + 1, &read_fds, NULL, NULL, &vtime);
    if (ready < 0) {
        handleEvent(mFd, Looper::EVENT_INPUT, 0);
        return 0;
    }

    if (FD_ISSET(mFd, &read_fds)) {
        handleEvent(mFd, Looper::EVENT_INPUT, 0);
        return 1;
    }

#else
    int read_len = 0;
    if (mI2cOpenReq.type == I2C_TYPE_SW)
        read_len = i2c_mcu_read_sw(mI2cOpenReq.addr, mRSBuf->last, mI2cOpenReq.len);
    else
        read_len = i2c_mcu_read_hw(mI2cOpenReq.addr, mI2cOpenReq.reg, mRSBuf->last, mI2cOpenReq.len);

    LOGV("uart recv. len=%d", read_len);

    if (read_len > 0) {
        mRSBuf->last += read_len;
    } else {
        return 0;
    }
    onRecvData();
    return 1;
#endif

    return 0;
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
