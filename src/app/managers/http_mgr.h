/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-08 22:48:56
 * @LastEditTime: 2026-04-09 01:01:54
 * @FilePath: /kk_frame/src/app/managers/http_mgr.h
 * @Description: Http 请求管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __HTTP_MGR_H__
#define __HTTP_MGR_H__

#include <core/looper.h>
#include <curl/curl.h>
#include <stdint.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define g_http HttpManager::getInstance()

class HttpManager {
public:
    typedef uint64_t RequestId;

    /* 支持的 HTTP 请求方法 */
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        PATCH,
        DELETE_,
        HEAD,
        CUSTOM,
    };

    /* 回调到主线程的事件类型 */
    enum class EventType {
        STARTED,
        PROGRESS,
        COMPLETED,
        CANCELLED,
    };

    /* 响应结果或阶段性响应数据 */
    struct Response {
        CURLcode curlCode;                        /* libcurl 返回码 */
        long httpStatusCode;                      /* HTTP 状态码 */
        std::string errorMessage;                 /* 错误描述 */
        std::vector<unsigned char> body;          /* 响应体，storeResponseBody=true 时有效 */
        std::vector<std::string> responseHeaders; /* 原始响应头 */
        std::string effectiveUrl;                 /* 重定向后的最终 URL */
        std::string outputFilePath;               /* 下载落盘路径 */
        double totalTimeSeconds;                  /* 总耗时 */
        curl_off_t downloadedBytes;               /* 当前已下载字节数 */
        curl_off_t downloadTotalBytes;            /* 总下载字节数 */
        curl_off_t uploadedBytes;                 /* 当前已上传字节数 */
        curl_off_t uploadTotalBytes;              /* 总上传字节数 */
        bool timedOut;                            /* 是否超时 */
        bool cancelled;                           /* 是否已取消 */

        Response();
        bool isSuccess() const;
    };

    /* 回调事件对象 */
    struct Event {
        EventType type;       /* 事件类型 */
        RequestId requestId;  /* 请求 ID */
        HttpMethod method;    /* 请求方法 */
        std::string url;      /* 原始 URL */
        std::string tag;      /* 业务标签 */
        void* opaque;         /* 业务透传指针 */
        Response response;    /* 当前事件的响应数据 */

        Event();
    };

    /*
     * 回调接口
     */
    class IEventListener {
    public:
        virtual ~IEventListener() { }
        virtual void onHttpEvent(const Event& event) = 0;
    };

    /* 单次请求描述对象 */
    struct Request {
        HttpMethod method;                /* HTTP 方法 */
        std::string url;                  /* 目标 URL */
        std::string tag;                  /* 业务标签，用于批量取消 */
        std::string customMethod;         /* method=CUSTOM 时的自定义方法名 */
        std::vector<std::string> headers; /* 自定义请求头 */
        std::string body;                 /* 内存请求体 */
        std::string uploadFilePath;       /* 上传文件路径 */
        std::string downloadFilePath;     /* 下载文件路径 */
        std::string contentType;          /* 仅便于业务记录或补默认头 */
        std::string caFilePath;           /* 自定义 CA 文件路径 */
        void* opaque;                     /* 业务透传指针 */
        IEventListener* listener;         /* 监听者，不能为空 */
        long connectTimeoutMs;            /* 连接超时 */
        long requestTimeoutMs;            /* 整体请求超时 */
        long lowSpeedTimeSeconds;         /* 低速持续时间限制 */
        long maxRecvSpeedBytesPerSec;     /* 最大下载速率 */
        long maxSendSpeedBytesPerSec;     /* 最大上传速率 */
        long lowSpeedLimitBytesPerSec;    /* 低速判定阈值 */
        bool followRedirects;             /* 是否自动跟随重定向 */
        bool verifyPeer;                  /* 是否校验证书 */
        bool storeResponseBody;           /* 是否把响应体缓存到内存 */
        bool emitProgress;                /* 是否派发进度事件 */
        long progressIntervalMs;          /* 进度回调最小间隔 */

        Request();

        static Request CreateGet(const std::string& url,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr);

        static Request CreatePost(const std::string& url,
            const std::string& body,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            const std::string& contentType = "application/json");

        static Request CreatePut(const std::string& url,
            const std::string& body,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            const std::string& contentType = "application/json");

        static Request CreateDownload(const std::string& url,
            const std::string& downloadFilePath,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr);

        static Request CreateUpload(const std::string& url,
            const std::string& uploadFilePath,
            IEventListener* listener,
            const std::string& tag = std::string(),
            void* opaque = nullptr,
            const std::string& contentType = "application/octet-stream");
    };

public:
    /* 获取全局唯一实例 */
    static HttpManager* getInstance();

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

    /* 主动释放线程、fd、curl 全局资源 */
    void shutdown();

    /* 当前单例是否已完成初始化 */
    bool isInitialized() const;

    /* 提交一个请求到队列，返回其唯一请求 ID */
    RequestId submit(const Request& request);

    /* 按请求 ID 取消单个请求 */
    bool cancel(RequestId requestId);

    /* 按 tag 批量取消请求，返回取消数量 */
    size_t cancelByTag(const std::string& tag);

    /* 取消当前所有待执行/执行中的请求 */
    void cancelAll();

    /* 查询队列状态 */
    size_t pendingCount() const;
    size_t runningCount() const;
    size_t requestCount() const;
    bool   contains(RequestId requestId) const;

private:
    struct RequestContext;
    struct MainThreadMessage;

private:
    HttpManager();
    ~HttpManager();

    HttpManager(const HttpManager&) = delete;
    HttpManager& operator=(const HttpManager&) = delete;

    static void initializeCurlGlobal();
    static void cleanupCurlGlobal();
    static void validateRequestOrDie(const Request& request);

    /*
     * 根据当前队列压力按需创建 worker
     * 调用时要求已持有 mRequestMutex
     */
    void ensureWorkerCapacityLocked();

    /*
     * worker 退出时统一收尾
     * 调用时要求已持有 mRequestMutex
     */
    void onWorkerExitLocked();

    /* eventfd 被主线程 Looper 唤醒后的回调入口 */
    static int onMainThreadWake(int fd, int events, void* context);

    /* 向主线程投递消息并唤醒 Looper */
    void wakeMainThread();
    void drainMainThreadMessages();
    void postEvent(const std::shared_ptr<RequestContext>& requestContext, EventType type);
    void postEvent(RequestContext* requestContext, EventType type);
    void postProgressEvent(RequestContext* requestContext,
        curl_off_t downloadTotal,
        curl_off_t downloadNow,
        curl_off_t uploadTotal,
        curl_off_t uploadNow);

    /* 工作线程主循环 */
    void workerLoop();

    /* 执行单个请求的完整流程 */
    void executeRequest(const std::shared_ptr<RequestContext>& requestContext);

    /* libcurl 回调 */
    static size_t onWrite(char* data, size_t size, size_t count, void* context);
    static size_t onHeader(char* data, size_t size, size_t count, void* context);
    static size_t onRead(char* buffer, size_t size, size_t count, void* context);
    static int onProgress(void* context,
        curl_off_t downloadTotal,
        curl_off_t downloadNow,
        curl_off_t uploadTotal,
        curl_off_t uploadNow);

private:
    /* 生命周期保护，防止重复 init/shutdown */
    mutable std::mutex mLifecycleMutex;

    /* 是否已经完成初始化 */
    std::atomic<bool> mInitialized;

    /* 主线程 Looper，用于将回调切回业务线程 */
    cdroid::Looper* mMainLooper;

    /* eventfd：工作线程通过它唤醒主线程处理回调 */
    int mWakeFd;

    /* 管理器是否处于停止流程中 */
    std::atomic<bool> mStopping;

    /* 全局递增请求 ID */
    std::atomic<RequestId> mNextRequestId;

    /* 请求队列锁与条件变量 */
    mutable std::mutex mRequestMutex;
    std::condition_variable mRequestCond;

    /* worker 全部退出时用于 shutdown 等待 */
    std::condition_variable mWorkerExitCond;

    /* 等待执行的请求队列 */
    std::deque<std::shared_ptr<RequestContext> > mPendingRequests;

    /* 正在执行的请求表，便于取消与查询 */
    std::unordered_map<RequestId, std::shared_ptr<RequestContext> > mRunningRequests;

    /* 仅作为并发上限，不等于当前已创建线程数 */
    size_t mMaxWorkerCount;

    /* worker 空闲自动退出时间，单位 ms；<=0 表示不自动退出 */
    long mWorkerIdleExitMs;

    /* 当前存活的 worker 数 */
    size_t mAliveWorkerCount;

    /* 当前处于空闲等待状态的 worker 数 */
    size_t mIdleWorkerCount;

    /* 主线程事件队列 */
    mutable std::mutex mMainThreadMutex;
    std::deque<MainThreadMessage> mMainThreadMessages;

    /* curl_global_init/cleanup 的跨实例引用计数 */
    static std::mutex sCurlGlobalMutex;
    static size_t sCurlGlobalRefCount;
};

#endif  // __HTTP_MGR_H__
