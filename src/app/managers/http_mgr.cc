/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-08 22:48:56
 * @LastEditTime: 2026-11-03 18:13:02
 * @FilePath: /kk_frame/src/app/managers/http_mgr.cc
 * @Description: Http 请求管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "http_mgr.h"

#include <core/systemclock.h>
#include <cdlog.h>

#include <sys/stat.h>
#include <unistd.h>

#include "string_utils.h"
#include "system_utils.h"
#include "url_utils.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

std::mutex HttpManager::sCurlGlobalMutex;        // curl 全局资源锁
size_t     HttpManager::sCurlGlobalRefCount = 0; // curl 全局资源引用计数

/// @brief HTTP 请求回调基类构造
HttpManager::IEventListener::IEventListener()
    : mAliveToken(std::make_shared<std::atomic<bool>>(true)) { }

/// @brief HTTP 请求回调基类析构
HttpManager::IEventListener::~IEventListener() {
    if (mAliveToken) {
        mAliveToken->store(false);
    }
}

namespace {

    /// @brief 事件类型名（仅用于日志）
    /// @note 事件派发不受 ENABLE_CURL 影响，故置于宏外
    const char* eventName(HttpManager::EventType type) {
        switch (type) {
        case HttpManager::EventType::STARTED:   return "STARTED";
        case HttpManager::EventType::PROGRESS:  return "PROGRESS";
        case HttpManager::EventType::COMPLETED: return "COMPLETED";
        case HttpManager::EventType::CANCELLED: return "CANCELLED";
        }
        return "UNKNOWN";
    }

} // namespace

#if ENABLED(CURL)
namespace {

    /// @brief 请求方法名（仅用于日志）
    const char* methodName(HttpManager::HttpMethod method) {
        switch (method) {
        case HttpManager::HttpMethod::GET:     return "GET";
        case HttpManager::HttpMethod::POST:    return "POST";
        case HttpManager::HttpMethod::PUT:     return "PUT";
        case HttpManager::HttpMethod::PATCH:   return "PATCH";
        case HttpManager::HttpMethod::DELETE_: return "DELETE";
        case HttpManager::HttpMethod::HEAD:    return "HEAD";
        case HttpManager::HttpMethod::CUSTOM:  return "CUSTOM";
        }
        return "UNKNOWN";
    }

} // namespace

/// @brief CURL 共享锁的加锁回调
static void httpShareLock(CURL* /*handle*/, curl_lock_data /*data*/, curl_lock_access /*access*/, void* userptr) {
    static_cast<std::recursive_mutex*>(userptr)->lock();
}

/// @brief CURL 共享锁的解锁回调
static void httpShareUnlock(CURL* /*handle*/, curl_lock_data /*data*/, void* userptr) {
    static_cast<std::recursive_mutex*>(userptr)->unlock();
}
#endif

/// @brief 判断响应是否成功
/// @return true 未取消、curl 无错且 HTTP 为 2xx/3xx（状态码为 0 也视为成功）
bool HttpManager::Response::isSuccess() const {
    if (cancelled) {
        return false;
    }
    if (curlCode != CURLE_OK) {
        return false;
    }
    if (httpStatusCode == 0) {
        return true;
    }
    return httpStatusCode >= 200 && httpStatusCode < 400;
}

/// @brief 创建 GET 请求对象
/// @param url 请求地址
/// @param listener 请求回调
/// @param tag 请求标签
/// @param opaque 业务透传指针
/// @return 请求对象
HttpManager::Request HttpManager::Request::CreateGet(const std::string& url,
    IEventListener*    listener,
    const std::string& tag,
    void*              opaque) {
    Request request;
    request.method = HttpMethod::GET;
    request.url = url;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.maxRetry = 2;
    return request;
}

/// @brief 创建 POST 请求对象
/// @param url 请求地址
/// @param body 请求体
/// @param listener 请求回调
/// @param tag 请求标签
/// @param opaque 业务透传指针
/// @param contentType 内容类型
/// @return 请求对象
HttpManager::Request HttpManager::Request::CreatePost(const std::string& url,
    const std::string& body,
    IEventListener*    listener,
    const std::string& tag,
    void*              opaque,
    const std::string& contentType) {
    Request request;
    request.method = HttpMethod::POST;
    request.url = url;
    request.body = body;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.contentType = contentType;
    if (!contentType.empty()) {
        request.headers.push_back(std::string("Content-Type: ") + contentType);
    }
    return request;
}

/// @brief 创建 PUT 请求对象
/// @param url 请求地址
/// @param body 请求体
/// @param listener 请求回调
/// @param tag 请求标签
/// @param opaque 业务透传指针
/// @param contentType 内容类型
/// @return 请求对象
HttpManager::Request HttpManager::Request::CreatePut(const std::string& url,
    const std::string& body,
    IEventListener*    listener,
    const std::string& tag,
    void*              opaque,
    const std::string& contentType) {
    Request request;
    request.method = HttpMethod::PUT;
    request.url = url;
    request.body = body;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.contentType = contentType;
    if (!contentType.empty()) {
        request.headers.push_back(std::string("Content-Type: ") + contentType);
    }
    return request;
}

/// @brief 创建下载请求对象
/// @param url 请求地址
/// @param downloadFilePath 下载目录或文件路径（传目录时自动补 URL 文件名）
/// @param listener 请求回调
/// @param tag 请求标签
/// @param opaque 业务透传指针
/// @return 请求对象
HttpManager::Request HttpManager::Request::CreateDownload(const std::string& url,
    const std::string& downloadFilePath,
    IEventListener*    listener,
    const std::string& tag,
    void*              opaque) {
    Request request;
    request.method = HttpMethod::GET;
    request.url = url;
    request.downloadFilePath = UrlUtils::resolveDownloadPath(url, downloadFilePath);
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.storeResponseBody = false;
    request.maxRetry = 2;
    return request;
}

/// @brief 创建上传请求对象
/// @param url 请求地址
/// @param uploadFilePath 上传文件路径
/// @param listener 请求回调
/// @param tag 请求标签
/// @param opaque 业务透传指针
/// @param contentType 内容类型
/// @return 请求对象
HttpManager::Request HttpManager::Request::CreateUpload(const std::string& url,
    const std::string& uploadFilePath,
    IEventListener*    listener,
    const std::string& tag,
    void*              opaque,
    const std::string& contentType) {
    Request request;
    request.method = HttpMethod::PUT;
    request.url = url;
    request.uploadFilePath = uploadFilePath;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.contentType = contentType;
    if (!contentType.empty()) {
        request.headers.push_back(std::string("Content-Type: ") + contentType);
    }
    return request;
}

/// @brief HTTP 请求上下文
struct HttpManager::RequestContext {
    Request                            request;                           // 原始请求
    RequestId                          requestId{ 0 };                    // 请求 ID
    HttpManager*                       owner{ nullptr };                  // 所属管理器
    CURL*                              easy{ nullptr };                   // easy 句柄（仅运行期有效）
    int                                retryCount{ 0 };                   // 已重试次数
    std::atomic<bool>                  cancelRequested{ false };          // 是否已请求取消
    std::shared_ptr<std::atomic<bool>> listenerAlive;                     // 监听者存活标记快照
    size_t                             bodyReadOffset{ 0 };               // body 读取偏移
    FILE*                              uploadFile{ nullptr };             // 上传文件句柄
    curl_off_t                         uploadFileSize{ 0 };               // 上传文件大小
    FILE*                              downloadFile{ nullptr };           // 下载文件句柄（.part 临时文件）
    std::string                        downloadTempPath;                  // 下载临时路径（成功后 rename）
    curl_slist*                        headerList{ nullptr };             // curl 请求头链表
    char                               curlErrorBuffer[CURL_ERROR_SIZE];  // curl 错误缓冲区
    Response                           response;                          // 响应快照
    int64_t                            lastProgressCallbackMs{ 0 };       // 上次进度回调时间（ms）

    explicit RequestContext(const Request& inRequest, RequestId inRequestId, HttpManager* inOwner)
        : request(inRequest), requestId(inRequestId), owner(inOwner) {
        std::memset(curlErrorBuffer, 0, sizeof(curlErrorBuffer));
    }

    ~RequestContext() {
        if (headerList != nullptr) {
#if ENABLED(CURL)
            curl_slist_free_all(headerList);
#endif
            headerList = nullptr;
        }
        if (uploadFile != nullptr) {
            std::fclose(uploadFile);
            uploadFile = nullptr;
        }
        if (downloadFile != nullptr) {
            std::fclose(downloadFile);
            downloadFile = nullptr;
        }
    }
};

/// @brief 主线程事件派发消息
struct HttpManager::MainThreadMessage {
    IEventListener*                    listener{ nullptr };  // 目标监听者
    std::shared_ptr<std::atomic<bool>> listenerAlive;        // 监听者存活标记快照
    Event                              event;                // 待派发事件

    MainThreadMessage() = default;
};

/// @brief 重试队列条目
struct HttpManager::RetryEntry {
    std::chrono::steady_clock::time_point dueTime;  // 到期时间
    RequestPtr                            request;  // 待重试请求
};

/// @brief 构造函数
HttpManager::HttpManager() = default;

/// @brief 析构函数
HttpManager::~HttpManager() {
    shutdown();
}

/// @brief 初始化
/// @param mainLooper 主线程 Looper（为空时自动取当前线程 Looper）
/// @param maxConcurrent 最大并发请求数（0 表示使用默认值 4）
/// @param pollTimeoutMs 轮询超时（ms，0 表示使用默认值 1000）
/// @note 必须在 submit / cancel 等接口前调用，初始化失败时直接终止进程
void HttpManager::init(cdroid::Looper* mainLooper, size_t maxConcurrent, long pollTimeoutMs) {
#if ENABLED(CURL)
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    FailFast(mInitialized.load(), "HttpManager::init called twice");

    mMainLooper = mainLooper != nullptr ? mainLooper : cdroid::Looper::getForThread();
    FailFast(mMainLooper == nullptr, "main looper is null, call cdroid::Looper::prepare()/getForThread() in owner thread first");

    mMaxConcurrent = (maxConcurrent == 0) ? 4 : maxConcurrent;
    mPollTimeoutMs = pollTimeoutMs > 0 ? pollTimeoutMs : 1000;

    LOGI("http init. maxConcurrent=%zu pollTimeoutMs=%ld mainLooper=%p", mMaxConcurrent, mPollTimeoutMs, mMainLooper);

    mStopping.store(false);
    mNextRequestId.store(1);
    mTotalSubmitted.store(0);
    mTotalCompleted.store(0);
    mTotalCancelled.store(0);
    mTotalRetried.store(0);

    initializeCurlGlobal();

    // curl_multi 多路复用句柄
    mMulti = curl_multi_init();
    FailFast(mMulti == nullptr, "curl_multi_init failed");

    // 连接池：跨 easy 复用 DNS 解析与 keep-alive 连接
    mShare = curl_share_init();
    FailFast(mShare == nullptr, "curl_share_init failed");
    curl_share_setopt(mShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(mShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(mShare, CURLSHOPT_LOCKFUNC, &httpShareLock);
    curl_share_setopt(mShare, CURLSHOPT_UNLOCKFUNC, &httpShareUnlock);
    curl_share_setopt(mShare, CURLSHOPT_USERDATA, &mShareMutex);

    // 主线程唤醒 eventfd
    mMainWakeFd = SystemUtils::createEventFd();
    FailFast(mMainWakeFd < 0, "eventfd(main) failed, errno=%d", errno);
    const int addResult = mMainLooper->addFd(mMainWakeFd, 0,
        cdroid::Looper::EVENT_INPUT, onMainThreadWake, this);
    FailFast(addResult < 0, "Looper::addFd failed, fd=%d", mMainWakeFd);

    // I/O 线程唤醒 eventfd
    mIoWakeFd = SystemUtils::createEventFd();
    FailFast(mIoWakeFd < 0, "eventfd(io) failed, errno=%d", errno);

    // 启动单 I/O 线程
    mIoThread = std::thread(&HttpManager::ioLoop, this);

    mInitialized.store(true);
    LOGI("http initialized. mainFd=%d ioFd=%d", mMainWakeFd, mIoWakeFd);
#endif
}

/// @brief 关闭管理器，释放线程、fd、curl 全局资源
void HttpManager::shutdown() {
#if ENABLED(CURL)
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    if (!mInitialized.load()) {
        return;
    }

    // 停止接收新请求，取消全部在途请求并唤醒 I/O 线程收尾
    mStopping.store(true);
    cancelAll();
    wakeIoThread();

    if (mIoThread.joinable()) {
        LOGV("http shutdown waiting io thread");
        mIoThread.join();
    }

    if (mMainLooper != nullptr && mMainWakeFd >= 0) {
        mMainLooper->removeFd(mMainWakeFd);
    }
    SystemUtils::destroyEventFd(mMainWakeFd);
    SystemUtils::destroyEventFd(mIoWakeFd);

    if (mMulti != nullptr) {
        curl_multi_cleanup(mMulti);
        mMulti = nullptr;
    }
    if (mShare != nullptr) {
        curl_share_cleanup(mShare);
        mShare = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        mMainThreadMessages.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        mPendingRequests.clear();
        mActiveRequests.clear();
        mRetryQueue.clear();
    }

    cleanupCurlGlobal();

    mMainLooper = nullptr;
    mStopping.store(false);
    mInitialized.store(false);
    LOGI("http shutdown done. submitted=%zu completed=%zu cancelled=%zu retried=%zu",
        mTotalSubmitted.load(), mTotalCompleted.load(), mTotalCancelled.load(), mTotalRetried.load());
#endif
}

/// @brief 是否已初始化
/// @return true 已初始化，false 未初始化
bool HttpManager::isInitialized() const {
    return mInitialized.load();
}

/// @brief 提交请求
/// @param request 请求对象
/// @return 唯一请求 ID（>0 表示成功，0 表示失败）
HttpManager::RequestId HttpManager::submit(const Request& request) {
#if ENABLED(CURL)
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    validateRequestOrDie(request);
    FailFast(mStopping.load(), "submit called while manager is stopping");

    const RequestId requestId = mNextRequestId.fetch_add(1);
    const RequestPtr requestContext(new RequestContext(request, requestId, this));

    // 关联生命周期标记，避免 UAF
    requestContext->listenerAlive = request.listener->aliveToken();
    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        mPendingRequests.push_back(requestContext);
    }
    mTotalSubmitted.fetch_add(1);
    wakeIoThread();
    LOGV("http submit. id=%llu method=%s url=%s tag=%s",
        static_cast<unsigned long long>(requestId),
        methodName(request.method),
        request.url.c_str(),
        request.tag.c_str());
    return requestId;
#else
    return 0;
#endif
}

/// @brief 按请求 ID 取消单个请求
/// @param requestId 请求 ID
/// @return true 已受理，false 请求不存在或已完成
bool HttpManager::cancel(RequestId requestId) {
#if ENABLED(CURL)
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    RequestPtr cancelledContext;
    bool activeFound = false;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        // 待执行请求：直接摘除并立即回取消事件
        for (auto it = mPendingRequests.begin(); it != mPendingRequests.end(); ++it) {
            if ((*it)->requestId == requestId) {
                cancelledContext = *it;
                mPendingRequests.erase(it);
                break;
            }
        }

        // 执行中 / 重试中的请求：仅打取消标记，由 I/O 线程收尾
        if (!cancelledContext) {
            auto it = mActiveRequests.find(requestId);
            if (it != mActiveRequests.end()) {
                it->second->cancelRequested.store(true);
                activeFound = true;
            } else {
                for (auto& entry : mRetryQueue) {
                    if (entry.request->requestId == requestId) {
                        entry.request->cancelRequested.store(true);
                        activeFound = true;
                        break;
                    }
                }
            }
        }
    }

    if (activeFound) {
        wakeIoThread();
        LOGV("http cancel. id=%llu hit=active/retry", static_cast<unsigned long long>(requestId));
        return true;
    }

    if (cancelledContext) {
        cancelledContext->response.cancelled = true;
        cancelledContext->response.errorMessage = "cancelled before execution";
        mTotalCancelled.fetch_add(1);
        LOGV("http cancel. id=%llu hit=pending", static_cast<unsigned long long>(requestId));
        postEvent(cancelledContext, EventType::CANCELLED);
        return true;
    }

    LOGV("http cancel. id=%llu hit=none", static_cast<unsigned long long>(requestId));
    return false;
#else
    return true;
#endif
}

/// @brief 按 tag 批量取消请求
/// @param tag 请求 tag
/// @return 取消数量
size_t HttpManager::cancelByTag(const std::string& tag) {
#if ENABLED(CURL)
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    if (tag.empty()) {
        return 0;
    }

    std::vector<RequestPtr> pendingCancelled;
    size_t cancelledCount = 0;
    bool needWake = false;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        for (auto it = mPendingRequests.begin(); it != mPendingRequests.end();) {
            if ((*it)->request.tag == tag) {
                (*it)->cancelRequested.store(true);
                pendingCancelled.push_back(*it);
                it = mPendingRequests.erase(it);
                ++cancelledCount;
            } else {
                ++it;
            }
        }

        for (auto& kv : mActiveRequests) {
            if (kv.second->request.tag == tag) {
                kv.second->cancelRequested.store(true);
                ++cancelledCount;
                needWake = true;
            }
        }

        for (auto& entry : mRetryQueue) {
            if (entry.request->request.tag == tag) {
                entry.request->cancelRequested.store(true);
                ++cancelledCount;
            }
        }
    }

    if (needWake) {
        wakeIoThread();
    }

    for (size_t i = 0; i < pendingCancelled.size(); ++i) {
        pendingCancelled[i]->response.cancelled = true;
        pendingCancelled[i]->response.errorMessage = "cancelled before execution";
        postEvent(pendingCancelled[i], EventType::CANCELLED);
    }
    /* 仅 pending 在此计数；active/retry 由各自收尾路径计数，避免重复统计 */
    mTotalCancelled.fetch_add(pendingCancelled.size());

    LOGV("http cancelByTag. tag=%s count=%zu pending=%zu", tag.c_str(), cancelledCount, pendingCancelled.size());
    return cancelledCount;
#else
    return 0;
#endif
}

/// @brief 取消全部待执行、执行中与重试中的请求
void HttpManager::cancelAll() {
#if ENABLED(CURL)
    if (!mInitialized.load()) {
        return;
    }

    std::vector<RequestPtr> pendingCancelled;
    bool needWake = false;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        while (!mPendingRequests.empty()) {
            RequestPtr requestContext = mPendingRequests.front();
            mPendingRequests.pop_front();
            requestContext->cancelRequested.store(true);
            pendingCancelled.push_back(requestContext);
        }

        for (auto& kv : mActiveRequests) {
            kv.second->cancelRequested.store(true);
            needWake = true;
        }

        /* 重试队列一并清空并派发取消事件（否则会静默丢失 CANCELLED 回调） */
        while (!mRetryQueue.empty()) {
            RequestPtr requestContext = mRetryQueue.front().request;
            mRetryQueue.pop_front();
            requestContext->cancelRequested.store(true);
            pendingCancelled.push_back(requestContext);
        }
    }

    if (needWake) {
        wakeIoThread();
    }

    for (size_t i = 0; i < pendingCancelled.size(); ++i) {
        pendingCancelled[i]->response.cancelled = true;
        pendingCancelled[i]->response.errorMessage = "cancelled before execution";
        postEvent(pendingCancelled[i], EventType::CANCELLED);
    }
    mTotalCancelled.fetch_add(pendingCancelled.size());

    if (!pendingCancelled.empty() || needWake) {
        LOGI("http cancelAll. cancelled=%zu activeWake=%d", pendingCancelled.size(), needWake ? 1 : 0);
    }
#endif
}

/// @brief 查询待执行请求数量
/// @return 待执行请求数量
size_t HttpManager::pendingCount() const {
#if ENABLED(CURL)
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size();
#else
    return 0;
#endif
}

/// @brief 查询正在执行的请求数量
/// @return 正在执行的请求数量
size_t HttpManager::runningCount() const {
#if ENABLED(CURL)
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mActiveRequests.size();
#else
    return 0;
#endif
}

/// @brief 查询队列中的请求总数（待执行 + 执行中 + 重试中）
/// @return 队列中的请求总数
size_t HttpManager::requestCount() const {
#if ENABLED(CURL)
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size() + mActiveRequests.size() + mRetryQueue.size();
#else
    return 0;
#endif
}

/// @brief 查询请求是否存在
/// @param requestId 请求 ID
/// @return true 存在，false 不存在
bool HttpManager::contains(RequestId requestId) const {
#if ENABLED(CURL)
    if (!mInitialized.load()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mRequestMutex);

    for (size_t i = 0; i < mPendingRequests.size(); ++i) {
        if (mPendingRequests[i]->requestId == requestId) {
            return true;
        }
    }
    if (mActiveRequests.find(requestId) != mActiveRequests.end()) {
        return true;
    }
    for (size_t i = 0; i < mRetryQueue.size(); ++i) {
        if (mRetryQueue[i].request->requestId == requestId) {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

/// @brief 初始化 libcurl 全局环境
/// @note 引用计数：首个实例真正调用 curl_global_init，失败直接终止进程
void HttpManager::initializeCurlGlobal() {
#if ENABLED(CURL)
    std::lock_guard<std::mutex> lock(sCurlGlobalMutex);
    if (sCurlGlobalRefCount == 0) {
        const CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
        FailFast(rc != CURLE_OK, "curl_global_init failed, rc=%d", static_cast<int>(rc));
        LOGV("curl_global_init done. version=%s", curl_version());
    }
    ++sCurlGlobalRefCount;
    LOGV("curl global ref. count=%zu", sCurlGlobalRefCount);
#endif
}

/// @brief 清理 libcurl 全局环境
/// @note 引用计数：计数减到 0 时才真正调用 curl_global_cleanup
void HttpManager::cleanupCurlGlobal() {
#if ENABLED(CURL)
    std::lock_guard<std::mutex> lock(sCurlGlobalMutex);
    FailFast(sCurlGlobalRefCount <= 0, "invalid curl global ref count");
    --sCurlGlobalRefCount;
    LOGV("curl global release. count=%zu", sCurlGlobalRefCount);
    if (sCurlGlobalRefCount == 0) {
        curl_global_cleanup();
        LOGV("curl_global_cleanup done");
    }
#endif
}

/// @brief 请求参数校验
/// @param request 请求对象
/// @note 失败时直接终止进程
void HttpManager::validateRequestOrDie(const Request& request) {
    FailFast(request.url.empty(), "request.url is empty");
    FailFast(request.listener == nullptr, "request.listener is null, url=%s", request.url.c_str());
    FailFast(request.connectTimeoutMs <= 0, "connectTimeoutMs must be > 0, url=%s", request.url.c_str());
    FailFast(request.requestTimeoutMs <= 0, "requestTimeoutMs must be > 0, url=%s", request.url.c_str());
    FailFast(request.progressIntervalMs < 0, "progressIntervalMs must be >= 0, url=%s", request.url.c_str());
    FailFast(!request.body.empty() && !request.uploadFilePath.empty(), "body and uploadFilePath cannot both be set, url=%s", request.url.c_str());

    if (request.method == HttpMethod::CUSTOM) {
        FailFast(request.customMethod.empty(), "customMethod is required when method == CUSTOM, url=%s", request.url.c_str());
    }

    if (!request.downloadFilePath.empty()) {
        FailFast(request.storeResponseBody, "downloadFilePath and storeResponseBody cannot both be enabled, url=%s", request.url.c_str());
    }

    if (!request.body.empty()) {
        switch (request.method) {
        case HttpMethod::POST:
        case HttpMethod::PUT:
        case HttpMethod::PATCH:
        case HttpMethod::DELETE_:
        case HttpMethod::CUSTOM: {
        }   break;
        default: {
            FailFast(true, "request.body is only supported for POST/PUT/PATCH/DELETE_/CUSTOM, url=%s", request.url.c_str());
        }   break;
        }
    }

    if (!request.uploadFilePath.empty()) {
        switch (request.method) {
        case HttpMethod::PUT:
        case HttpMethod::PATCH:
        case HttpMethod::CUSTOM: {
        }   break;
        default: {
            FailFast(true, "uploadFilePath is only supported for PUT/PATCH/CUSTOM, url=%s", request.url.c_str());
        }   break;
        }
    }
}

/// @brief 主线程唤醒回调
int HttpManager::onMainThreadWake(int fd, int /*events*/, void* context) {
    SystemUtils::drainEventFd(fd);

    HttpManager* manager = static_cast<HttpManager*>(context);
    manager->drainMainThreadMessages();
    return 1;
}

/// @brief 唤醒主线程处理事件
void HttpManager::wakeMainThread() {
    if (mMainWakeFd < 0) {
        return;
    }

    FailFast(!SystemUtils::notifyEventFd(mMainWakeFd), "write(eventfd) failed, errno=%d", errno);
}

/// @brief 唤醒 I/O 线程处理请求
void HttpManager::wakeIoThread() {
    if (mIoWakeFd < 0) {
        return;
    }

    FailFast(!SystemUtils::notifyEventFd(mIoWakeFd), "write(eventfd) failed, errno=%d", errno);
}

/// @brief 派发主线程事件队列
void HttpManager::drainMainThreadMessages() {
    std::deque<MainThreadMessage> localMessages;
    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        localMessages.swap(mMainThreadMessages);
    }

    while (!localMessages.empty()) {
        MainThreadMessage message = localMessages.front();
        localMessages.pop_front();

        FailFast(message.listener == nullptr, "message.listener is null, requestId=%llu", static_cast<unsigned long long>(message.event.requestId));

        // 投递前再次校验：监听者可能在事件入队后、派发前失效
        if (message.listenerAlive && !message.listenerAlive->load()) {
            LOGV("http dispatch drop. id=%llu event=%s reason=listener gone",
                static_cast<unsigned long long>(message.event.requestId),
                eventName(message.event.type));
            continue;
        }
        LOGV("http dispatch. id=%llu event=%s",
            static_cast<unsigned long long>(message.event.requestId),
            eventName(message.event.type));
        message.listener->onHttpEvent(message.event);
    }
}

/// @brief 投递主线程事件（智能指针重载）
/// @param requestContext 请求上下文
/// @param type 事件类型
void HttpManager::postEvent(const RequestPtr& requestContext, EventType type) {
    postEvent(requestContext.get(), type);
}

/// @brief 投递主线程事件（实际实现）
/// @param requestContext 请求上下文
/// @param type 事件类型
/// @note 监听者已失效时直接丢弃，不投递也不入队
void HttpManager::postEvent(RequestContext* requestContext, EventType type) {
    FailFast(requestContext == nullptr, "requestContext is null");

    // 监听者已失效则不再投递事件，避免队列堆积与野指针回调
    const std::shared_ptr<std::atomic<bool>>& listenerAlive = requestContext->listenerAlive;
    if (listenerAlive && !listenerAlive->load()) {
        LOGV("http postEvent skip. id=%llu event=%s reason=listener gone",
            static_cast<unsigned long long>(requestContext->requestId),
            eventName(type));
        return;
    }

    MainThreadMessage message;
    message.listener = requestContext->request.listener;
    message.listenerAlive = listenerAlive;
    message.event.type = type;
    message.event.requestId = requestContext->requestId;
    message.event.method = requestContext->request.method;
    message.event.url = requestContext->request.url;
    message.event.tag = requestContext->request.tag;
    message.event.opaque = requestContext->request.opaque;
    message.event.response = requestContext->response;

    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        mMainThreadMessages.push_back(message);
    }
    wakeMainThread();
}

/// @brief 投递进度事件
/// @param requestContext 请求上下文
/// @param downloadTotal 下载总字节数
/// @param downloadNow 当前已下载字节数
/// @param uploadTotal 上传总字节数
/// @param uploadNow 当前已上传字节数
void HttpManager::postProgressEvent(RequestContext* requestContext,
    curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow) {
    if (!requestContext->request.emitProgress) {
        return;
    }

    const int64_t nowMs = cdroid::SystemClock::uptimeMillis();
    if (requestContext->request.progressIntervalMs > 0 &&
        requestContext->lastProgressCallbackMs > 0 &&
        nowMs - requestContext->lastProgressCallbackMs < requestContext->request.progressIntervalMs) {
        return;
    }

    requestContext->lastProgressCallbackMs = nowMs;
    requestContext->response.downloadedBytes = downloadNow;
    requestContext->response.downloadTotalBytes = downloadTotal;
    requestContext->response.uploadedBytes = uploadNow;
    requestContext->response.uploadTotalBytes = uploadTotal;
    postEvent(requestContext, EventType::PROGRESS);
}

/// @brief I/O 线程主循环
/// @note 仅在独立 I/O 线程内执行，阻塞在 curl_multi_wait 上等待 socket 或 eventfd
void HttpManager::ioLoop() {
#if ENABLED(CURL)
    LOGV("http io loop enter");
    for (;;) {
        processPendingOperations();

        int runningHandles = 0;
        curl_multi_perform(mMulti, &runningHandles);

        processCompleted();
        processRetries();

        // 停止流程中，且所有请求都已收尾，退出循环
        if (mStopping.load()) {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            if (mPendingRequests.empty()
                && mActiveRequests.empty()
                && mRetryQueue.empty()) {
                LOGV("http io loop exit");
                return;
            }
        }

        // 等待 socket 活动或 eventfd 唤醒；若存在待重试请求，则缩短超时
        // 至其最早到期时间，保证 500ms 档退避延迟能准时触发
        int waitTimeoutMs = static_cast<int>(mPollTimeoutMs);
        {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            if (!mRetryQueue.empty()) {
                const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                const std::chrono::steady_clock::time_point& due = mRetryQueue.front().dueTime;
                if (due <= now) {
                    waitTimeoutMs = 0;
                } else {
                    const long remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(due - now).count();
                    if (remainingMs < waitTimeoutMs) {
                        waitTimeoutMs = static_cast<int>(remainingMs);
                    }
                }
            }
        }

        int numFds = 0;
        curl_waitfd waitFd;
        waitFd.fd = mIoWakeFd;
        waitFd.events = CURL_WAIT_POLLIN;
        waitFd.revents = 0;
        curl_multi_wait(mMulti, &waitFd, 1, waitTimeoutMs, &numFds);

        SystemUtils::drainEventFd(mIoWakeFd);
    }
#endif
}

/// @brief 处理待执行请求队列，在并发上限内启动请求
void HttpManager::processPendingOperations() {
#if ENABLED(CURL)
    std::vector<RequestPtr> toStart;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        while (!mPendingRequests.empty() && mActiveRequests.size() < mMaxConcurrent) {
            toStart.push_back(mPendingRequests.front());
            mPendingRequests.pop_front();
        }
    }

    for (size_t i = 0; i < toStart.size(); ++i) {
        startRequest(toStart[i]);
    }
    if (!toStart.empty()) {
        LOGV("http start batch. count=%zu active=%zu limit=%zu",
            toStart.size(), mActiveRequests.size(), mMaxConcurrent);
    }
#endif
}

/// @brief 处理 curl_multi 中已完成的请求
void HttpManager::processCompleted() {
#if ENABLED(CURL)
    CURLMsg* msg = nullptr;
    int msgsLeft = 0;

    while ((msg = curl_multi_info_read(mMulti, &msgsLeft)) != nullptr) {
        if (msg->msg != CURLMSG_DONE) {
            continue;
        }

        CURL* easy = msg->easy_handle;
        char* priv = nullptr;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv);
        RequestContext* raw = reinterpret_cast<RequestContext*>(priv);

        RequestPtr requestContext;
        if (raw != nullptr) {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            auto it = mActiveRequests.find(raw->requestId);
            if (it != mActiveRequests.end()) {
                requestContext = it->second;
                mActiveRequests.erase(it);
            }
        }

        if (requestContext) {
            handleCompletion(requestContext, easy, msg->data.result);
        } else {
            LOGW("http done without context. easy=%p result=%d", easy, static_cast<int>(msg->data.result));
            curl_easy_cleanup(easy);
        }
    }
#endif
}

/// @brief 处理重试队列，按到期时间将请求重新入队
void HttpManager::processRetries() {
#if ENABLED(CURL)
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    std::vector<RequestPtr> due;
    bool requeued = false;
    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        while (!mRetryQueue.empty() && mRetryQueue.front().dueTime <= now) {
            due.push_back(mRetryQueue.front().request);
            mRetryQueue.pop_front();
        }
    }

    for (size_t i = 0; i < due.size(); ++i) {
        if (due[i]->cancelRequested.load()) {
            due[i]->response.cancelled = true;
            due[i]->response.errorMessage = "cancelled before retry";
            mTotalCancelled.fetch_add(1);
            LOGV("http retry cancelled. id=%llu", static_cast<unsigned long long>(due[i]->requestId));
            postEvent(due[i], EventType::CANCELLED);
            continue;
        }

        // 到期且未取消：放回待执行队列头部（重试优先于新请求），
        // 交由 processPendingOperations 统一受 mMaxConcurrent 约束出队，
        // 避免重试路径绕过全局并发上限。
        {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            mPendingRequests.push_front(due[i]);
        }
        requeued = true;
        LOGV("http retry requeue. id=%llu retried=%d",
            static_cast<unsigned long long>(due[i]->requestId), due[i]->retryCount);
    }

    if (requeued) {
        wakeIoThread();
    }
#endif
}

/// @brief 启动一个 HTTP 请求
/// @param requestContext 请求上下文
void HttpManager::startRequest(const RequestPtr& requestContext) {
#if ENABLED(CURL)
    if (requestContext->cancelRequested.load()) {
        requestContext->response.cancelled = true;
        requestContext->response.errorMessage = "cancelled before execution";
        mTotalCancelled.fetch_add(1);
        LOGV("http start skip. id=%llu reason=cancelled", static_cast<unsigned long long>(requestContext->requestId));
        postEvent(requestContext, EventType::CANCELLED);
        return;
    }

    requestContext->bodyReadOffset = 0;

    LOGV("http start. id=%llu method=%s url=%s retry=%d",
        static_cast<unsigned long long>(requestContext->requestId),
        methodName(requestContext->request.method),
        requestContext->request.url.c_str(),
        requestContext->retryCount);

    CURL* easy = buildEasy(requestContext);
    if (easy == nullptr) {
        // 构建失败：清理可能已打开的下载临时文件与上传句柄
        LOGW("http build failed. id=%llu url=%s err=%s",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->request.url.c_str(),
            requestContext->response.errorMessage.c_str());
        finalizeDownload(requestContext.get(), false);
        if (requestContext->uploadFile != nullptr) {
            std::fclose(requestContext->uploadFile);
            requestContext->uploadFile = nullptr;
        }
        postEvent(requestContext, EventType::COMPLETED);
        return;
    }

    requestContext->easy = easy;
    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        mActiveRequests[requestContext->requestId] = requestContext;
    }

    const CURLMcode addCode = curl_multi_add_handle(mMulti, easy);
    if (addCode != CURLM_OK) {
        // add 失败：回滚 active 表并清理句柄，投递失败事件
        LOGE("http add handle failed. id=%llu code=%d(%s)",
            static_cast<unsigned long long>(requestContext->requestId),
            static_cast<int>(addCode), curl_multi_strerror(addCode));
        {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            mActiveRequests.erase(requestContext->requestId);
        }
        requestContext->response.curlCode = CURLE_FAILED_INIT;
        requestContext->response.errorMessage = "curl_multi_add_handle failed";
        finalizeDownload(requestContext.get(), false);
        if (requestContext->uploadFile != nullptr) {
            std::fclose(requestContext->uploadFile);
            requestContext->uploadFile = nullptr;
        }
        curl_easy_cleanup(easy);
        requestContext->easy = nullptr;
        postEvent(requestContext, EventType::COMPLETED);
        return;
    }

    // STARTED 必须在 active 表登记、add_handle 成功之后投递，
    // 否则监听者在 STARTED 回调里调用 cancel() 时请求尚未入表，
    // cancel() 找不到请求会静默失败，导致"取消无效果"。
    postEvent(requestContext, EventType::STARTED);
#endif
}

/// @brief 构建一个 CURL 句柄，并设置其选项
/// @param requestContext 请求上下文
/// @return CURL 句柄，失败时返回 nullptr
CURL* HttpManager::buildEasy(const RequestPtr& requestContext) {
#if ENABLED(CURL)
    // 重试会重复调用本函数：先释放上一轮残留的请求头链表，并清空错误缓冲，
    // 避免请求头重复追加、以及旧错误信息污染新一次尝试
    if (requestContext->headerList != nullptr) {
        curl_slist_free_all(requestContext->headerList);
        requestContext->headerList = nullptr;
    }
    std::memset(requestContext->curlErrorBuffer, 0, sizeof(requestContext->curlErrorBuffer));

    // 重试会复用 Response 快照：清空上一轮的单次尝试数据，
    // 避免响应体/响应头跨尝试拼接、旧错误信息与进度残留污染新一次尝试
    requestContext->response.errorMessage.clear();
    requestContext->response.effectiveUrl.clear();
    requestContext->response.body.clear();
    requestContext->response.responseHeaders.clear();
    requestContext->response.downloadedBytes = 0;
    requestContext->response.downloadTotalBytes = 0;
    requestContext->response.uploadedBytes = 0;
    requestContext->response.uploadTotalBytes = 0;

    CURL* easy = curl_easy_init();
    if (easy == nullptr) {
        LOGE("curl_easy_init failed. id=%llu", static_cast<unsigned long long>(requestContext->requestId));
        requestContext->response.curlCode = CURLE_FAILED_INIT;
        requestContext->response.errorMessage = "curl_easy_init failed";
        return nullptr;
    }

    Request& request = requestContext->request;
    Response& response = requestContext->response;

    curl_easy_setopt(easy, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, requestContext->curlErrorBuffer);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, request.connectTimeoutMs);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, request.requestTimeoutMs);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, request.followRedirects ? 1L : 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, request.verifyPeer ? 1L : 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, request.verifyPeer ? 2L : 0L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &HttpManager::onWrite);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, requestContext.get());
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &HttpManager::onHeader);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, requestContext.get());
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, &HttpManager::onProgress);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, requestContext.get());
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, requestContext.get());
    if (mShare != nullptr) {
        curl_easy_setopt(easy, CURLOPT_SHARE, mShare);
    }

    if (!request.caFilePath.empty()) {
        curl_easy_setopt(easy, CURLOPT_CAINFO, request.caFilePath.c_str());
    }
    if (request.lowSpeedTimeSeconds > 0) {
        curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, request.lowSpeedTimeSeconds);
    }
    if (request.lowSpeedLimitBytesPerSec > 0) {
        curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, request.lowSpeedLimitBytesPerSec);
    }
    if (request.maxRecvSpeedBytesPerSec > 0) {
        curl_easy_setopt(easy, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(request.maxRecvSpeedBytesPerSec));
    }
    if (request.maxSendSpeedBytesPerSec > 0) {
        curl_easy_setopt(easy, CURLOPT_MAX_SEND_SPEED_LARGE, static_cast<curl_off_t>(request.maxSendSpeedBytesPerSec));
    }

    for (size_t i = 0; i < request.headers.size(); ++i) {
        curl_slist* appended = curl_slist_append(requestContext->headerList,
            request.headers[i].c_str());
        if (appended == nullptr) {
            LOGE("curl_slist_append failed. id=%llu header=%s",
                static_cast<unsigned long long>(requestContext->requestId),
                request.headers[i].c_str());
            curl_easy_cleanup(easy);
            response.curlCode = CURLE_OUT_OF_MEMORY;
            response.errorMessage = "curl_slist_append failed";
            return nullptr;
        }
        requestContext->headerList = appended;
    }
    if (requestContext->headerList != nullptr) {
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, requestContext->headerList);
    }

    // 下载目标
    if (!request.downloadFilePath.empty()) {
        requestContext->downloadTempPath = request.downloadFilePath + ".part";
        requestContext->downloadFile = std::fopen(requestContext->downloadTempPath.c_str(), "wb");
        if (requestContext->downloadFile == nullptr) {
            LOGE("open download file failed. id=%llu path=%s errno=%d",
                static_cast<unsigned long long>(requestContext->requestId),
                requestContext->downloadTempPath.c_str(), errno);
            curl_easy_cleanup(easy);
            response.curlCode = CURLE_WRITE_ERROR;
            response.errorMessage = "fopen download file failed, errno=" + std::to_string(errno);
            return nullptr;
        }
        response.outputFilePath = request.downloadFilePath;
        LOGV("http download target. id=%llu temp=%s",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->downloadTempPath.c_str());
    } else if (!request.storeResponseBody) {
        curl_easy_cleanup(easy);
        response.curlCode = CURLE_WRITE_ERROR;
        response.errorMessage = "no output target for response body";
        return nullptr;
    }

    // 上传目标
    if (!request.uploadFilePath.empty()) {
        struct stat fileStat;
        const int statRc = stat(request.uploadFilePath.c_str(), &fileStat);
        if (statRc != 0 || !S_ISREG(fileStat.st_mode)) {
            LOGE("upload file not readable. id=%llu path=%s",
                static_cast<unsigned long long>(requestContext->requestId),
                request.uploadFilePath.c_str());
            curl_easy_cleanup(easy);
            response.curlCode = CURLE_READ_ERROR;
            response.errorMessage = "upload file not readable";
            return nullptr;
        }

        requestContext->uploadFile = std::fopen(request.uploadFilePath.c_str(), "rb");
        if (requestContext->uploadFile == nullptr) {
            LOGE("open upload file failed. id=%llu path=%s errno=%d",
                static_cast<unsigned long long>(requestContext->requestId),
                request.uploadFilePath.c_str(), errno);
            curl_easy_cleanup(easy);
            response.curlCode = CURLE_READ_ERROR;
            response.errorMessage = "fopen upload file failed";
            return nullptr;
        }
        requestContext->uploadFileSize = static_cast<curl_off_t>(fileStat.st_size);
        curl_easy_setopt(easy, CURLOPT_READFUNCTION, &HttpManager::onRead);
        curl_easy_setopt(easy, CURLOPT_READDATA, requestContext.get());
        curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, requestContext->uploadFileSize);
    }

    switch (request.method) {
    case HttpMethod::GET:
        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
        break;

    case HttpMethod::POST:
        curl_easy_setopt(easy, CURLOPT_POST, 1L);
        if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        }
        break;

    case HttpMethod::PUT:
        if (!request.uploadFilePath.empty()) {
            curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, requestContext->uploadFileSize);
        } else if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        } else {
            curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PUT");
        }
        break;

    case HttpMethod::PATCH:
        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (!request.uploadFilePath.empty()) {
            curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, requestContext->uploadFileSize);
        } else if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        }
        break;

    case HttpMethod::DELETE_:
        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "DELETE");
        if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        }
        break;

    case HttpMethod::HEAD:
        curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
        break;

    case HttpMethod::CUSTOM:
        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, request.customMethod.c_str());
        if (!request.uploadFilePath.empty()) {
            curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, requestContext->uploadFileSize);
        } else if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        }
        break;
    }

    return easy;
#else
    return nullptr;
#endif
}

/// @brief 处理 HTTP 请求完成事件
/// @param requestContext 请求上下文
/// @param easy CURL 句柄
/// @param result CURL 执行结果码
void HttpManager::handleCompletion(const RequestPtr& requestContext, CURL* easy, CURLcode result) {
#if ENABLED(CURL)
    Response& response = requestContext->response;

    long httpCode = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME, &response.totalTimeSeconds);

    char* effectiveUrl = nullptr;
    if (curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effectiveUrl) == CURLE_OK && effectiveUrl != nullptr) {
        response.effectiveUrl = effectiveUrl;
    }

    // 从 libcurl 取真实传输字节数。
    // COMPLETED 事件必须携带准确的字节统计，不能依赖 progress 回调是否触发
    // （本地回环/极小响应时 progress 可能完全不回调，导致 downloadedBytes 恒为 0）
    curl_off_t sizeDownload = 0;
    curl_off_t contentLengthDownload = 0;
    curl_off_t sizeUpload = 0;
    curl_off_t contentLengthUpload = 0;
    curl_easy_getinfo(easy, CURLINFO_SIZE_DOWNLOAD_T, &sizeDownload);
    curl_easy_getinfo(easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLengthDownload);
    curl_easy_getinfo(easy, CURLINFO_SIZE_UPLOAD_T, &sizeUpload);
    curl_easy_getinfo(easy, CURLINFO_CONTENT_LENGTH_UPLOAD_T, &contentLengthUpload);
    response.downloadedBytes = sizeDownload;
    response.downloadTotalBytes = contentLengthDownload;
    response.uploadedBytes = sizeUpload;
    response.uploadTotalBytes = contentLengthUpload;

    response.curlCode = result;
    response.httpStatusCode = httpCode;
    if (requestContext->curlErrorBuffer[0] != '\0') {
        response.errorMessage = requestContext->curlErrorBuffer;
    } else if (result != CURLE_OK) {
        response.errorMessage = curl_easy_strerror(result);
    }

    const bool cancelledNow = requestContext->cancelRequested.load() || result == CURLE_ABORTED_BY_CALLBACK;

    LOGV("http done. id=%llu curl=%d http=%ld time=%.3fs down=%lld/%lld up=%lld/%lld cancelled=%d",
        static_cast<unsigned long long>(requestContext->requestId),
        static_cast<int>(result), httpCode, response.totalTimeSeconds,
        static_cast<long long>(response.downloadedBytes), static_cast<long long>(response.downloadTotalBytes),
        static_cast<long long>(response.uploadedBytes), static_cast<long long>(response.uploadTotalBytes),
        cancelledNow ? 1 : 0);

    if (result != CURLE_OK && !cancelledNow) {
        LOGW("http failed. id=%llu url=%s curl=%d(%s) http=%ld err=%s",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->request.url.c_str(),
            static_cast<int>(result), curl_easy_strerror(result), httpCode,
            response.errorMessage.c_str());
    } else if (!cancelledNow && (httpCode >= 400)) {
        LOGW("http bad status. id=%llu url=%s http=%ld",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->request.url.c_str(), httpCode);
    }

    // 下载文件：仅在真正成功（2xx 且未取消）时原子落盘，否则清理临时文件
    if (!requestContext->request.downloadFilePath.empty()) {
        const bool downloadOk = (result == CURLE_OK)
            && !cancelledNow
            && httpCode >= 200 && httpCode < 300;
        finalizeDownload(requestContext.get(), downloadOk);
    }

    // 关闭上传句柄
    if (requestContext->uploadFile != nullptr) {
        std::fclose(requestContext->uploadFile);
        requestContext->uploadFile = nullptr;
    }

    if (cancelledNow) {
        response.cancelled = true;
        if (response.errorMessage.empty()) {
            response.errorMessage = "cancelled";
        }
        mTotalCancelled.fetch_add(1);
        LOGI("http cancelled. id=%llu url=%s",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->request.url.c_str());
        postEvent(requestContext, EventType::CANCELLED);
        curl_easy_cleanup(easy);
        requestContext->easy = nullptr;
        return;
    }

    // 幂等请求（GET/HEAD）网络层失败退避重试
    const bool retryableMethod =
        requestContext->request.method == HttpMethod::GET
        || requestContext->request.method == HttpMethod::HEAD;
    if (result != CURLE_OK
        && retryableMethod
        && requestContext->retryCount < requestContext->request.maxRetry
        && isRetryable(result, httpCode)) {
        requestContext->retryCount += 1;
        mTotalRetried.fetch_add(1);

        curl_easy_cleanup(easy);
        requestContext->easy = nullptr;

        const int delayMs = 500 * (1 << (requestContext->retryCount - 1)); // 500 / 1000 / 2000
        RetryEntry entry;
        entry.dueTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
        entry.request = requestContext;

        LOGW("http retry scheduled. id=%llu url=%s retry=%d delay=%dms curl=%d",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->request.url.c_str(), requestContext->retryCount, delayMs,
            static_cast<int>(result));

        // 入队前复查：cancel() 可能在「active 表擦除 → 入重试队列」的过渡
        // 窗口内打上取消标记却找不到请求，这里兜底拦截，避免被静默重试
        bool cancelledDuringWindow = false;
        {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            if (requestContext->cancelRequested.load()) {
                cancelledDuringWindow = true;
            } else {
                mRetryQueue.push_back(entry);
            }
        }
        if (cancelledDuringWindow) {
            response.cancelled = true;
            if (response.errorMessage.empty()) {
                response.errorMessage = "cancelled";
            }
            mTotalCancelled.fetch_add(1);
            postEvent(requestContext, EventType::CANCELLED);
        }
        return;
    }

    response.timedOut = (result == CURLE_OPERATION_TIMEDOUT);
    mTotalCompleted.fetch_add(1);
    postEvent(requestContext, EventType::COMPLETED);
    curl_easy_cleanup(easy);
    requestContext->easy = nullptr;
#endif
}

/// @brief 收尾下载文件
/// @param requestContext 请求上下文
/// @param keepFile true 落盘（fsync + rename）；false 丢弃临时文件
void HttpManager::finalizeDownload(RequestContext* requestContext, bool keepFile) {
    if (requestContext->downloadFile == nullptr) {
        return;
    }

    if (keepFile) {
        std::fflush(requestContext->downloadFile);
        const int fd = ::fileno(requestContext->downloadFile);
        if (fd >= 0) {
            ::fsync(fd);
        }
    }

    std::fclose(requestContext->downloadFile);
    requestContext->downloadFile = nullptr;

    if (keepFile) {
        if (::rename(requestContext->downloadTempPath.c_str(),
            requestContext->request.downloadFilePath.c_str()) != 0) {
            LOGE("rename downloaded file failed. src=%s dst=%s errno=%d",
                requestContext->downloadTempPath.c_str(),
                requestContext->request.downloadFilePath.c_str(), errno);
            ::unlink(requestContext->downloadTempPath.c_str());
            requestContext->response.curlCode = CURLE_WRITE_ERROR;
            requestContext->response.errorMessage = "rename downloaded file failed, errno=" + std::to_string(errno);
        } else {
            LOGI("download saved. id=%llu path=%s",
                static_cast<unsigned long long>(requestContext->requestId),
                requestContext->request.downloadFilePath.c_str());
        }
    } else {
        LOGV("download discarded. id=%llu temp=%s",
            static_cast<unsigned long long>(requestContext->requestId),
            requestContext->downloadTempPath.c_str());
        ::unlink(requestContext->downloadTempPath.c_str());
    }
}

/// @brief 判断请求是否可重试
/// @param code CURL 执行结果码
/// @param httpStatusCode HTTP 响应码
/// @return true 可重试，false 不可重试
bool HttpManager::isRetryable(CURLcode code, long httpStatusCode) {
#if ENABLED(CURL)
    // 4xx/5xx 属于服务端已响应，重试无意义（避免对非幂等语义造成重复副作用）
    if (httpStatusCode >= 400) {
        return false;
    }
    switch (code) {
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_GOT_NOTHING:
        return true;
    default:
        return false;
    }
#else
    return false;
#endif
}

/// @brief CURL 写入回调（响应体）
/// @param data 待写入的数据
/// @param size 每个数据项的字节数
/// @param count 数据项的数量
/// @param context 上下文
/// @return 写入的字节数
size_t HttpManager::onWrite(char* data, size_t size, size_t count, void* context) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);
    const size_t bytes = size * count;

    if (requestContext->downloadFile != nullptr) {
        const size_t written = std::fwrite(data, 1, bytes, requestContext->downloadFile);
        if (written != bytes) {
            LOGE("write download file failed. id=%llu written=%zu bytes=%zu errno=%d",
                static_cast<unsigned long long>(requestContext->requestId), written, bytes, errno);
            return 0;
        }
    }

    if (requestContext->request.storeResponseBody) {
        const unsigned char* first = reinterpret_cast<const unsigned char*>(data);
        requestContext->response.body.insert(requestContext->response.body.end(), first, first + bytes);
    }

    return bytes;
}

/// @brief CURL 头部回调（响应头行）
/// @param data 待处理的响应头数据
/// @param size 每个数据项的字节数
/// @param count 数据项的数量
/// @param context 上下文
/// @return 写入的字节数
size_t HttpManager::onHeader(char* data, size_t size, size_t count, void* context) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);
    const size_t bytes = size * count;
    // 去掉行尾的 \r\n（以及可能的尾随空白）
    const std::string headerLine = StringUtils::trimRight(std::string(data, bytes));
    if (!headerLine.empty()) {
        requestContext->response.responseHeaders.push_back(headerLine);
    }
    return bytes;
}

/// @brief CURL 读取回调（上传数据）
/// @param buffer 读取缓冲区（由 curl 提供，需写入数据）
/// @param size 每个数据项的字节数
/// @param count 数据项的数量
/// @param context 上下文
/// @return 读取的字节数
size_t HttpManager::onRead(char* buffer, size_t size, size_t count, void* context) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);
    const size_t maxBytes = size * count;

    if (requestContext->uploadFile != nullptr) {
        return std::fread(buffer, 1, maxBytes, requestContext->uploadFile);
    }

    const std::string& body = requestContext->request.body;
    if (requestContext->bodyReadOffset >= body.size()) {
        return 0;
    }

    const size_t remaining = body.size() - requestContext->bodyReadOffset;
    const size_t bytesToCopy = std::min(maxBytes, remaining);
    std::memcpy(buffer, body.data() + requestContext->bodyReadOffset, bytesToCopy);
    requestContext->bodyReadOffset += bytesToCopy;
    return bytesToCopy;
}

/// @brief CURL 进度回调
/// @param context 上下文
/// @param downloadTotal 总下载字节数
/// @param downloadNow 当前下载字节数
/// @param uploadTotal 总上传字节数
/// @param uploadNow 当前上传字节数
/// @return 0 继续执行，1 中断执行
int HttpManager::onProgress(void* context, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);

    if (requestContext->cancelRequested.load()) {
        return 1;
    }

    requestContext->owner->postProgressEvent(requestContext, downloadTotal, downloadNow, uploadTotal, uploadNow);
    return 0;
}
