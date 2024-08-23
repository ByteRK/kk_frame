#include "socket_client.h"
#include <comm_func.h>

#define RPORT_LEN 0 // 端口往后顺序找的范围
#define CONN_TIMEOUT 5
#define CONN_SPACE 5

SocketClient::SocketClient() {
    mIp           = "127.0.0.1";
    mPort         = 19090;
    mRPort        = mPort + RPORT_LEN;
    mLastConnTime = 0;
    mSockId       = -1;
    mInit         = false;
}

SocketClient::~SocketClient() {}

int SocketClient::init(const char *ip, ushort port) {
    if (mInit) return 0;
    if (ip) mIp = ip;
    if (port > 0) mPort = port;
    mRPort = mPort + RPORT_LEN;
    mInit  = true;
    return 0;
}

void SocketClient::onTick() {
    if (!mInit) return;

    int64_t cur_time = SystemClock::currentTimeSeconds();

    if (isConn()) {
        if (isTimeout()) {
            LOG(ERROR) << "Conn time out, close it. snd=" << mLastSndTime << " rcv=" << mLastRcvTime;
            mStatus = ST_DISCONNECT;
            onStatusChange();
        }
    } else {
        if (mStatus == ST_CONNECTING) {
            onCheckConnecting();
        } else if (cur_time - mLastConnTime >= CONN_SPACE) {
            mLastConnTime = cur_time;
            connectServer();
        }
    }
}

bool SocketClient::connectServer() {
    int                sockfd, ret;
    struct sockaddr_in server_addr;

    if (mStatus != ST_DISCONNECT) { return true; }

    // 创建Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOGE("Error in creating socket, error=%d", errno);
        return false;
    }

    // 将Socket设置为非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (-1 == fcntl(sockfd, F_SETFL, flags | O_NONBLOCK)) {
        LOG(ERROR) << " set socket nonblock error:" << errno;
        close(sockfd);
        return false;
    }

    // 设置服务器地址和端口
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(mPort);
    server_addr.sin_addr.s_addr = inet_addr(mIp.c_str());

    LOG(VERBOSE) << "ip=" << mIp << " port=" << mPort << " ptr=" << this;

    // 连接到服务器
    mLastConnTime = SystemClock::currentTimeSeconds();
    ret           = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            LOGE("Error in connecting to server. port=%d err=%d errmsg=%s", mPort, errno, strerror(errno));
            close(sockfd);
            if (++mPort > mRPort) { mPort = mRPort - RPORT_LEN; }
            return false;
        }
        LOGV("connecting... fd=%d", sockfd);
        mSockId = sockfd;
        mStatus = ST_CONNECTING;
        return true;
    }

    LOG(DEBUG) << "client connect ok. ip=" << mIp << " port=" << mPort << " fd=" << sockfd;
    setFd(sockfd);

    return true;
}

bool SocketClient::onCheckConnecting() {
    int err_ret = 0;
    int result  = 0;

    if (mStatus != ST_CONNECTING || mSockId <= 0) {
        LOGE("do not check connecting... fd=%d", mSockId);
        return false;
    }

    int64_t now_seconds = SystemClock::currentTimeSeconds();
    if (now_seconds - mLastConnTime > CONN_TIMEOUT) {
        LOG(VERBOSE) << "async connect timeout. conn_time=" << mLastConnTime;
        err_ret = -100;
        goto connect_over;
        return true;
    }

    do {
        fd_set writeset;
        FD_ZERO(&writeset);
        FD_SET(mSockId, &writeset);

        // 可以利用tv_sec和tv_usec做更小精度的超时控制
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 10000;

        result = select(mSockId + 1, nullptr, &writeset, nullptr, &tv);
        LOGV("async connect result:%d", result);
        if (result <= 0) {
            LOG(VERBOSE) << "async connect err:" << errno << ", str:" << strerror(errno);
            err_ret = -1;
            break;
        }

        // 检查socket 错误信息
        int32_t   temperr    = 0;
        socklen_t temperrlen = sizeof(temperr);
        if (-1 == getsockopt(mSockId, SOL_SOCKET, SO_ERROR, (void *)&temperr, &temperrlen)) {
            LOG(VERBOSE) << "async connect...getsockopt err:" << errno << ", str:" << strerror(errno);
            err_ret = -2;
            break;
        }
        if (0 != temperr) {
            LOG(VERBOSE) << "async connect...getsockopt temperr:" << temperr << ", str:" << strerror(temperr);
            err_ret = -3;
            break;
        }

        LOG(INFO) << "client connect ok. ip=" << mIp << " port=" << mPort << " fd=" << mSockId;
        setFd(mSockId);

    } while (false);

connect_over:
    if (err_ret < 0) {
        LOGV("connecting error. result=%d ip=%s port=%d", err_ret, mIp.c_str(), mPort);
        close(mSockId);
        mSockId = -1;
        mStatus = ST_DISCONNECT;
        if (++mPort > mRPort) { mPort = mRPort - RPORT_LEN; }
    }

    return true;
}

int SocketClient::onRecvData() {
    LOG(DEBUG) << "data len is " << mRSBuf->last - mRSBuf->pos;
    return 0;
}

void SocketClient::onStatusChange() {
    if (mStatus == ST_DISCONNECT) { mSockId = -1; }
    Client::onStatusChange();
}
