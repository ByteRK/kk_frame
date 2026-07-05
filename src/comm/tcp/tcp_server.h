/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.h
 * @Description: Raw TCP server
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "transport.h"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

/**
 * @brief 支持多个客户端的 TCP 服务端原始字节通讯实现。
 *
 * 监听、接入和接收在内部工作线程执行，事件经 AsyncTransport 投递到 init() 所在线程。
 * 每个客户端在连接期间拥有唯一 id，上层发送回复时必须传回对应 id。
 */
class TcpServer : public AsyncTransport {
public:
    /** @brief TCP 监听及接收参数。 */
    struct Config {
        /** @brief 本地监听端口，必须大于 0。 */
        uint16_t port{ 0 };
        /** @brief listen() 使用的待连接队列长度。 */
        int backlog{ 8 };
        /** @brief 单个客户端单次接收使用的缓存大小；为 0 时使用 4096 字节。 */
        size_t readBufferSize{ 4096 };
        /** @brief 一次完整发送的总超时，单位毫秒，必须大于 0。 */
        int sendTimeoutMs{ 1000 };
    };

public:
    /** @brief 使用监听端口构造，其他参数采用默认值。 */
    explicit TcpServer(uint16_t port);
    /** @brief 使用完整配置构造。 */
    explicit TcpServer(const Config& config);
    ~TcpServer();

    /** @brief 校验端口并将当前线程 Looper 初始化为事件接收线程。 */
    int init() override;
    /** @brief 创建监听套接字并启动接入/接收线程。 */
    bool start() override;
    /** @brief 停止监听、关闭全部连接、等待线程退出并释放事件分发器。 */
    void stop() override;
    /** @brief 服务端正在监听时返回 true，不表示一定已有客户端连接。 */
    bool isConnected() const override;
    /**
     * @brief 向指定客户端写入原始字节。
     * @param id onConnected()/onRecv() 回调中提供的客户端标识，必须大于等于 0。
     */
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

private:
    void threadLoop();
    int createListenSocket();
    void closeAllSockets();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);

private:
    Config mConfig;
    int mListenSock;
    std::map<int, int> mClients;
    int mNextClientId;
    std::mutex mSendLock;
    mutable std::mutex mSocketLock;
    std::thread mThread;
    std::atomic<bool> mRunning;
};

#endif // !__TCP_SERVER_H__
