
#include "uart_client.h"
#include "cmd_handler.h"
#include <core/app.h>
#include <uart.h>

#ifdef DEBUG
#define CONN_TCP 1
#else
#define CONN_TCP 0
#endif

#define RECV_PACKET_TIME_SPACE 50  // epoll_wait 收包间隔ms
#define SEND_PACKET_TIME_SPACE 200 // 发包间隔ms

#define ADD_TCP_HEARD false

UartClient::UartClient(IPacketBuffer* ipacket, BufferType type, UartOpenReq& uartInfo, const std::string& ip,
    short port, int recv_space)
    : mPacketBuff(ipacket), mBufType(type), mUartInfo(uartInfo), mIp(ip), mPort(port), mRecvSpace(recv_space) {

    mLastSendTime = 0; // 最后一次发包时间
    mSendCount    = 0;
    mRecvCount    = 0;
    mChkErrCount  = 0;
    mLastRecvTime = SystemClock::uptimeMillis() + 5000;
    mLastRecv     = mPacketBuff->obtain(type);
    mCurrRecv     = mPacketBuff->obtain(type);
    mLastSndHeart = SystemClock::uptimeMillis();
    mSerialOk     = 0;
}

UartClient::~UartClient() {
    mPacketBuff->recycle(mLastRecv);
    mPacketBuff->recycle(mCurrRecv);

    for (BuffData* ask : mSendQueue) { mPacketBuff->recycle(ask); }
    mSendQueue.clear();
}

int UartClient::init() {

    LOGI("Uart: %s %d-%d-%d-%d-%C %p", mUartInfo.serialPort, mUartInfo.speed, mUartInfo.flow_ctrl, mUartInfo.databits,
        mUartInfo.stopbits, mUartInfo.parity, this);

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
    int uart_fd = UART0_Open(mUartInfo.serialPort);
    if (uart_fd <= 0) {
        LOGE("uart open fail. port=%s", mUartInfo.serialPort);
        return -1;
    }
    int result = UART0_Init(uart_fd, mUartInfo.speed, mUartInfo.flow_ctrl, mUartInfo.databits, mUartInfo.stopbits,
        mUartInfo.parity);
    if (result == FALSE) {
        LOGE("uart init fail. %d-%d-%d-%d-%C", mUartInfo.speed, mUartInfo.flow_ctrl, mUartInfo.databits,
            mUartInfo.stopbits, mUartInfo.parity);
        UART0_Close(uart_fd);
        return -2;
    }

    // 读取缓冲区积累的数据丢弃
    int read_len;
    while ((read_len = UART0_Recv(uart_fd, mRSBuf->pos, mRSBuf->end - mRSBuf->last)) >= mRSBuf->end - mRSBuf->last) {
        LOG(VERBOSE) << "fore-read data, give up. buf=" << hexstr(mRSBuf->pos, read_len);
    }

    // 读消息事件处理
    if (mRecvSpace < RECV_PACKET_TIME_SPACE) {
        setFd(uart_fd);
    } else {
        mFd     = uart_fd;
        mStatus = ST_CONNECTED;
        onStatusChange();
        App::getInstance().addEventHandler(this);
    }
#endif
#endif
    return 0;
}

bool UartClient::isOk() {
    bool is_ok = isConn();
#if CONN_TCP
    return is_ok && mSerialOk;
#endif
    return is_ok;
}

int UartClient::readData() {
    int read_len;

#if CONN_TCP
    read_len = Client::readData();
#else
    int64_t curr_tick = SystemClock::uptimeMillis();
    if (curr_tick - mLastRecvTime < mRecvSpace) { return 1; }
    mLastRecvTime = curr_tick;

    read_len = UART0_Recv(mFd, mRSBuf->last, mRSBuf->end - mRSBuf->last);
    if (read_len > 0) {
        mRSBuf->last += read_len;
    } else {
        LOGE("uart recv error. result=%d", read_len);
        return 0;
    }
#endif

    return read_len;
}

int UartClient::onRecvData() {
    int    count    = 0;
    size_t data_len = mRSBuf->last - mRSBuf->pos;

    // 串口数据包
    int64_t oldRecvCount = mRecvCount;
    onUartData(mRSBuf->pos, data_len);
    mRSBuf->pos  = mRSBuf->start;
    mRSBuf->last = mRSBuf->pos;
    count        = mRecvCount - oldRecvCount;

    return count;
}

int UartClient::onUartData(uchar* buf, int len) {
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
            if (1 || !mPacketBuff->compare(mCurrRecv, mLastRecv)) {
                LOG(VERBOSE) << "new packet:" << mPacketBuff->str(mCurrRecv);
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
            hexdump("uart recv data, check fail", mCurrRecv->buf, mCurrRecv->len, 100);
        }

        mCurrRecv->len = 0;
    }

    return 0;
}

void UartClient::onTick() {
    int64_t now_tick = SystemClock::uptimeMillis();

    static int64_t s_last_time = 0;
    if (now_tick - s_last_time >= 60000) {
        s_last_time = now_tick;
        LOG(DEBUG) << mUartInfo.serialPort << " packet info. send=" << mSendCount << " recv=" << mRecvCount
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

    // 上一个包接收超时，发送队列不为空
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

int UartClient::send(BuffData* ask) {

    if (!isConn()) {
        mPacketBuff->recycle(ask);
        return -1;
    }

    // 检查队列是否有相同的包还未发送
    for (auto it = mSendQueue.begin(); it != mSendQueue.end(); it++) {
        BuffData* dst = *it;
        if (mPacketBuff->compare(ask, dst)) {
            LOG(VERBOSE) << "same package exists! str=" << mPacketBuff->str(ask);
            mPacketBuff->recycle(ask);
            return 1;
        }
    }

    mSendQueue.push_back(ask);

    return 0;
}

void UartClient::onStatusChange() {
#if CONN_TCP
    SocketClient::onStatusChange();
    if (isConn()) { sendConn(); }
#else
    LOGE_IF(!isConn(), "errno=%d errstr=%s", errno, strerror(errno));
#endif
}

bool UartClient::isTimeout(int out_time /* = 0*/) {
#if CONN_TCP
    return SocketClient::isTimeout(out_time);
#endif
    return false;
}

void UartClient::sendConn() {
    char        buf[LEN_4K];
    UartHeader *head = (UartHeader *)buf;
    head->mcmd       = main_cmd_uart;
    head->scmd       = sub_cmd_uart_open_req;
    head->size       = sizeof(UartHeader);

    UartOpenReq* req = (UartOpenReq*)(buf + head->size);
    memcpy(req, &mUartInfo, sizeof(UartOpenReq));
    head->size += sizeof(UartOpenReq);

    sendData(buf, head->size);
    LOG(INFO) << "connect uart. name=" << req->serialPort;
}

void UartClient::sendTrans(BuffData* ask) {
#if CONN_TCP
#if ADD_TCP_HEARD
    char        buf[LEN_4K];
    UartHeader* head = (UartHeader*)buf;
    head->mcmd       = main_cmd_data;
    head->scmd       = sub_cmd_data_trans;
    head->size       = sizeof(UartHeader);

    memcpy(buf + head->size, ask->buf, ask->len);
    head->size += ask->len;

    LOG(VERBOSE) << mFd << " sock send:" << hexstr(ask->buf, ask->len);
    sendData(buf, head->size);
#else
    sendData(ask->buf, ask->len);
#endif
#else
    LOG(VERBOSE) << mFd << " uart send:" << hexstr(ask->buf, ask->len);
    UART0_Send(mFd, ask->buf, ask->len);
    mLastRecvTime = SystemClock::uptimeMillis();
#endif
}

void UartClient::sendHeart() {
#if CONN_TCP
    char        buf[LEN_4K];
    UartHeader* head = (UartHeader*)buf;
    head->mcmd       = main_cmd_data;
    head->scmd       = sub_cmd_heart_ask;
    head->size       = sizeof(UartHeader);

    int64_t tt = SystemClock::uptimeMillis();
    memcpy(buf + head->size, &tt, sizeof(int64_t));
    head->size += sizeof(int64_t);

    sendData(buf, head->size);
#endif
}

int UartClient::checkEvents() {
    int64_t now_time_tick = SystemClock::uptimeMillis();

    if (now_time_tick - mLastRecvTime >= mRecvSpace) {
        mLastRecvTime = now_time_tick;
        return 1;
    }

    return 0;
}

int UartClient::handleEvents() {
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
    int read_len = UART0_Recv(mFd, mRSBuf->last, mRSBuf->end - mRSBuf->last);

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

int UartClient::getRecvSpace() {
    return mRecvSpace;
}
