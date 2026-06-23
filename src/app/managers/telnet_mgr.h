/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-14
 * @FilePath: /kk_frame/src/app/managers/telnet_mgr.h
 * @Description: Telnet 访问管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TELNET_MGR_H__
#define __TELNET_MGR_H__

#include <core/looper.h>

#include <stdint.h>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "template/singleton.h"

#define g_telnet TelnetManager::instance()

class TelnetManager : public Singleton<TelnetManager> {
    friend Singleton<TelnetManager>;

public:
    typedef uint64_t RequestId;

    /* 回调到主线程的事件类型 */
    enum class EventType {
        STARTED,       /* 请求已进入 worker */
        CONNECTED,     /* TCP 已连接 */
        LOGGED_IN,     /* 登录流程已完成 */
        COMMAND_SENT,  /* 已发送一条命令 */
        DATA,          /* 收到 Telnet 数据片段 */
        COMPLETED,     /* 请求完成 */
        CANCELLED,     /* 请求取消 */
    };

    /* Telnet 执行结果或阶段性数据 */
    struct Response {
        int socketErrno;              /* socket / connect / recv / send 的 errno */
        std::string errorMessage;     /* 错误描述 */
        std::string output;           /* 累积输出，storeOutput=true 时有效 */
        std::string lastData;         /* DATA 事件最近一次收到的数据 */
        size_t sentBytes;             /* 已发送业务字节数，不含 Telnet 协商回复 */
        size_t receivedBytes;         /* 已接收原始字节数，含 Telnet 协商字节 */
        bool connected;               /* TCP 是否连接成功 */
        bool loggedIn;                /* 登录流程是否完成 */
        bool timedOut;                /* 是否超时 */
        bool cancelled;               /* 是否取消 */
        bool peerClosed;              /* 对端是否关闭连接 */
        bool outputTruncated;         /* output 是否因为 maxOutputBytes 被截断 */

        Response();
        bool isSuccess() const;
    };

    /* 回调事件对象 */
    struct Event {
        EventType type;       /* 事件类型 */
        RequestId requestId;  /* 请求 ID */
        std::string host;     /* 目标主机 */
        uint16_t port;        /* 目标端口 */
        std::string tag;      /* 业务标签 */
        void* opaque;         /* 业务透传指针 */
        size_t commandIndex;  /* 当前命令序号 */
        std::string command;  /* 当前命令 */
        Response response;    /* 当前事件的响应快照 */

        Event();
    };

    class IEventListener {
    public:
        virtual ~IEventListener() { }
        virtual void onTelnetEvent(const Event& event) = 0;
    };

    /* 单次 Telnet 会话请求 */
    struct Request {
        std::string host;                 /* 目标 IP 或域名 */
        uint16_t port;                    /* Telnet 端口，默认 23 */
        std::string tag;                  /* 业务标签，用于批量取消 */
        void* opaque;                     /* 业务透传指针 */
        IEventListener* listener;         /* 监听者，不能为空 */

        std::string username;             /* 登录用户名；为空表示不发送用户名 */
        std::string password;             /* 登录密码；为空表示不发送密码 */
        std::vector<std::string> commands;/* 登录后顺序执行的命令 */

        std::string loginPrompt;          /* 用户名提示符，默认 login: */
        std::string passwordPrompt;       /* 密码提示符，默认 Password: */
        std::string prompt;               /* 命令结束提示符，例如 #、$、>；为空时使用 idle 作为命令结束 */
        std::string lineEnding;           /* 发送行结尾，Telnet 一般使用 \r\n */

        long connectTimeoutMs;            /* TCP 连接超时 */
        long loginTimeoutMs;              /* 登录阶段总超时 */
        long commandTimeoutMs;            /* 单条命令等待输出超时 */
        long initialReadTimeoutMs;        /* 无登录时连接后读取 banner 的时间 */
        long readIdleTimeoutMs;           /* 无 prompt 时，多久没新数据认为当前阶段结束 */
        long sendTimeoutMs;               /* send 等待可写超时 */
        long dataIntervalMs;              /* DATA 事件最小间隔；0 表示每次收到数据都派发 */

        bool storeOutput;                 /* 是否累积输出 */
        bool emitData;                    /* 是否派发 DATA 事件 */
        bool replyTelnetNegotiation;      /* 是否对 Telnet 协商统一回复 WONT/DONT */
        bool tcpNoDelay;                  /* 是否设置 TCP_NODELAY */
        bool closeAfterCompleted;         /* 完成后是否主动关闭连接 */
        size_t maxOutputBytes;            /* output 最大缓存；0 表示不限制 */

        Request();

        static Request CreateCommand(const std::string& host,
            const std::string& command,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            uint16_t port = 23);

        static Request CreateCommands(const std::string& host,
            const std::vector<std::string>& commands,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            uint16_t port = 23);

        static Request CreateLoginCommand(const std::string& host,
            const std::string& username,
            const std::string& password,
            const std::string& command,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            uint16_t port = 23);

        static Request CreateLoginCommands(const std::string& host,
            const std::string& username,
            const std::string& password,
            const std::vector<std::string>& commands,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            uint16_t port = 23);
    };

protected:
    TelnetManager();
    ~TelnetManager();

public:
    /*
     * 初始化管理器
     * 1. 必须在 submit/cancel 等接口前调用；
     * 2. 不会预创建 worker，worker 在 submit 时按需创建；
     * 3. maxWorkerCount 仅表示允许创建的最大 worker 数；
     * 4. workerIdleExitMs > 0 时，worker 在空闲等待该时长后会自动退出；
     *    workerIdleExitMs <= 0 时，worker 会一直等待新任务，不自动退出；
     */
    void init(cdroid::Looper* mainLooper = nullptr,
        size_t maxWorkerCount = 0,
        long workerIdleExitMs = 0);

    /* 主动释放线程与 fd */
    void shutdown();

    bool isInitialized() const;

    /* 提交一个 Telnet 会话请求，返回其唯一请求 ID */
    RequestId submit(const Request& request);

    /* 按请求 ID 取消单个请求 */
    bool cancel(RequestId requestId);

    /* 按 tag 批量取消请求，返回取消数量 */
    size_t cancelByTag(const std::string& tag);

    /* 取消当前所有待执行/执行中的请求 */
    void cancelAll();

    size_t pendingCount() const;
    size_t runningCount() const;
    size_t requestCount() const;
    bool contains(RequestId requestId) const;

private:
    struct RequestContext;
    struct MainThreadMessage;

    enum class ReadStatus {
        OK,
        TIMEOUT,
        CANCELLED,
        CLOSED,
        ERROR,
    };

private:
    static void validateRequestOrDie(const Request& request);
    static int onMainThreadWake(int fd, int events, void* context);

    void ensureWorkerCapacityLocked();
    void onWorkerExitLocked();

    void wakeMainThread();
    void drainMainThreadMessages();
    void postEvent(const std::shared_ptr<RequestContext>& requestContext, EventType type);
    void postEvent(RequestContext* requestContext, EventType type);
    void postDataEvent(RequestContext* requestContext, const std::string& data);

    void workerLoop();
    void executeRequest(const std::shared_ptr<RequestContext>& requestContext);

    bool connectSocket(RequestContext* requestContext);
    bool doLogin(RequestContext* requestContext);
    bool doCommands(RequestContext* requestContext);

    bool sendLine(RequestContext* requestContext, const std::string& line, bool countAsBusinessBytes);
    bool sendRaw(RequestContext* requestContext, const char* data, size_t size, long timeoutMs, bool countAsBusinessBytes);

    ReadStatus readUntil(RequestContext* requestContext,
        const std::vector<std::string>& stopTokens,
        long totalTimeoutMs,
        long idleTimeoutMs,
        bool requireStopToken);

    void appendIncomingData(RequestContext* requestContext, const char* data, size_t size);
    void appendOutput(RequestContext* requestContext, const std::string& data);
    bool hasAnyStopToken(RequestContext* requestContext, const std::vector<std::string>& stopTokens) const;

    static int64_t nowMs();
    static bool setNonBlocking(int fd, bool nonBlocking);
    static void closeContextSocket(RequestContext* requestContext);
    static bool isLoginRequired(const Request& request);
    static std::vector<std::string> makePromptTokens(const Request& request);
    static const char* readStatusToString(ReadStatus status);

private:
    mutable std::mutex mLifecycleMutex;
    std::atomic<bool> mInitialized;
    cdroid::Looper* mMainLooper;
    int mWakeFd;
    std::atomic<bool> mStopping;
    std::atomic<RequestId> mNextRequestId;

    mutable std::mutex mRequestMutex;
    std::condition_variable mRequestCond;
    std::condition_variable mWorkerExitCond;
    std::deque<std::shared_ptr<RequestContext> > mPendingRequests;
    std::unordered_map<RequestId, std::shared_ptr<RequestContext> > mRunningRequests;

    size_t mMaxWorkerCount;
    long mWorkerIdleExitMs;
    size_t mAliveWorkerCount;
    size_t mIdleWorkerCount;

    mutable std::mutex mMainThreadMutex;
    std::deque<MainThreadMessage> mMainThreadMessages;
};

#endif // __TELNET_MGR_H__
