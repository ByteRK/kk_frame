
#include "client.h"
#include <cstring>
#include <cdlog.h>

Client::Client() { initData(); }

Client::Client(int fd) {
    initData();
    setFd(fd);
}

void Client::initData() {
    mFd = -1;
    mStatus = ST_DISCONNECT;
    mIsAuth = false;

    mRSBuf = (SockBuf*)calloc(1, sizeof(SockBuf) + LEN_4K);
    mRSBuf->start = mRSBuf->buf;
    mRSBuf->pos = mRSBuf->start;
    mRSBuf->last = mRSBuf->pos;
    mRSBuf->end = mRSBuf->start + LEN_4K;
}

Client::~Client() {
    closeFd();
    free(mRSBuf);
}

void Client::setFd(int fd) {
    if (fd <= 0) {
        LOGE("fd is error. fd=%d", fd);
        return;
    }

    LOG(VERBOSE) << "add client. fd=" << fd;

    mFd = fd;
    mStatus = ST_CONNECTED;

    // 限制收包间隔，则不使用底层消息回调
    if (getRecvSpace() <= 10) {
        Looper::getMainLooper()->addFd(mFd, 0, Looper::EVENT_INPUT, this, 0);
    }

    mLastRcvTime = SystemClock::uptimeMillis();
    mLastSndTime = mLastRcvTime;
    onStatusChange();
}

int Client::handleEvent(int fd, int events, void* data) {
    int free_len, read_len, deal_count;

    LOG(VERBOSE) << "fd=" << fd << " events=" << events << " data=" << data;
    // 可读事件
    if (events & Looper::EVENT_INPUT) {
        while (1) {
            free_len = mRSBuf->end - mRSBuf->last;
            read_len = readData();
            if (read_len <= 0) {
                if (errno == ENOENT) {
                    mStatus = ST_DISCONNECT;
                    onStatusChange();
                    return 0;
                }
                // EAGAIN
                break;
            }

            deal_count = onRecvData();
                if (deal_count < 0) {
                mStatus = ST_DISCONNECT;
                onStatusChange();
                return 0;
            }

            // 缓冲区未满
            if (read_len < free_len) break;

            // 有完整数据包
            if (deal_count > 0) break;
        }
    }

    return 1;
}

int Client::readData() {
    int read_len = recv(mFd, mRSBuf->last, mRSBuf->end - mRSBuf->last, MSG_NOSIGNAL);
    if (read_len > 0) {
        LOG(VERBOSE) << "fd=" << mFd << " data=" << (void*)mRSBuf->last << " len=" << read_len;
        mRSBuf->last += read_len;
        mLastRcvTime = SystemClock::uptimeMillis();
    }
    return read_len;
}

int Client::readFile() {
    int read_len = read(mFd, mRSBuf->last, mRSBuf->end - mRSBuf->last);
    if (read_len > 0) {
        LOG(VERBOSE) << "fd=" << mFd << " data=" << (void*)mRSBuf->last << " len=" << read_len;
        mRSBuf->last += read_len;
        mLastRcvTime = SystemClock::uptimeMillis();
    }
    return read_len;
}

bool Client::isConn() { return mStatus == ST_CONNECTED; }

int  Client::sendData(const char* data, int len) {
    if (!isConn()) return -1;
    LOG(VERBOSE) << "fd=" << mFd << " data=" << (void*)data << " len=" << len;
    mLastSndTime = SystemClock::uptimeMillis();
    return send(mFd, data, len, MSG_NOSIGNAL);
}

int  Client::sendData(const uint8_t* data, int len) { return sendData((const char*)data, len); }

void Client::onStatusChange() {
    if (mStatus == ST_DISCONNECT) {
        LOGE("disconnect... errno=%d err=%s fd=%d", errno, strerror(errno), mFd);
        mRSBuf->pos = mRSBuf->start;
        mRSBuf->last = mRSBuf->pos;
        closeFd();
    }
}

bool Client::isAuthentication() { return mIsAuth; }

bool Client::isTimeout(int out_time /* = 0*/) {
    int64_t tt = SystemClock::uptimeMillis();
    if (out_time <= 0) out_time = TIME_OUT;
    if (mLastRcvTime + out_time < tt) return true;
    if (mLastSndTime + out_time < tt) return true;
    return false;
}

void Client::closeFd() {
    if (mFd == -1) return;
    if (getRecvSpace() <= 10) {
        Looper::getMainLooper()->removeFd(mFd);
        close(mFd);
    }
    mFd = -1;
}

int Client::checkEvents() {
    return 0;
}

int Client::handleEvents() {
    LOGE("do not deal events...");
    return 0;
}

int Client::getRecvSpace() {
    return 0;
}
