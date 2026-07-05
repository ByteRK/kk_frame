/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/tcp/tcp_client.h
 * @Description: Raw TCP client
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "transport.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

/**
 * @brief 自动重连的 TCP 客户端原始字节通讯实现。
 *
 * 连接与接收在内部工作线程执行，事件经 AsyncTransport 投递到 init()
 * 所在线程的 Looper。心跳和应用层握手不属于本类职责。
 */
class TcpClient : public AsyncTransport {
public:
    /** @brief TCP 服务端地址及接收参数。 */
    struct Config {
        /** @brief 服务端 IP 地址或可解析的主机名。 */
        std::string host;
        /** @brief 服务端端口，必须大于 0。 */
        uint16_t port{ 0 };
        /** @brief 连接失败或断开后的重试间隔，单位毫秒，必须大于等于 0。 */
        int reconnectDelayMs{ 2000 };
        /** @brief 单次接收使用的缓存大小；为 0 时使用 4096 字节。 */
        size_t readBufferSize{ 4096 };
        /** @brief 一次完整发送的总超时，单位毫秒，必须大于 0。 */
        int sendTimeoutMs{ 1000 };
    };

public:
    /** @brief 使用主机和端口构造，其他参数采用默认值。 */
    TcpClient(const std::string& host, uint16_t port);
    /** @brief 使用完整配置构造。 */
    explicit TcpClient(const Config& config);
    ~TcpClient();

    /** @brief 校验地址并将当前线程 Looper 初始化为事件接收线程。 */
    int init() override;
    /** @brief 启动连接和接收线程；连接失败时按配置持续重试。 */
    bool start() override;
    /** @brief 中止解析、连接和重连等待，关闭套接字并释放事件分发器。 */
    void stop() override;
    /** @brief 与服务端 TCP 连接已经建立时返回 true。 */
    bool isConnected() const override;
    /** @brief 向当前服务端连接写入原始字节；id 会被忽略。 */
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

private:
    void threadLoop();
    int connectServer();
    void waitForReconnectDelay();
    void closeSocket();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);

private:
    Config mConfig;
    std::mutex mSendLock;
    mutable std::mutex mSocketLock;
    int mSock;
    std::thread mThread;
    std::atomic<bool> mRunning;
    std::atomic<bool> mConnected;
    std::mutex mStopLock;
    std::condition_variable mStopCondition;
};

#endif // !__TCP_CLIENT_H__
