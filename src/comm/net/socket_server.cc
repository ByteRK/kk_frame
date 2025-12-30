
#include "socket_server.h"
#include <cstring>
#include <cdlog.h>

#define AUTH_KEYS "cdroidServer"
#define SVR_PORT 8080

ServerClient::ServerClient(SocketServer *s, int fd)
: mSvr(s)
, Client(fd) { }

ServerClient::~ServerClient() { }

int  ServerClient::init() { return 0; }

void ServerClient::onTick() { }

int  ServerClient::onRecvData() {
    if (!mIsAuth) {
        if (strncmp((char *)mRSBuf->pos, AUTH_KEYS, sizeof(AUTH_KEYS) - 1) == 0) {
            mIsAuth = true;
            LOGI("client auth ok. fd=%d", mFd);
        }
    }
    mLastRcvTime = time(0);
    return 0;
}

void ServerClient::onStatusChange() {
    int oldFd = mFd;

    // base function
    Client::onStatusChange();

    // 释放客户端
    if (mStatus == ST_DISCONNECT) {
        mSvr->close(oldFd);
    }
}

/////////////////////////////////////////////////////
SocketServer::SocketServer() { }

SocketServer::~SocketServer() { }

int SocketServer::init() {
    int                serverSocket;
    struct sockaddr_in serverAddress;

    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        LOGE("Error in creating socket");
        return 1;
    }

    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // 设置服务器地址和端口
    int port                      = SVR_PORT;
    int eport                     = (SVR_PORT + 1000) / 1000 * 1000;
    LOGI("server port %d-%d", port, eport);
    for (; port <= eport; port++) {
        serverAddress.sin_port = htons(port);

        // 绑定服务器套接字到指定地址和端口
        if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            LOGE("Error in binding. port=%d err=%d errs=%s", port, errno, strerror(errno));
            continue;
        }

        break;
    }
    if (port > eport) {
        LOGE("all port bind fail");
        close(serverSocket);
        return 1;
    }

    // 监听传入连接请求
    if (listen(serverSocket, 5) < 0) {
        LOGE("Error in listening");
        close(serverSocket);
        return 1;
    }

    LOG(DEBUG) << "server listen ok. port=" << port << " socket=" << serverSocket;
    setFd(serverSocket);

    return 0;
}

int SocketServer::handleEvent(int fd, int events, void *data) {
    LOG(VERBOSE) << "server msg. fd=" << fd << " events=" << events;

    do {
        struct sockaddr_in clientAddress;
        socklen_t          clientAddressLength = sizeof(clientAddress);

        // 接受客户端连接
        int                clientSocket        = accept(fd, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSocket < 0) {
            LOGE("Error in accepting client connection");
            break;
        }

        // 将Socket设置为非阻塞模式
        int flags = fcntl(clientSocket, F_GETFL, 0);
        fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

        mClients.insert(std::make_pair(clientSocket, new ServerClient(this, clientSocket)));
        LOG(VERBOSE) << "accept socket client. socket=" << clientSocket << " size=" << mClients.size();

    } while (false);

    return 1;
}

void SocketServer::onTick() {
    int count = 0;
    for (auto it = mClients.begin(); it != mClients.end();) {
        ServerClient *client = it->second;
        if (client->isTimeout()) {
            delete client;
            it = mClients.erase(it);
            count++;
        } else {
            it++;
        }
    }
    if (count > 0) {
        LOG(DEBUG) << "free time out client. count=" << count;
    }
}

void SocketServer::close(int fd) {
    auto it = mClients.find(fd);
    if (it == mClients.end()) {
        LOGE("not find fd. fd=%d", fd);
        return;
    }

    LOGV("close fd. fd=%d", fd);
    delete it->second;
    mClients.erase(it);
}

int SocketServer::sendData(const char *data, int len) {
    int send_client_count = 0;
    for (auto &kv : mClients) {
        ServerClient *sc = kv.second;
        if (!sc->isAuthentication()) continue;
        sc->sendData(data, len);
        send_client_count++;
    }
    return send_client_count;
}

int SocketServer::sendData(const uint8_t *data, int len) { return sendData((const char *)data, len); }
