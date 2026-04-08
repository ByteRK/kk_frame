/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-04-08 22:48:56
 * @LastEditTime: 2026-04-09 01:42:47
 * @FilePath: /kk_frame/src/app/managers/http_mgr.cc
 * @Description: Http 请求管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "http_mgr.h"

#if defined(ENABLE_CURL)

#include <core/systemclock.h>
#include <cdlog.h>

#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <thread>

#if 1
/// @brief FailFast 检查函数
#define FailFast(condition, fmt, ...)   \
    do {                                \
        if(condition){                  \
            LOGE(fmt, ##__VA_ARGS__);   \
            std::abort();               \
        }                               \
    } while (0)
#else
/// @brief FailFast 检查函数
#define FailFast(condition, fmt, ...)                                               \
    do {                                                                            \
        if (condition) {                                                            \
            std::fprintf(stderr, "[HttpManager][FATAL] " fmt "\n", ##__VA_ARGS__);  \
            std::fflush(stderr);                                                    \
            std::abort();                                                           \
        }                                                                           \
    } while (0)
#endif

std::mutex HttpManager::sCurlGlobalMutex;
size_t HttpManager::sCurlGlobalRefCount = 0;

static std::string trimHttpLine(const char* data, size_t size) {
    std::string line(data, size);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    return line;
}

static size_t resolveDefaultMaxWorkerCount() {
    const size_t hardwareCount = static_cast<size_t>(std::thread::hardware_concurrency());
    return std::max<size_t>(2, hardwareCount == 0 ? 2 : hardwareCount);
}


struct HttpManager::RequestContext {
    explicit RequestContext(const Request& inRequest,
        RequestId inRequestId,
        HttpManager* inOwner)
        : request(inRequest),
        requestId(inRequestId),
        owner(inOwner),
        cancelRequested(false),
        started(false),
        completed(false),
        bodyReadOffset(0),
        uploadFile(nullptr),
        downloadFile(nullptr),
        headerList(nullptr),
        uploadFileSize(0),
        lastProgressCallbackMs(0) {
        std::memset(curlErrorBuffer, 0, sizeof(curlErrorBuffer));
    }

    ~RequestContext() {
        if (headerList != nullptr) {
            curl_slist_free_all(headerList);
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

    Request request;                       /* 原始请求描述 */
    RequestId requestId;                   /* 请求唯一 ID */
    HttpManager* owner;                    /* 所属管理器 */
    std::atomic<bool> cancelRequested;     /* 是否已请求取消 */
    std::atomic<bool> started;             /* 是否已开始执行 */
    std::atomic<bool> completed;           /* 是否已执行结束 */

    size_t bodyReadOffset;                 /* body 读取偏移 */
    FILE* uploadFile;                      /* 上传文件句柄 */
    FILE* downloadFile;                    /* 下载文件句柄 */
    curl_slist* headerList;                /* curl 请求头链表 */
    curl_off_t uploadFileSize;             /* 上传文件大小 */
    char curlErrorBuffer[CURL_ERROR_SIZE]; /* curl 错误缓冲区 */
    Response response;                     /* 当前响应快照 */
    int64_t lastProgressCallbackMs;        /* 上次进度回调时间 */
};

struct HttpManager::MainThreadMessage {
    IEventListener* listener;
    Event event;

    MainThreadMessage() : listener(nullptr) { }
};

HttpManager::Response::Response()
    : curlCode(CURLE_OK),
    httpStatusCode(0),
    totalTimeSeconds(0.0),
    downloadedBytes(0),
    downloadTotalBytes(0),
    uploadedBytes(0),
    uploadTotalBytes(0),
    timedOut(false),
    cancelled(false) { }

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

HttpManager::Event::Event()
    : type(EventType::COMPLETED),
    requestId(0),
    method(HttpMethod::GET),
    opaque(nullptr) { }

HttpManager::Request::Request()
    : method(HttpMethod::GET),
    opaque(nullptr),
    listener(nullptr),
    connectTimeoutMs(5000),
    requestTimeoutMs(30000),
    lowSpeedTimeSeconds(0),
    maxRecvSpeedBytesPerSec(0),
    maxSendSpeedBytesPerSec(0),
    lowSpeedLimitBytesPerSec(0),
    followRedirects(true),
    verifyPeer(true),
    storeResponseBody(true),
    emitProgress(true),
    progressIntervalMs(100) { }

HttpManager::Request HttpManager::Request::CreateGet(const std::string& url,
    IEventListener* listener,
    const std::string& tag,
    void* opaque) {
    Request request;
    request.method = HttpMethod::GET;
    request.url = url;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    return request;
}

HttpManager::Request HttpManager::Request::CreatePost(const std::string& url,
    const std::string& body,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
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

HttpManager::Request HttpManager::Request::CreatePut(const std::string& url,
    const std::string& body,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
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

HttpManager::Request HttpManager::Request::CreateDownload(const std::string& url,
    const std::string& downloadFilePath,
    IEventListener* listener,
    const std::string& tag,
    void* opaque) {
    Request request;
    request.method = HttpMethod::GET;
    request.url = url;
    request.downloadFilePath = downloadFilePath;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    request.storeResponseBody = false;
    return request;
}

HttpManager::Request HttpManager::Request::CreateUpload(const std::string& url,
    const std::string& uploadFilePath,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
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

HttpManager* HttpManager::getInstance() {
    static HttpManager instance;
    return &instance;
}

HttpManager::HttpManager()
    : mInitialized(false),
    mMainLooper(nullptr),
    mWakeFd(-1),
    mStopping(false),
    mNextRequestId(1),
    mMaxWorkerCount(0),
    mWorkerIdleExitMs(0),
    mAliveWorkerCount(0),
    mIdleWorkerCount(0) { }

HttpManager::~HttpManager() {
    shutdown();
}

void HttpManager::initializeCurlGlobal() {
    std::lock_guard<std::mutex> lock(sCurlGlobalMutex);
    if (sCurlGlobalRefCount == 0) {
        const CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
        FailFast(rc != CURLE_OK,
            "curl_global_init failed, rc=%d",
            static_cast<int>(rc));
    }
    ++sCurlGlobalRefCount;
}

void HttpManager::cleanupCurlGlobal() {
    std::lock_guard<std::mutex> lock(sCurlGlobalMutex);
    FailFast(sCurlGlobalRefCount <= 0, "invalid curl global ref count");
    --sCurlGlobalRefCount;
    if (sCurlGlobalRefCount == 0) {
        curl_global_cleanup();
    }
}

void HttpManager::validateRequestOrDie(const Request& request) {
    FailFast(request.url.empty(), "request.url is empty");
    FailFast(request.listener == nullptr,
        "request.listener is null, url=%s",
        request.url.c_str());
    FailFast(request.connectTimeoutMs <= 0,
        "connectTimeoutMs must be > 0, url=%s",
        request.url.c_str());
    FailFast(request.requestTimeoutMs <= 0,
        "requestTimeoutMs must be > 0, url=%s",
        request.url.c_str());
    FailFast(request.progressIntervalMs < 0,
        "progressIntervalMs must be >= 0, url=%s",
        request.url.c_str());
    FailFast(!request.body.empty() && !request.uploadFilePath.empty(),
        "body and uploadFilePath cannot both be set, url=%s",
        request.url.c_str());

    if (request.method == HttpMethod::CUSTOM) {
        FailFast(request.customMethod.empty(),
            "customMethod is required when method == CUSTOM, url=%s",
            request.url.c_str());
    }

    if (!request.downloadFilePath.empty()) {
        FailFast(request.storeResponseBody,
            "downloadFilePath and storeResponseBody cannot both be enabled, url=%s",
            request.url.c_str());
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
            FailFast(true, "request.body is only supported for POST/PUT/PATCH/DELETE_/CUSTOM, url=%s",
                request.url.c_str());
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
            FailFast(true, "uploadFilePath is only supported for PUT/PATCH/CUSTOM, url=%s",
                request.url.c_str());
        }   break;
        }
    }
}

void HttpManager::init(cdroid::Looper* mainLooper,
    size_t maxWorkerCount,
    long workerIdleExitMs) {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    FailFast(mInitialized.load(), "HttpManager::init called twice");

    mMainLooper = mainLooper != nullptr ? mainLooper : cdroid::Looper::getForThread();
    FailFast(mMainLooper == nullptr,
        "main looper is null, call cdroid::Looper::prepare()/getForThread() in owner thread first");

    mMaxWorkerCount = (maxWorkerCount == 0) ? resolveDefaultMaxWorkerCount() : maxWorkerCount;
    FailFast(mMaxWorkerCount <= 0, "maxWorkerCount must be > 0");
    mWorkerIdleExitMs = workerIdleExitMs;

    mStopping.store(false);
    mNextRequestId.store(1);
    mAliveWorkerCount = 0;
    mIdleWorkerCount = 0;

    initializeCurlGlobal();

    mWakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    FailFast(mWakeFd < 0, "eventfd failed, errno=%d", errno);

    const int addResult = mMainLooper->addFd(mWakeFd, 0, cdroid::Looper::EVENT_INPUT, onMainThreadWake, this);
    FailFast(addResult == 0, "Looper::addFd failed, fd=%d", mWakeFd);

    mInitialized.store(true);
}

void HttpManager::shutdown() {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    if (!mInitialized.load()) {
        return;
    }

    /*
     * 先停止接收新请求，再取消队列中的请求
     * 正在执行的请求会在 progress 回调中感知 cancelRequested 并尽快退出
     */
    mStopping.store(true);
    cancelAll();
    mRequestCond.notify_all();

    {
        std::unique_lock<std::mutex> lock(mRequestMutex);
        mWorkerExitCond.wait(lock, [this] {
            return mAliveWorkerCount == 0;
        });
    }

    if (mMainLooper != nullptr && mWakeFd >= 0) {
        mMainLooper->removeFd(mWakeFd);
    }

    if (mWakeFd >= 0) {
        close(mWakeFd);
        mWakeFd = -1;
    }

    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        mMainThreadMessages.clear();
    }

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        mPendingRequests.clear();
        mRunningRequests.clear();
        mAliveWorkerCount = 0;
        mIdleWorkerCount = 0;
    }

    cleanupCurlGlobal();

    mMainLooper = nullptr;
    mStopping.store(false);
    mInitialized.store(false);
}

bool HttpManager::isInitialized() const {
    return mInitialized.load();
}

HttpManager::RequestId HttpManager::submit(const Request& request) {
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    validateRequestOrDie(request);
    FailFast(mStopping.load(), "submit called while manager is stopping");

    const RequestId requestId = mNextRequestId.fetch_add(1);
    const std::shared_ptr<RequestContext> requestContext(
        new RequestContext(request, requestId, this));

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);
        mPendingRequests.push_back(requestContext);
        ensureWorkerCapacityLocked();
    }
    mRequestCond.notify_all();
    return requestId;
}

bool HttpManager::cancel(RequestId requestId) {
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    std::shared_ptr<RequestContext> cancelledContext;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        /* 先尝试从等待队列中直接移除 */
        for (auto it = mPendingRequests.begin();
            it != mPendingRequests.end();
            ++it) {
            if ((*it)->requestId == requestId) {
                cancelledContext = *it;
                cancelledContext->cancelRequested.store(true);
                mPendingRequests.erase(it);
                break;
            }
        }

        /* 若请求已开始执行，则仅打取消标记，由工作线程尽快结束 */
        if (!cancelledContext) {
            auto it = mRunningRequests.find(requestId);
            if (it != mRunningRequests.end()) {
                it->second->cancelRequested.store(true);
                return true;
            }
        }
    }

    if (cancelledContext) {
        cancelledContext->response.cancelled = true;
        cancelledContext->response.errorMessage = "cancelled before execution";
        postEvent(cancelledContext, EventType::CANCELLED);
        return true;
    }

    return false;
}

size_t HttpManager::cancelByTag(const std::string& tag) {
    FailFast(!mInitialized.load(), "HttpManager is not initialized, call init() first");

    if (tag.empty()) {
        return 0;
    }

    std::vector<std::shared_ptr<RequestContext> > pendingCancelled;
    size_t cancelledCount = 0;

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

        for (auto it = mRunningRequests.begin(); it != mRunningRequests.end(); ++it) {
            if (it->second->request.tag == tag) {
                it->second->cancelRequested.store(true);
                ++cancelledCount;
            }
        }
    }

    for (size_t i = 0; i < pendingCancelled.size(); ++i) {
        pendingCancelled[i]->response.cancelled = true;
        pendingCancelled[i]->response.errorMessage = "cancelled before execution";
        postEvent(pendingCancelled[i], EventType::CANCELLED);
    }

    return cancelledCount;
}

void HttpManager::cancelAll() {
    if (!mInitialized.load()) {
        return;
    }

    std::vector<std::shared_ptr<RequestContext> > pendingCancelled;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        while (!mPendingRequests.empty()) {
            std::shared_ptr<RequestContext> requestContext = mPendingRequests.front();
            mPendingRequests.pop_front();
            requestContext->cancelRequested.store(true);
            pendingCancelled.push_back(requestContext);
        }

        for (auto it = mRunningRequests.begin(); it != mRunningRequests.end(); ++it) {
            it->second->cancelRequested.store(true);
        }
    }

    for (size_t i = 0; i < pendingCancelled.size(); ++i) {
        pendingCancelled[i]->response.cancelled = true;
        pendingCancelled[i]->response.errorMessage = "cancelled before execution";
        postEvent(pendingCancelled[i], EventType::CANCELLED);
    }
}

size_t HttpManager::pendingCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size();
}

size_t HttpManager::runningCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mRunningRequests.size();
}

size_t HttpManager::requestCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size() + mRunningRequests.size();
}

bool HttpManager::contains(RequestId requestId) const {
    if (!mInitialized.load()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mRequestMutex);

    for (size_t i = 0; i < mPendingRequests.size(); ++i) {
        if (mPendingRequests[i]->requestId == requestId) {
            return true;
        }
    }
    return mRunningRequests.find(requestId) != mRunningRequests.end();
}

void HttpManager::ensureWorkerCapacityLocked() {
    FailFast(!mInitialized.load(), "ensureWorkerCapacityLocked called before init");

    /*
     * 若待处理请求数大于当前空闲 worker 数，则按需补 worker
     * 这里只看“立即可消费 pending 队列的能力”，busy worker 不计入空闲能力
     */
    size_t neededWorkers = 0;
    if (mPendingRequests.size() > mIdleWorkerCount) {
        neededWorkers = mPendingRequests.size() - mIdleWorkerCount;
    }

    while (neededWorkers > 0 && mAliveWorkerCount < mMaxWorkerCount) {
        ++mAliveWorkerCount;
        std::thread(&HttpManager::workerLoop, this).detach();
        --neededWorkers;
    }
}

void HttpManager::onWorkerExitLocked() {
    FailFast(mAliveWorkerCount <= 0, "mAliveWorkerCount underflow");
    --mAliveWorkerCount;
    mWorkerExitCond.notify_all();
}

int HttpManager::onMainThreadWake(int fd, int /*events*/, void* context) {
    uint64_t value = 0;
    while (read(fd, &value, sizeof(value)) > 0) {
    }

    HttpManager* manager = static_cast<HttpManager*>(context);
    manager->drainMainThreadMessages();
    return 1;
}

void HttpManager::wakeMainThread() {
    if (mWakeFd < 0) {
        return;
    }

    const uint64_t value = 1;
    const ssize_t rc = write(mWakeFd, &value, sizeof(value));
    FailFast(rc < 0 && errno != EAGAIN, "write(eventfd) failed, errno=%d", errno);
}

void HttpManager::drainMainThreadMessages() {
    std::deque<MainThreadMessage> localMessages;
    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        localMessages.swap(mMainThreadMessages);
    }

    while (!localMessages.empty()) {
        MainThreadMessage message = localMessages.front();
        localMessages.pop_front();

        FailFast(message.listener == nullptr,
            "message.listener is null, requestId=%llu",
            static_cast<unsigned long long>(message.event.requestId));
        message.listener->onHttpEvent(message.event);
    }
}

void HttpManager::postEvent(const std::shared_ptr<RequestContext>& requestContext,
    EventType type) {
    postEvent(requestContext.get(), type);
}

void HttpManager::postEvent(RequestContext* requestContext, EventType type) {
    FailFast(requestContext == nullptr, "requestContext is null");

    MainThreadMessage message;
    message.listener = requestContext->request.listener;
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

void HttpManager::postProgressEvent(RequestContext* requestContext,
    curl_off_t downloadTotal,
    curl_off_t downloadNow,
    curl_off_t uploadTotal,
    curl_off_t uploadNow) {
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

void HttpManager::workerLoop() {
    for (;;) {
        std::shared_ptr<RequestContext> requestContext;

        {
            std::unique_lock<std::mutex> lock(mRequestMutex);

            while (!mStopping.load() && mPendingRequests.empty()) {
                ++mIdleWorkerCount;

                if (mWorkerIdleExitMs > 0) {
                    const bool hasWork = mRequestCond.wait_for(
                        lock,
                        std::chrono::milliseconds(mWorkerIdleExitMs),
                        [this] { return mStopping.load() || !mPendingRequests.empty(); });
                    --mIdleWorkerCount;

                    if (!hasWork) {
                        /*
                         * 空闲超时且仍无任务，当前 worker 自动退出
                         * submit() 后若再次有任务，会重新按需创建新 worker
                         */
                        onWorkerExitLocked();
                        return;
                    }
                } else {
                    mRequestCond.wait(lock, [this] {
                        return mStopping.load() || !mPendingRequests.empty();
                    });
                    --mIdleWorkerCount;
                }
            }

            if (mStopping.load() && mPendingRequests.empty()) {
                onWorkerExitLocked();
                return;
            }

            requestContext = mPendingRequests.front();
            mPendingRequests.pop_front();
            mRunningRequests[requestContext->requestId] = requestContext;
        }

        if (requestContext->cancelRequested.load()) {
            requestContext->response.cancelled = true;
            requestContext->response.errorMessage = "cancelled before execution";
            postEvent(requestContext, EventType::CANCELLED);
        } else {
            requestContext->started.store(true);
            postEvent(requestContext, EventType::STARTED);
            executeRequest(requestContext);
        }

        {
            std::lock_guard<std::mutex> lock(mRequestMutex);
            mRunningRequests.erase(requestContext->requestId);
        }
    }
}

void HttpManager::executeRequest(const std::shared_ptr<RequestContext>& requestContext) {
    CURL* easy = curl_easy_init();
    FailFast(easy == nullptr,
        "curl_easy_init failed, requestId=%llu",
        static_cast<unsigned long long>(requestContext->requestId));

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
        curl_easy_setopt(easy, CURLOPT_MAX_RECV_SPEED_LARGE,
            static_cast<curl_off_t>(request.maxRecvSpeedBytesPerSec));
    }
    if (request.maxSendSpeedBytesPerSec > 0) {
        curl_easy_setopt(easy, CURLOPT_MAX_SEND_SPEED_LARGE,
            static_cast<curl_off_t>(request.maxSendSpeedBytesPerSec));
    }

    for (size_t i = 0; i < request.headers.size(); ++i) {
        requestContext->headerList = curl_slist_append(requestContext->headerList, request.headers[i].c_str());
        FailFast(requestContext->headerList == nullptr,
            "curl_slist_append failed, requestId=%llu",
            static_cast<unsigned long long>(requestContext->requestId));
    }
    if (requestContext->headerList != nullptr) {
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, requestContext->headerList);
    }

    if (!request.downloadFilePath.empty()) {
        requestContext->downloadFile = std::fopen(request.downloadFilePath.c_str(), "wb");
        FailFast(requestContext->downloadFile == nullptr,
            "fopen(downloadFilePath=%s) failed, errno=%d",
            request.downloadFilePath.c_str(),
            errno);
        response.outputFilePath = request.downloadFilePath;
    }

    if (!request.uploadFilePath.empty()) {
        struct stat fileStat;
        const int statRc = stat(request.uploadFilePath.c_str(), &fileStat);
        FailFast(statRc != 0,
            "stat(uploadFilePath=%s) failed, errno=%d",
            request.uploadFilePath.c_str(),
            errno);
        FailFast(!S_ISREG(fileStat.st_mode),
            "uploadFilePath is not a regular file, path=%s",
            request.uploadFilePath.c_str());

        requestContext->uploadFile = std::fopen(request.uploadFilePath.c_str(), "rb");
        FailFast(requestContext->uploadFile == nullptr,
            "fopen(uploadFilePath=%s) failed, errno=%d",
            request.uploadFilePath.c_str(),
            errno);
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
            curl_easy_setopt(easy,
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.body.size()));
        }
        break;

    case HttpMethod::PUT:
        if (!request.uploadFilePath.empty()) {
            curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, requestContext->uploadFileSize);
        } else if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy,
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.body.size()));
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
            curl_easy_setopt(easy,
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.body.size()));
        }
        break;

    case HttpMethod::DELETE_:
        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "DELETE");
        if (!request.body.empty()) {
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.data());
            curl_easy_setopt(easy,
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.body.size()));
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
            curl_easy_setopt(easy,
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.body.size()));
        }
        break;
    }

    const CURLcode curlCode = curl_easy_perform(easy);
    response.curlCode = curlCode;

    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response.httpStatusCode);
    curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME, &response.totalTimeSeconds);

    char* effectiveUrl = nullptr;
    if (curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effectiveUrl) == CURLE_OK && effectiveUrl != nullptr) {
        response.effectiveUrl = effectiveUrl;
    }

    if (requestContext->curlErrorBuffer[0] != '\0') {
        response.errorMessage = requestContext->curlErrorBuffer;
    } else if (curlCode != CURLE_OK) {
        response.errorMessage = curl_easy_strerror(curlCode);
    }

    if (requestContext->cancelRequested.load() || curlCode == CURLE_ABORTED_BY_CALLBACK) {
        response.cancelled = true;
        if (response.errorMessage.empty()) {
            response.errorMessage = "cancelled";
        }
        postEvent(requestContext, EventType::CANCELLED);
    } else {
        response.timedOut = (curlCode == CURLE_OPERATION_TIMEDOUT);
        postEvent(requestContext, EventType::COMPLETED);
    }

    requestContext->completed.store(true);
    curl_easy_cleanup(easy);
}

size_t HttpManager::onWrite(char* data, size_t size, size_t count, void* context) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);
    const size_t bytes = size * count;

    if (requestContext->downloadFile != nullptr) {
        const size_t written = std::fwrite(data, 1, bytes, requestContext->downloadFile);
        if (written != bytes) {
            return 0;
        }
    }

    if (requestContext->request.storeResponseBody) {
        const unsigned char* first = reinterpret_cast<const unsigned char*>(data);
        requestContext->response.body.insert(requestContext->response.body.end(), first, first + bytes);
    }

    return bytes;
}

size_t HttpManager::onHeader(char* data, size_t size, size_t count, void* context) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);
    const size_t bytes = size * count;
    const std::string headerLine = trimHttpLine(data, bytes);
    if (!headerLine.empty()) {
        requestContext->response.responseHeaders.push_back(headerLine);
    }
    return bytes;
}

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

int HttpManager::onProgress(void* context,
    curl_off_t downloadTotal,
    curl_off_t downloadNow,
    curl_off_t uploadTotal,
    curl_off_t uploadNow) {
    RequestContext* requestContext = static_cast<RequestContext*>(context);

    if (requestContext->cancelRequested.load()) {
        return 1;
    }

    requestContext->owner->postProgressEvent(requestContext,
        downloadTotal,
        downloadNow,
        uploadTotal,
        uploadNow);
    return 0;
}

#endif // defined(ENABLE_CURL)