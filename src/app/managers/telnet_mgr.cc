/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-14
 * @FilePath: /kk_frame/src/app/managers/telnet_mgr.cc
 * @Description: Telnet 访问管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "telnet_mgr.h"
#include "project_utils.h"

#include <core/systemclock.h>
#include <cdlog.h>

#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {

static const unsigned char TELNET_IAC  = 255;
static const unsigned char TELNET_DONT = 254;
static const unsigned char TELNET_DO   = 253;
static const unsigned char TELNET_WONT = 252;
static const unsigned char TELNET_WILL = 251;
static const unsigned char TELNET_SB   = 250;
static const unsigned char TELNET_SE   = 240;

static size_t resolveDefaultMaxWorkerCount() {
    const size_t hardwareCount = static_cast<size_t>(std::thread::hardware_concurrency());
    return std::max<size_t>(2, hardwareCount == 0 ? 2 : hardwareCount);
}

static bool isWouldBlockErrno(int errorCode) {
    return errorCode == EAGAIN || errorCode == EWOULDBLOCK;
}

static std::string makeErrnoMessage(const char* prefix, int errorCode) {
    std::string message(prefix == nullptr ? "errno" : prefix);
    message += ", errno=";
    message += std::to_string(errorCode);
    message += ", ";
    message += std::strerror(errorCode);
    return message;
}

} // namespace

struct TelnetManager::RequestContext {
    enum class TelnetState {
        DATA,
        IAC,
        OPTION,
        SUBNEG,
        SUBNEG_IAC,
    };

    explicit RequestContext(const Request& inRequest,
        RequestId inRequestId,
        TelnetManager* inOwner)
        : request(inRequest),
          requestId(inRequestId),
          owner(inOwner),
          cancelRequested(false),
          started(false),
          completed(false),
          socketFd(-1),
          currentCommandIndex(0),
          telnetState(TelnetState::DATA),
          pendingTelnetCommand(0),
          lastDataCallbackMs(0) { }

    ~RequestContext() {
        TelnetManager::closeContextSocket(this);
    }

    Request request;
    RequestId requestId;
    TelnetManager* owner;
    std::atomic<bool> cancelRequested;
    std::atomic<bool> started;
    std::atomic<bool> completed;
    std::atomic<int> socketFd;

    Response response;
    size_t currentCommandIndex;
    std::string currentCommand;

    TelnetState telnetState;
    unsigned char pendingTelnetCommand;
    std::string matchBuffer;
    int64_t lastDataCallbackMs;
};

struct TelnetManager::MainThreadMessage {
    IEventListener* listener;
    Event event;

    MainThreadMessage() : listener(nullptr) { }
};

TelnetManager::Response::Response()
    : socketErrno(0),
      sentBytes(0),
      receivedBytes(0),
      connected(false),
      loggedIn(false),
      timedOut(false),
      cancelled(false),
      peerClosed(false),
      outputTruncated(false) { }

bool TelnetManager::Response::isSuccess() const {
    return connected && !timedOut && !cancelled && socketErrno == 0 && errorMessage.empty();
}

TelnetManager::Event::Event()
    : type(EventType::COMPLETED),
      requestId(0),
      port(23),
      opaque(nullptr),
      commandIndex(0) { }

TelnetManager::Request::Request()
    : port(23),
      opaque(nullptr),
      listener(nullptr),
      loginPrompt("login:"),
      passwordPrompt("Password:"),
      lineEnding("\r\n"),
      connectTimeoutMs(5000),
      loginTimeoutMs(10000),
      commandTimeoutMs(10000),
      initialReadTimeoutMs(1000),
      readIdleTimeoutMs(300),
      sendTimeoutMs(5000),
      dataIntervalMs(0),
      storeOutput(true),
      emitData(false),
      replyTelnetNegotiation(true),
      tcpNoDelay(true),
      closeAfterCompleted(true),
      maxOutputBytes(64 * 1024) { }

TelnetManager::Request TelnetManager::Request::CreateCommand(const std::string& host,
    const std::string& command,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
    uint16_t port) {
    Request request;
    request.host = host;
    request.port = port;
    request.commands.push_back(command);
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    return request;
}

TelnetManager::Request TelnetManager::Request::CreateCommands(const std::string& host,
    const std::vector<std::string>& commands,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
    uint16_t port) {
    Request request;
    request.host = host;
    request.port = port;
    request.commands = commands;
    request.listener = listener;
    request.tag = tag;
    request.opaque = opaque;
    return request;
}

TelnetManager::Request TelnetManager::Request::CreateLoginCommand(const std::string& host,
    const std::string& username,
    const std::string& password,
    const std::string& command,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
    uint16_t port) {
    Request request = CreateCommand(host, command, listener, tag, opaque, port);
    request.username = username;
    request.password = password;
    return request;
}

TelnetManager::Request TelnetManager::Request::CreateLoginCommands(const std::string& host,
    const std::string& username,
    const std::string& password,
    const std::vector<std::string>& commands,
    IEventListener* listener,
    const std::string& tag,
    void* opaque,
    uint16_t port) {
    Request request = CreateCommands(host, commands, listener, tag, opaque, port);
    request.username = username;
    request.password = password;
    return request;
}

TelnetManager::TelnetManager()
    : mInitialized(false),
      mMainLooper(nullptr),
      mWakeFd(-1),
      mStopping(false),
      mNextRequestId(1),
      mMaxWorkerCount(0),
      mWorkerIdleExitMs(0),
      mAliveWorkerCount(0),
      mIdleWorkerCount(0) { }

TelnetManager::~TelnetManager() {
    shutdown();
}

void TelnetManager::validateRequestOrDie(const Request& request) {
    FailFast(request.host.empty(), "request.host is empty");
    FailFast(request.port == 0, "request.port is invalid, host=%s", request.host.c_str());
    FailFast(request.listener == nullptr,
        "request.listener is null, host=%s", request.host.c_str());
    FailFast(request.connectTimeoutMs <= 0,
        "connectTimeoutMs must be > 0, host=%s", request.host.c_str());
    FailFast(request.loginTimeoutMs <= 0,
        "loginTimeoutMs must be > 0, host=%s", request.host.c_str());
    FailFast(request.commandTimeoutMs <= 0,
        "commandTimeoutMs must be > 0, host=%s", request.host.c_str());
    FailFast(request.initialReadTimeoutMs < 0,
        "initialReadTimeoutMs must be >= 0, host=%s", request.host.c_str());
    FailFast(request.readIdleTimeoutMs <= 0,
        "readIdleTimeoutMs must be > 0, host=%s", request.host.c_str());
    FailFast(request.sendTimeoutMs <= 0,
        "sendTimeoutMs must be > 0, host=%s", request.host.c_str());
    FailFast(request.dataIntervalMs < 0,
        "dataIntervalMs must be >= 0, host=%s", request.host.c_str());
    FailFast(request.lineEnding.empty(),
        "lineEnding is empty, host=%s", request.host.c_str());

    if (isLoginRequired(request)) {
        FailFast(!request.username.empty() && request.loginPrompt.empty(),
            "loginPrompt is empty but username is set, host=%s", request.host.c_str());
        FailFast(!request.password.empty() && request.passwordPrompt.empty(),
            "passwordPrompt is empty but password is set, host=%s", request.host.c_str());
    }
}

void TelnetManager::init(cdroid::Looper* mainLooper,
    size_t maxWorkerCount,
    long workerIdleExitMs) {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    FailFast(mInitialized.load(), "TelnetManager::init called twice");

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

    mWakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    FailFast(mWakeFd < 0, "eventfd failed, errno=%d", errno);

    const int addResult = mMainLooper->addFd(mWakeFd, 0, cdroid::Looper::EVENT_INPUT, onMainThreadWake, this);
    FailFast(addResult == 0, "Looper::addFd failed, fd=%d", mWakeFd);

    mInitialized.store(true);
}

void TelnetManager::shutdown() {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    if (!mInitialized.load()) {
        return;
    }

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

    mMainLooper = nullptr;
    mStopping.store(false);
    mInitialized.store(false);
}

bool TelnetManager::isInitialized() const {
    return mInitialized.load();
}

TelnetManager::RequestId TelnetManager::submit(const Request& request) {
    FailFast(!mInitialized.load(), "TelnetManager is not initialized, call init() first");
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

bool TelnetManager::cancel(RequestId requestId) {
    FailFast(!mInitialized.load(), "TelnetManager is not initialized, call init() first");

    std::shared_ptr<RequestContext> cancelledContext;

    {
        std::lock_guard<std::mutex> lock(mRequestMutex);

        for (auto it = mPendingRequests.begin(); it != mPendingRequests.end(); ++it) {
            if ((*it)->requestId == requestId) {
                cancelledContext = *it;
                cancelledContext->cancelRequested.store(true);
                mPendingRequests.erase(it);
                break;
            }
        }

        if (!cancelledContext) {
            auto it = mRunningRequests.find(requestId);
            if (it != mRunningRequests.end()) {
                it->second->cancelRequested.store(true);
                closeContextSocket(it->second.get());
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

size_t TelnetManager::cancelByTag(const std::string& tag) {
    FailFast(!mInitialized.load(), "TelnetManager is not initialized, call init() first");

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
                closeContextSocket(it->second.get());
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

void TelnetManager::cancelAll() {
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
            closeContextSocket(it->second.get());
        }
    }

    for (size_t i = 0; i < pendingCancelled.size(); ++i) {
        pendingCancelled[i]->response.cancelled = true;
        pendingCancelled[i]->response.errorMessage = "cancelled before execution";
        postEvent(pendingCancelled[i], EventType::CANCELLED);
    }
}

size_t TelnetManager::pendingCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size();
}

size_t TelnetManager::runningCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mRunningRequests.size();
}

size_t TelnetManager::requestCount() const {
    if (!mInitialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mRequestMutex);
    return mPendingRequests.size() + mRunningRequests.size();
}

bool TelnetManager::contains(RequestId requestId) const {
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

void TelnetManager::ensureWorkerCapacityLocked() {
    FailFast(!mInitialized.load(), "ensureWorkerCapacityLocked called before init");

    size_t neededWorkers = 0;
    if (mPendingRequests.size() > mIdleWorkerCount) {
        neededWorkers = mPendingRequests.size() - mIdleWorkerCount;
    }

    while (neededWorkers > 0 && mAliveWorkerCount < mMaxWorkerCount) {
        ++mAliveWorkerCount;
        std::thread(&TelnetManager::workerLoop, this).detach();
        --neededWorkers;
    }
}

void TelnetManager::onWorkerExitLocked() {
    FailFast(mAliveWorkerCount <= 0, "mAliveWorkerCount underflow");
    --mAliveWorkerCount;
    mWorkerExitCond.notify_all();
}

int TelnetManager::onMainThreadWake(int fd, int /*events*/, void* context) {
    uint64_t value = 0;
    while (read(fd, &value, sizeof(value)) > 0) {
    }

    TelnetManager* manager = static_cast<TelnetManager*>(context);
    manager->drainMainThreadMessages();
    return 1;
}

void TelnetManager::wakeMainThread() {
    if (mWakeFd < 0) {
        return;
    }

    const uint64_t value = 1;
    const ssize_t rc = write(mWakeFd, &value, sizeof(value));
    FailFast(rc < 0 && errno != EAGAIN, "write(eventfd) failed, errno=%d", errno);
}

void TelnetManager::drainMainThreadMessages() {
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
        message.listener->onTelnetEvent(message.event);
    }
}

void TelnetManager::postEvent(const std::shared_ptr<RequestContext>& requestContext,
    EventType type) {
    postEvent(requestContext.get(), type);
}

void TelnetManager::postEvent(RequestContext* requestContext, EventType type) {
    FailFast(requestContext == nullptr, "requestContext is null");

    MainThreadMessage message;
    message.listener = requestContext->request.listener;
    message.event.type = type;
    message.event.requestId = requestContext->requestId;
    message.event.host = requestContext->request.host;
    message.event.port = requestContext->request.port;
    message.event.tag = requestContext->request.tag;
    message.event.opaque = requestContext->request.opaque;
    message.event.commandIndex = requestContext->currentCommandIndex;
    message.event.command = requestContext->currentCommand;
    message.event.response = requestContext->response;

    {
        std::lock_guard<std::mutex> lock(mMainThreadMutex);
        mMainThreadMessages.push_back(message);
    }
    wakeMainThread();
}

void TelnetManager::postDataEvent(RequestContext* requestContext, const std::string& data) {
    if (!requestContext->request.emitData || data.empty()) {
        return;
    }

    const int64_t now = nowMs();
    if (requestContext->request.dataIntervalMs > 0 &&
        requestContext->lastDataCallbackMs > 0 &&
        now - requestContext->lastDataCallbackMs < requestContext->request.dataIntervalMs) {
        return;
    }

    requestContext->lastDataCallbackMs = now;
    requestContext->response.lastData = data;
    postEvent(requestContext, EventType::DATA);
}

void TelnetManager::workerLoop() {
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

void TelnetManager::executeRequest(const std::shared_ptr<RequestContext>& requestContext) {
    bool ok = connectSocket(requestContext.get());
    if (requestContext->cancelRequested.load()) {
        requestContext->response.cancelled = true;
        if (requestContext->response.errorMessage.empty()) {
            requestContext->response.errorMessage = "cancelled";
        }
        postEvent(requestContext, EventType::CANCELLED);
        requestContext->completed.store(true);
        closeContextSocket(requestContext.get());
        return;
    }

    if (!ok) {
        postEvent(requestContext, EventType::COMPLETED);
        requestContext->completed.store(true);
        closeContextSocket(requestContext.get());
        return;
    }

    requestContext->response.connected = true;
    postEvent(requestContext, EventType::CONNECTED);

    if (isLoginRequired(requestContext->request)) {
        ok = doLogin(requestContext.get());
    } else if (requestContext->request.initialReadTimeoutMs > 0) {
        std::vector<std::string> emptyTokens;
        const ReadStatus status = readUntil(requestContext.get(),
            emptyTokens,
            requestContext->request.initialReadTimeoutMs,
            requestContext->request.readIdleTimeoutMs,
            false);
        ok = status == ReadStatus::OK || status == ReadStatus::CLOSED;
        if (status == ReadStatus::CANCELLED) {
            requestContext->response.cancelled = true;
        } else if (status == ReadStatus::ERROR) {
            ok = false;
        }
    }

    if (ok && !requestContext->cancelRequested.load()) {
        ok = doCommands(requestContext.get());
    }

    if (requestContext->cancelRequested.load() || requestContext->response.cancelled) {
        requestContext->response.cancelled = true;
        if (requestContext->response.errorMessage.empty()) {
            requestContext->response.errorMessage = "cancelled";
        }
        postEvent(requestContext, EventType::CANCELLED);
    } else {
        postEvent(requestContext, EventType::COMPLETED);
    }

    requestContext->completed.store(true);

    if (requestContext->request.closeAfterCompleted) {
        closeContextSocket(requestContext.get());
    }
}

bool TelnetManager::connectSocket(RequestContext* requestContext) {
    Request& request = requestContext->request;
    Response& response = requestContext->response;

    char portText[16];
    std::snprintf(portText, sizeof(portText), "%u", static_cast<unsigned int>(request.port));

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    const int gaiRc = getaddrinfo(request.host.c_str(), portText, &hints, &result);
    if (gaiRc != 0) {
        response.socketErrno = 0;
        response.errorMessage = std::string("getaddrinfo failed: ") + gai_strerror(gaiRc);
        return false;
    }

    bool connected = false;

    for (struct addrinfo* ai = result; ai != nullptr && !connected; ai = ai->ai_next) {
        if (requestContext->cancelRequested.load()) {
            break;
        }

        int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) {
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("socket failed", errno);
            continue;
        }

        requestContext->socketFd.store(fd);

        if (request.tcpNoDelay) {
            int yes = 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        }

        if (!setNonBlocking(fd, true)) {
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("fcntl(O_NONBLOCK) failed", errno);
            closeContextSocket(requestContext);
            continue;
        }

        int rc = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (rc == 0) {
            connected = true;
            break;
        }

        if (rc < 0 && errno != EINPROGRESS) {
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("connect failed", errno);
            closeContextSocket(requestContext);
            continue;
        }

        const int64_t start = nowMs();
        for (;;) {
            if (requestContext->cancelRequested.load()) {
                closeContextSocket(requestContext);
                break;
            }

            const int64_t elapsed = nowMs() - start;
            const long remain = request.connectTimeoutMs - static_cast<long>(elapsed);
            if (remain <= 0) {
                response.timedOut = true;
                response.socketErrno = ETIMEDOUT;
                response.errorMessage = "connect timeout";
                closeContextSocket(requestContext);
                break;
            }

            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(fd, &wfds);

            struct timeval tv;
            tv.tv_sec = remain / 1000;
            tv.tv_usec = (remain % 1000) * 1000;

            rc = select(fd + 1, nullptr, &wfds, nullptr, &tv);
            if (rc < 0) {
                if (errno == EINTR) {
                    continue;
                }
                response.socketErrno = errno;
                response.errorMessage = makeErrnoMessage("select(connect) failed", errno);
                closeContextSocket(requestContext);
                break;
            }

            if (rc == 0) {
                response.timedOut = true;
                response.socketErrno = ETIMEDOUT;
                response.errorMessage = "connect timeout";
                closeContextSocket(requestContext);
                break;
            }

            int soError = 0;
            socklen_t soErrorLen = sizeof(soError);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soError, &soErrorLen) != 0) {
                response.socketErrno = errno;
                response.errorMessage = makeErrnoMessage("getsockopt(SO_ERROR) failed", errno);
                closeContextSocket(requestContext);
                break;
            }

            if (soError != 0) {
                response.socketErrno = soError;
                response.errorMessage = makeErrnoMessage("connect failed", soError);
                closeContextSocket(requestContext);
                break;
            }

            connected = true;
            break;
        }
    }

    freeaddrinfo(result);

    if (requestContext->cancelRequested.load()) {
        response.cancelled = true;
        if (response.errorMessage.empty()) {
            response.errorMessage = "cancelled";
        }
        return false;
    }

    if (!connected && response.errorMessage.empty()) {
        response.errorMessage = "connect failed";
    }

    return connected;
}

bool TelnetManager::doLogin(RequestContext* requestContext) {
    Request& request = requestContext->request;
    Response& response = requestContext->response;

    if (!request.username.empty()) {
        std::vector<std::string> loginTokens;
        loginTokens.push_back(request.loginPrompt);
        ReadStatus status = readUntil(requestContext,
            loginTokens,
            request.loginTimeoutMs,
            request.readIdleTimeoutMs,
            true);
        if (status != ReadStatus::OK) {
            if (status == ReadStatus::CANCELLED) {
                response.cancelled = true;
            } else {
                response.errorMessage = std::string("wait login prompt failed: ") + readStatusToString(status);
                response.timedOut = status == ReadStatus::TIMEOUT;
            }
            return false;
        }

        if (!sendLine(requestContext, request.username, true)) {
            return false;
        }
    }

    if (!request.password.empty()) {
        std::vector<std::string> passwordTokens;
        passwordTokens.push_back(request.passwordPrompt);
        ReadStatus status = readUntil(requestContext,
            passwordTokens,
            request.loginTimeoutMs,
            request.readIdleTimeoutMs,
            true);
        if (status != ReadStatus::OK) {
            if (status == ReadStatus::CANCELLED) {
                response.cancelled = true;
            } else {
                response.errorMessage = std::string("wait password prompt failed: ") + readStatusToString(status);
                response.timedOut = status == ReadStatus::TIMEOUT;
            }
            return false;
        }

        if (!sendLine(requestContext, request.password, true)) {
            return false;
        }
    }

    std::vector<std::string> promptTokens = makePromptTokens(request);
    if (!promptTokens.empty()) {
        const ReadStatus status = readUntil(requestContext,
            promptTokens,
            request.loginTimeoutMs,
            request.readIdleTimeoutMs,
            true);
        if (status != ReadStatus::OK) {
            if (status == ReadStatus::CANCELLED) {
                response.cancelled = true;
            } else {
                response.errorMessage = std::string("wait shell prompt failed: ") + readStatusToString(status);
                response.timedOut = status == ReadStatus::TIMEOUT;
            }
            return false;
        }
    } else {
        const ReadStatus status = readUntil(requestContext,
            promptTokens,
            request.loginTimeoutMs,
            request.readIdleTimeoutMs,
            false);
        if (status == ReadStatus::CANCELLED) {
            response.cancelled = true;
            return false;
        }
        if (status == ReadStatus::ERROR) {
            return false;
        }
    }

    response.loggedIn = true;
    postEvent(requestContext, EventType::LOGGED_IN);
    return true;
}

bool TelnetManager::doCommands(RequestContext* requestContext) {
    Request& request = requestContext->request;
    Response& response = requestContext->response;
    const std::vector<std::string> promptTokens = makePromptTokens(request);
    const bool requirePrompt = !promptTokens.empty();

    for (size_t i = 0; i < request.commands.size(); ++i) {
        if (requestContext->cancelRequested.load()) {
            response.cancelled = true;
            return false;
        }

        requestContext->currentCommandIndex = i;
        requestContext->currentCommand = request.commands[i];

        if (!sendLine(requestContext, request.commands[i], true)) {
            return false;
        }
        postEvent(requestContext, EventType::COMMAND_SENT);

        const ReadStatus status = readUntil(requestContext,
            promptTokens,
            request.commandTimeoutMs,
            request.readIdleTimeoutMs,
            requirePrompt);

        if (status == ReadStatus::OK || (!requirePrompt && status == ReadStatus::CLOSED)) {
            continue;
        }

        if (status == ReadStatus::CANCELLED) {
            response.cancelled = true;
        } else {
            response.errorMessage = std::string("read command output failed: ") + readStatusToString(status);
            response.timedOut = status == ReadStatus::TIMEOUT;
        }
        return false;
    }

    return true;
}

bool TelnetManager::sendLine(RequestContext* requestContext,
    const std::string& line,
    bool countAsBusinessBytes) {
    std::string data = line;
    data += requestContext->request.lineEnding;
    return sendRaw(requestContext,
        data.data(),
        data.size(),
        requestContext->request.sendTimeoutMs,
        countAsBusinessBytes);
}

bool TelnetManager::sendRaw(RequestContext* requestContext,
    const char* data,
    size_t size,
    long timeoutMs,
    bool countAsBusinessBytes) {
    FailFast(requestContext == nullptr, "requestContext is null");
    if (data == nullptr || size == 0) {
        return true;
    }

    Response& response = requestContext->response;
    size_t offset = 0;
    const int64_t start = nowMs();

    while (offset < size) {
        if (requestContext->cancelRequested.load()) {
            response.cancelled = true;
            response.errorMessage = "cancelled";
            return false;
        }

        const int fd = requestContext->socketFd.load();
        if (fd < 0) {
            response.socketErrno = EBADF;
            response.errorMessage = "socket closed";
            return false;
        }

        const int64_t elapsed = nowMs() - start;
        const long remain = timeoutMs - static_cast<long>(elapsed);
        if (remain <= 0) {
            response.timedOut = true;
            response.socketErrno = ETIMEDOUT;
            response.errorMessage = "send timeout";
            return false;
        }

        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);

        struct timeval tv;
        tv.tv_sec = remain / 1000;
        tv.tv_usec = (remain % 1000) * 1000;

        const int rc = select(fd + 1, nullptr, &wfds, nullptr, &tv);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (requestContext->cancelRequested.load()) {
                response.cancelled = true;
                response.errorMessage = "cancelled";
                return false;
            }
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("select(send) failed", errno);
            return false;
        }

        if (rc == 0) {
            response.timedOut = true;
            response.socketErrno = ETIMEDOUT;
            response.errorMessage = "send timeout";
            return false;
        }

        const ssize_t n = send(fd, data + offset, size - offset, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR || isWouldBlockErrno(errno)) {
                continue;
            }
            if (requestContext->cancelRequested.load()) {
                response.cancelled = true;
                response.errorMessage = "cancelled";
                return false;
            }
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("send failed", errno);
            return false;
        }

        if (n == 0) {
            response.peerClosed = true;
            response.errorMessage = "send returned 0";
            return false;
        }

        offset += static_cast<size_t>(n);
        if (countAsBusinessBytes) {
            response.sentBytes += static_cast<size_t>(n);
        }
    }

    return true;
}

TelnetManager::ReadStatus TelnetManager::readUntil(RequestContext* requestContext,
    const std::vector<std::string>& stopTokens,
    long totalTimeoutMs,
    long idleTimeoutMs,
    bool requireStopToken) {
    FailFast(requestContext == nullptr, "requestContext is null");

    Response& response = requestContext->response;
    const int64_t start = nowMs();
    int64_t lastDataMs = start;

    for (;;) {
        if (requestContext->cancelRequested.load()) {
            response.cancelled = true;
            return ReadStatus::CANCELLED;
        }

        if (!stopTokens.empty() && hasAnyStopToken(requestContext, stopTokens)) {
            return ReadStatus::OK;
        }

        const int64_t now = nowMs();
        const long totalRemain = totalTimeoutMs - static_cast<long>(now - start);
        if (totalRemain <= 0) {
            response.timedOut = true;
            response.socketErrno = ETIMEDOUT;
            return requireStopToken ? ReadStatus::TIMEOUT : ReadStatus::OK;
        }

        const long idleRemain = idleTimeoutMs - static_cast<long>(now - lastDataMs);
        if (!requireStopToken && idleRemain <= 0) {
            return ReadStatus::OK;
        }

        const long waitMs = std::max<long>(1, std::min<long>(100, std::min<long>(totalRemain, idleRemain > 0 ? idleRemain : totalRemain)));

        const int fd = requestContext->socketFd.load();
        if (fd < 0) {
            return requestContext->cancelRequested.load() ? ReadStatus::CANCELLED : ReadStatus::CLOSED;
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        struct timeval tv;
        tv.tv_sec = waitMs / 1000;
        tv.tv_usec = (waitMs % 1000) * 1000;

        const int rc = select(fd + 1, &rfds, nullptr, nullptr, &tv);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (requestContext->cancelRequested.load()) {
                response.cancelled = true;
                return ReadStatus::CANCELLED;
            }
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("select(recv) failed", errno);
            return ReadStatus::ERROR;
        }

        if (rc == 0) {
            continue;
        }

        char buffer[512];
        const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n < 0) {
            if (errno == EINTR || isWouldBlockErrno(errno)) {
                continue;
            }
            if (requestContext->cancelRequested.load()) {
                response.cancelled = true;
                return ReadStatus::CANCELLED;
            }
            response.socketErrno = errno;
            response.errorMessage = makeErrnoMessage("recv failed", errno);
            return ReadStatus::ERROR;
        }

        if (n == 0) {
            response.peerClosed = true;
            return requireStopToken ? ReadStatus::CLOSED : ReadStatus::OK;
        }

        response.receivedBytes += static_cast<size_t>(n);
        appendIncomingData(requestContext, buffer, static_cast<size_t>(n));
        lastDataMs = nowMs();
    }
}

void TelnetManager::appendIncomingData(RequestContext* requestContext,
    const char* data,
    size_t size) {
    if (data == nullptr || size == 0) {
        return;
    }

    std::string plain;
    plain.reserve(size);

    const int fd = requestContext->socketFd.load();

    for (size_t i = 0; i < size; ++i) {
        const unsigned char c = static_cast<unsigned char>(data[i]);

        switch (requestContext->telnetState) {
        case RequestContext::TelnetState::DATA:
            if (c == TELNET_IAC) {
                requestContext->telnetState = RequestContext::TelnetState::IAC;
            } else {
                plain.push_back(static_cast<char>(c));
            }
            break;

        case RequestContext::TelnetState::IAC:
            if (c == TELNET_IAC) {
                plain.push_back(static_cast<char>(TELNET_IAC));
                requestContext->telnetState = RequestContext::TelnetState::DATA;
            } else if (c == TELNET_DO || c == TELNET_DONT || c == TELNET_WILL || c == TELNET_WONT) {
                requestContext->pendingTelnetCommand = c;
                requestContext->telnetState = RequestContext::TelnetState::OPTION;
            } else if (c == TELNET_SB) {
                requestContext->telnetState = RequestContext::TelnetState::SUBNEG;
            } else {
                requestContext->telnetState = RequestContext::TelnetState::DATA;
            }
            break;

        case RequestContext::TelnetState::OPTION:
            if (requestContext->request.replyTelnetNegotiation && fd >= 0) {
                unsigned char reply[3];
                reply[0] = TELNET_IAC;
                reply[1] = (requestContext->pendingTelnetCommand == TELNET_DO ||
                    requestContext->pendingTelnetCommand == TELNET_DONT) ? TELNET_WONT : TELNET_DONT;
                reply[2] = c;
                (void)send(fd, reinterpret_cast<const char*>(reply), sizeof(reply), MSG_NOSIGNAL);
            }
            requestContext->pendingTelnetCommand = 0;
            requestContext->telnetState = RequestContext::TelnetState::DATA;
            break;

        case RequestContext::TelnetState::SUBNEG:
            if (c == TELNET_IAC) {
                requestContext->telnetState = RequestContext::TelnetState::SUBNEG_IAC;
            }
            break;

        case RequestContext::TelnetState::SUBNEG_IAC:
            if (c == TELNET_SE) {
                requestContext->telnetState = RequestContext::TelnetState::DATA;
            } else {
                requestContext->telnetState = RequestContext::TelnetState::SUBNEG;
            }
            break;
        }
    }

    if (!plain.empty()) {
        appendOutput(requestContext, plain);
        postDataEvent(requestContext, plain);
    }
}

void TelnetManager::appendOutput(RequestContext* requestContext, const std::string& data) {
    if (data.empty()) {
        return;
    }

    requestContext->matchBuffer.append(data);
    if (requestContext->matchBuffer.size() > 4096) {
        requestContext->matchBuffer.erase(0, requestContext->matchBuffer.size() - 4096);
    }

    if (!requestContext->request.storeOutput) {
        return;
    }

    if (requestContext->request.maxOutputBytes == 0) {
        requestContext->response.output.append(data);
        return;
    }

    const size_t currentSize = requestContext->response.output.size();
    if (currentSize >= requestContext->request.maxOutputBytes) {
        requestContext->response.outputTruncated = true;
        return;
    }

    const size_t remain = requestContext->request.maxOutputBytes - currentSize;
    if (data.size() <= remain) {
        requestContext->response.output.append(data);
    } else {
        requestContext->response.output.append(data.data(), remain);
        requestContext->response.outputTruncated = true;
    }
}

bool TelnetManager::hasAnyStopToken(RequestContext* requestContext,
    const std::vector<std::string>& stopTokens) const {
    for (size_t i = 0; i < stopTokens.size(); ++i) {
        if (!stopTokens[i].empty() &&
            requestContext->matchBuffer.find(stopTokens[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int64_t TelnetManager::nowMs() {
    return cdroid::SystemClock::uptimeMillis();
}

bool TelnetManager::setNonBlocking(int fd, bool nonBlocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    if (nonBlocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(fd, F_SETFL, flags) == 0;
}

void TelnetManager::closeContextSocket(RequestContext* requestContext) {
    if (requestContext == nullptr) {
        return;
    }

    const int fd = requestContext->socketFd.exchange(-1);
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

bool TelnetManager::isLoginRequired(const Request& request) {
    return !request.username.empty() || !request.password.empty();
}

std::vector<std::string> TelnetManager::makePromptTokens(const Request& request) {
    std::vector<std::string> tokens;
    if (!request.prompt.empty()) {
        tokens.push_back(request.prompt);
    }
    return tokens;
}

const char* TelnetManager::readStatusToString(ReadStatus status) {
    switch (status) {
    case ReadStatus::OK:
        return "ok";
    case ReadStatus::TIMEOUT:
        return "timeout";
    case ReadStatus::CANCELLED:
        return "cancelled";
    case ReadStatus::CLOSED:
        return "peer closed";
    case ReadStatus::ERROR:
        return "socket error";
    }
    return "unknown";
}
