/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:48:00
 * @LastEditTime: 2026-07-05 21:15:50
 * @FilePath: /kk_frame/src/comm/tcp/tcp_server.h
 * @Description: TCP通讯服务端
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

/// @brief Tcp通讯服务端 [带重连、一对多]
/// @note 基于 AsyncTransport 的 FD 模式实现
/// @note 每个客户端在连接期间拥有唯一 id，上层发送回复时必须传回对应 id
class TcpServer : public AsyncTransport {
public:
    /// @brief TCP 服务端配置
    struct Config {
        uint16_t port{ 0 };               // 本地监听端口
        int      backlog{ 8 };            // listen() 使用的待连接队列长度
        size_t   readBufferSize{ 4096 };  // 单客户端读取缓存大小，为 0 时使用 4096 字节
        int      sendTimeoutMs{ 1000 };   // 发送超时，单位毫秒
    };

private:
    Config             mConfig;              // 通讯配置
    int                mListenSock{ -1 };    // 监听套接字
    std::map<int, int> mClients;             // 客户端套接字与 id 映射
    int                mNextClientId{ 1 };   // 下一个客户端 id
    std::mutex         mSendLock;            // 发送锁
    mutable std::mutex mSocketLock;          // 套接字锁
    std::thread        mThread;              // 接收线程
    std::atomic<bool>  mRunning{ false };    // 是否正在运行

public:
    explicit TcpServer(uint16_t port);
    explicit TcpServer(const Config& config);
    ~TcpServer();

    int     init() override;
    bool    start() override;
    void    stop() override;
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;
    bool    isConnected() const override;

private:
    void    threadLoop();
    int     createListenSocket();
    void    closeAllSockets();
    ssize_t sendAll(int fd, const uint8_t* data, size_t len);

};

#endif // !__TCP_SERVER_H__
