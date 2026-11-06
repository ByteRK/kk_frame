/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-08 22:48:56
 * @LastEditTime: 2026-11-06 09:24:58
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

#include <stdint.h>
#include <atomic>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "template/singleton.h"

#include "macro_define.h"
#include "event_fd.h"

#if ENABLED(CURL)
#include <curl/curl.h>
#include <curl/multi.h>
#else
typedef int   CURLcode;
typedef long  curl_off_t;
typedef void* curl_slist;
typedef void* CURL;
typedef void* CURLM;
typedef void* CURLSH;
#define CURL_ERROR_SIZE 1

/**
 * 非 CURL 构建下的错误码占位，数值与 libcurl 保持一致；
 * 仅在此处定义「在 #if ENABLED(CURL) 之外仍被引用」的宏，
 * 新增此类引用时务必同步补充，否则会导致编译失败
**/
#define CURLE_OK          0  // 无错误
#define CURLE_WRITE_ERROR 23 // 写入数据失败（下载临时文件落盘失败时使用）
#endif

#define g_http HttpManager::instance() // 单例快捷访问

/// @brief HTTP 请求管理器
class HttpManager : public Singleton<HttpManager> {
    friend Singleton<HttpManager>;

public:
    typedef uint64_t RequestId; // 请求唯一 ID

    /// @brief 支持的 HTTP 请求方法
    enum class HttpMethod {
        GET,     // GET 请求
        POST,    // POST 请求
        PUT,     // PUT 请求
        PATCH,   // PATCH 请求
        DELETE_, // DELETE 请求
        HEAD,    // HEAD 请求
        CUSTOM,  // 自定义请求（需指定 customMethod）
    };

    /// @brief 回调到主线程的事件类型
    enum class EventType {
        STARTED,   // 请求开始
        PROGRESS,  // 请求进度
        COMPLETED, // 请求完成
        CANCELLED, // 请求取消
    };

    /// @brief 响应数据
    struct Response {
        CURLcode                   curlCode{ CURLE_OK };    // curl 错误码
        long                       httpStatusCode{ 0 };     // HTTP 状态码
        std::string                errorMessage;            // 错误信息
        bool                       timedOut{ false };       // 是否超时
        bool                       cancelled{ false };      // 是否已取消

        std::vector<unsigned char> body;                    // 响应体（storeResponseBody 时填充）
        std::vector<std::string>   responseHeaders;         // 响应头
        std::string                effectiveUrl;            // 最终 URL（含重定向）
        std::string                outputFilePath;          // 下载落盘路径

        double                     totalTimeSeconds{ 0.0 }; // 总耗时（s）
        curl_off_t                 downloadedBytes{ 0 };    // 已下载字节数
        curl_off_t                 downloadTotalBytes{ 0 }; // 待下载总字节数
        curl_off_t                 uploadedBytes{ 0 };      // 已上传字节数
        curl_off_t                 uploadTotalBytes{ 0 };   // 待上传总字节数

        Response() = default;

        /// @brief 是否成功（未取消、curl 无错、HTTP 2xx/3xx）
        /// @note httpStatusCode 为 0 视为成功（无 HTTP 响应可判断）
        bool isSuccess() const;
    };

    /// @brief 回调事件对象
    struct Event {
        EventType   type{ EventType::COMPLETED }; // 事件类型
        RequestId   requestId{ 0 };               // 请求 ID
        HttpMethod  method{ HttpMethod::GET };    // 请求方法
        std::string url;                          // 请求 URL
        std::string tag;                          // 业务标签
        void*       opaque{ nullptr };            // 业务透传指针
        Response    response;                     // 响应快照

        Event() = default;
    };

    /// @brief HTTP 请求回调基类
    /// @note 自带回调生命周期管理：listener 析构后不会再收到事件
    class IEventListener {
    private:
        std::shared_ptr<std::atomic<bool>> mAliveToken; // 监听者存活标记
    public:
        IEventListener();
        virtual ~IEventListener();
        virtual void onHttpEvent(const Event& event) = 0;
    public:
        /// @brief 获取存活标记
        /// @note 用于 HttpManager 内部判断 listener 是否已析构
        const std::shared_ptr<std::atomic<bool>>& aliveToken() const { return mAliveToken; }
    };

    /// @brief HTTP 请求对象
    struct Request {
        HttpMethod               method{ HttpMethod::GET };      // 请求方法
        std::string              url;                            // 请求 URL
        std::string              customMethod;                   // CUSTOM 时的方法名

        std::vector<std::string> headers;                        // 请求头
        std::string              contentType;                    // 内容类型（Create* 时自动补 Content-Type 头）
        std::string              body;                           // 内存请求体

        std::string              uploadFilePath;                 // 上传文件路径
        std::string              downloadFilePath;               // 下载目录或文件路径（CreateDownload 时自动补 URL 文件名）
        std::string              caFilePath;                     // CA 证书路径

        IEventListener*          listener{ nullptr };            // 事件回调（不可为空）
        void*                    opaque{ nullptr };              // 业务透传指针
        std::string              tag;                            // 业务标签（用于批量取消）

        long                     connectTimeoutMs{ 5000 };       // 连接超时（ms）
        long                     requestTimeoutMs{ 30000 };      // 请求总超时（ms）
        int                      maxRetry{ 0 };                  // 网络失败重试次数（仅 GET/HEAD 生效）
        long                     progressIntervalMs{ 100 };      // 进度回调间隔（ms）

        long                     lowSpeedTimeSeconds{ 0 };       // 低速持续超时（s）
        long                     lowSpeedLimitBytesPerSec{ 0 };  // 低速阈值（B/s）
        long                     maxRecvSpeedBytesPerSec{ 0 };   // 下载限速（B/s）
        long                     maxSendSpeedBytesPerSec{ 0 };   // 上传限速（B/s）

        bool                     followRedirects{ true };        // 自动跟随重定向
        bool                     verifyPeer{ true };             // 校验证书
        bool                     storeResponseBody{ true };      // 缓存响应体到内存
        bool                     emitProgress{ true };           // 派发进度事件

        Request() = default;

        static Request CreateGet(const std::string& url,
            IEventListener*    listener,
            const std::string& tag = std::string(),
            void*              opaque = nullptr);

        static Request CreatePost(const std::string& url,
            const std::string& body,
            IEventListener*    listener,
            const std::string& tag = std::string(),
            void*              opaque = nullptr,
            const std::string& contentType = "application/json");

        static Request CreatePut(const std::string& url,
            const std::string& body,
            IEventListener*    listener,
            const std::string& tag = std::string(),
            void*              opaque = nullptr,
            const std::string& contentType = "application/json");

        static Request CreateDownload(const std::string& url,
            const std::string& downloadFilePath,
            IEventListener*    listener,
            const std::string& tag = std::string(),
            void*              opaque = nullptr);

        static Request CreateUpload(const std::string& url,
            const std::string& uploadFilePath,
            IEventListener*    listener,
            const std::string& tag = std::string(),
            void*              opaque = nullptr,
            const std::string& contentType = "application/octet-stream");

    private:
        /// @brief 解析下载落盘路径
        /// @param url 下载地址
        /// @param downloadPath 目标路径（已存在的目录会自动补 URL 文件名）
        /// @return 目录场景返回 <dir>/<文件名>；非目录原样返回；
        ///         无法从 URL 取得安全文件名、或结果逃出目标目录时返回空串
        /// @note 文件名取自 UrlUtils::fileName（已拒绝 %2F/%2E%2E 解码穿越），
        ///       此处再用 PathUtils::isSubPath 兜底
        static std::string resolveDownloadPath(const std::string& url, const std::string& downloadPath);
    };

private:
    struct RequestContext;    // 请求上下文
    struct MainThreadMessage; // 主线程消息
    struct RetryEntry;        // 重试记录

private:
    typedef std::shared_ptr<RequestContext>           RequestPtr;  // 请求上下文智能指针
    typedef std::unordered_map<RequestId, RequestPtr> RequestMap;  // 请求上下文表（按 RequestId 索引）

protected:
    HttpManager();
    ~HttpManager();

public:
    /// @brief 初始化管理器
    /// @param mainLooper 主线程 Looper（为空时取当前线程 Looper）
    /// @param maxConcurrent 最大并发请求数（0 表示默认值 4）
    /// @param pollTimeoutMs 轮询超时（0 表示默认值 1000 ms）
    /// @note 必须在 submit / cancel 之前调用，初始化失败直接终止进程
    void      init(cdroid::Looper* mainLooper = nullptr, size_t maxConcurrent = 0, long pollTimeoutMs = 0);

    /// @brief 关闭管理器，释放线程、fd 与 curl 资源
    void      shutdown();

    /// @brief 是否已初始化
    bool      isInitialized() const;

    /// @brief 提交请求到待执行队列
    /// @return 请求 ID（0 表示未启用 curl，提交失败）
    RequestId submit(const Request& request);

    /// @brief 按请求 ID 取消
    /// @return true 已受理；false 请求不存在（已完成/已取消）
    bool      cancel(RequestId requestId);

    /// @brief 按 tag 批量取消
    /// @return 受理的请求数量；tag 为空时返回 0
    size_t    cancelByTag(const std::string& tag);

    /// @brief 取消全部待执行、执行中与重试中的请求
    void      cancelAll();

    /// @brief 待执行请求数量
    size_t    pendingCount() const;

    /// @brief 执行中请求数量
    size_t    runningCount() const;

    /// @brief 队列中的请求总数（待执行 + 执行中 + 重试中）
    size_t    requestCount() const;

    /// @brief 请求是否仍在队列中（不含已完成）
    bool      contains(RequestId requestId) const;

private:
    /* 全局资源与参数校验 */
    static void   initializeCurlGlobal();
    static void   cleanupCurlGlobal();
    static void   validateRequestOrDie(const Request& request);

    /* 线程唤醒与事件派发（回调统一切回主线程） */
    static int    onMainThreadWake(int fd, int events, void* context);
    void          wakeMainThread();
    void          wakeIoThread();
    void          drainMainThreadMessages();
    void          postEvent(const RequestPtr& requestContext, EventType type);
    void          postEvent(RequestContext* requestContext, EventType type);
    void          postProgressEvent(RequestContext* requestContext, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow);

    /* I/O 线程主循环与调度 */
    void          ioLoop();
    void          processPendingOperations();
    void          processCompleted();
    void          processRetries();

    /* 单请求执行流程 */
    void          startRequest(const RequestPtr& requestContext);
    CURL*         buildEasy(const RequestPtr& requestContext);
    void          handleCompletion(const RequestPtr& requestContext, CURL* easy, CURLcode result);
    void          finalizeDownload(RequestContext* requestContext, bool keepFile);
    static bool   isRetryable(CURLcode code, long httpStatusCode);

    /* libcurl 回调 */
    static size_t onWrite(char* data, size_t size, size_t count, void* context);
    static size_t onHeader(char* data, size_t size, size_t count, void* context);
    static size_t onRead(char* buffer, size_t size, size_t count, void* context);
    static int    onProgress(void* context, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow);

private:
    /* 生命周期 */
    mutable std::mutex            mLifecycleMutex;        // 生命周期锁（防重复 init/shutdown）
    std::atomic<bool>             mInitialized{ false };  // 是否已初始化
    std::atomic<bool>             mStopping{ false };     // 是否正在停止

    /* 主线程回调 */
    cdroid::Looper*               mMainLooper{ nullptr }; // 主线程 Looper（回调线程回归）
    EventFd                       mMainWakeFd;            // 主线程唤醒 fd
    mutable std::mutex            mMainThreadMutex;       // 事件队列锁
    std::deque<MainThreadMessage> mMainThreadMessages;    // 待派发事件队列

    /* I/O 线程与 curl */
    std::thread                   mIoThread;              // I/O 线程
    EventFd                       mIoWakeFd;              // I/O 线程唤醒 fd
    CURLM*                        mMulti{ nullptr };      // curl_multi 句柄
    CURLSH*                       mShare{ nullptr };      // 连接池句柄（复用 DNS / keep-alive）
    std::recursive_mutex          mShareMutex;            // 连接池锁（递归锁，避免同线程重入死锁）

    /* 请求表 */
    std::atomic<RequestId>        mNextRequestId{ 1 };    // 请求 ID 计数器
    mutable std::mutex            mRequestMutex;          // 请求表锁
    std::deque<RequestPtr>        mPendingRequests;       // 待执行队列
    RequestMap                    mActiveRequests;        // 执行中请求表（已加入 multi）
    std::deque<RetryEntry>        mRetryQueue;            // 重试队列（按到期时间排序）

    /* 运行参数 */
    size_t                        mMaxConcurrent{ 4 };    // 并发上限（同时在 multi 中的请求数）
    long                          mPollTimeoutMs{ 1000 }; // curl_multi_wait 轮询超时（ms）

    /* 运行期统计 */
    std::atomic<size_t>           mTotalSubmitted{ 0 };   // 总提交数
    std::atomic<size_t>           mTotalCompleted{ 0 };   // 总完成数
    std::atomic<size_t>           mTotalCancelled{ 0 };   // 总取消数
    std::atomic<size_t>           mTotalRetried{ 0 };     // 总重试数

    /* curl 全局资源引用计数 */
    static std::mutex             sCurlGlobalMutex;       // curl 全局资源锁
    static size_t                 sCurlGlobalRefCount;    // curl 全局资源引用计数
};

#endif // __HTTP_MGR_H__
