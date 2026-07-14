/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-24 09:40:23
 * @LastEditTime: 2026-07-14 23:03:53
 * @FilePath: /kk_frame/src/app/managers/thread_mgr.cc
 * @Description: 固定 worker、eventfd 主线程分发、可取消且可安全关闭的线程池
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "thread_mgr.h"
#include "quick_define.h"

#include <core/systemclock.h>
#include <cdlog.h>
#include <algorithm>
#include <climits>
#include <errno.h>
#include <exception>
#include <inttypes.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

using cdroid::Looper;
using cdroid::SystemClock;

ThreadPool::ThreadPool() = default;

ThreadPool::~ThreadPool() {
    shutdown();
}

/// @brief 启动固定数量的 worker 线程
/// @param count worker 线程数，必须大于0
/// @note 重复初始化、shutdown() 后重启或任何资源初始化失败都会 FailFast。
void ThreadPool::init(int count) {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    std::lock_guard<std::mutex> lock(mMutex);

    FailFast(mIsInit,     "ThreadPool::init called twice");
    FailFast(mTerminated, "ThreadPool cannot restart after shutdown");
    FailFast(count <= 0,  "worker count must be > 0, count=%d", count);

    mStopping = false;
    mCount = count;
    mMainLooper = Looper::getMainLooper();
    FailFast(mMainLooper == nullptr, "main looper is null");

    const int wakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    FailFast(wakeFd < 0, "eventfd failed, errno=%d err=%s", errno, strerror(errno));

    const int addResult = mMainLooper->addFd(wakeFd, 0, Looper::EVENT_INPUT, onWake, this);
    FailFast(addResult <= 0, "Looper::addFd failed, fd=%d result=%d", wakeFd, addResult);

    {
        std::lock_guard<std::mutex> wakeLock(mWakeMutex);
        mWakeFd = wakeFd;
    }

    try {
        mWorkers.reserve(static_cast<size_t>(mCount));
        for (int i = 0; i < mCount; ++i) {
            mWorkers.emplace_back(&ThreadPool::workerLoop, this, static_cast<size_t>(i));
        }
    }
    catch (const std::exception& e) {
        FailFast(true, "worker creation failed: %s", e.what());
    }
    catch (...) {
        FailFast(true, "worker creation failed with unknown exception");
    }

    mIsInit = true;
}

/// @brief 关闭线程池并等待所有 worker 退出
/// @note 关闭后不可重新 init()；正在执行的 onTask() 会被等待至自然返回。
void ThreadPool::shutdown() {
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
    Looper* mainLooper = nullptr;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mIsInit && mWorkers.empty()) return;

        mIsInit = false;
        mStopping = true;
        mTerminated = true;
        mainLooper = mMainLooper;
        for (auto& item : mTasks) item.second->canceled.store(true);

        for (const TaskPtr& task : mWaitTasks) finishTaskLocked(task);
        for (const TaskPtr& task : mMainTasks) finishTaskLocked(task);
        mWaitTasks.clear();
        mMainTasks.clear();
    }

    mWorkerCv.notify_all();
    mTaskStateCv.notify_all();
    for (auto& worker : mWorkers) {
        if (worker.joinable()) worker.join();
    }
    mWorkers.clear();

    {
        std::lock_guard<std::mutex> wakeLock(mWakeMutex);
        if (mainLooper != nullptr && mWakeFd >= 0) {
            mainLooper->removeFd(mWakeFd);
        }
        if (mWakeFd >= 0) {
            close(mWakeFd);
            mWakeFd = -1;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto& item : mTasks) {
            item.second->phase = TaskPhase::Finished;
        }
        mTasks.clear();
        mStopping = false;
        mCount = 0;
        mMainLooper = nullptr;
    }
    mTaskStateCv.notify_all();
}

/// @brief 添加异步任务
/// @param sink 任务回调对象
/// @param data 透传给回调的用户数据
/// @return 大于0-任务ID，-1-参数无效或线程池不可用
int ThreadPool::add(ThreadTask* sink, void* data) {
    if (!sink) return -1;

    TaskPtr task(new TaskData());
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mIsInit || mStopping) return -1;

        task->id = nextTaskIdLocked();
        task->atime = SystemClock::uptimeMillis();
        task->sink = sink;
        task->data = data;
        task->phase = TaskPhase::Queued;
        mTasks.emplace(task->id, task);
        mWaitTasks.push_back(task);
    }

    LOGV("task add. id=%d", task->id);
    mWorkerCv.notify_one();
    return task->id;
}

/// @brief 取消任务并等待正在运行的回调返回
/// @param taskId 任务ID
/// @return 0-取消成功，-1-任务不存在，-2-当前正处于该任务的回调中
int ThreadPool::del(int taskId) {
    std::unique_lock<std::mutex> lock(mMutex);
    auto found = mTasks.find(taskId);
    if (found == mTasks.end()) return -1;

    const TaskPtr task = found->second;
    task->canceled.store(true);

    if ((task->phase == TaskPhase::Running || task->phase == TaskPhase::MainRunning) &&
        task->activeThread == std::this_thread::get_id()) {
        LOGE("task cannot wait for cancellation from its own callback. id=%d", taskId);
        return -2;
    }

    while (true) {
        switch (task->phase) {
        case TaskPhase::Queued:
            removeQueuedTaskLocked(task);
            finishTaskLocked(task);
            LOGV("cancel queued task. id=%d", taskId);
            mTaskStateCv.notify_all();
            return 0;

        case TaskPhase::Completed:
            removeQueuedTaskLocked(task);
            finishTaskLocked(task);
            LOGV("cancel completed task. id=%d", taskId);
            mTaskStateCv.notify_all();
            return 0;

        case TaskPhase::Finished:
            mTasks.erase(taskId);
            return 0;

        case TaskPhase::Running:
        case TaskPhase::MainRunning:
            // onTask/onMain uses the raw sink. Wait until it has returned before
            // allowing the owner to destroy sink/data.
            mTaskStateCv.wait(lock, [&task]() {
                return task->phase != TaskPhase::Running &&
                    task->phase != TaskPhase::MainRunning;
            });
            break;
        }
    }
}

/// @brief 在主线程处理已完成任务
/// @return 本次已处理的回调数量
/// @note 单次最多处理3个任务；累计超过500ms时提前让出主线程。
int ThreadPool::dispatchMainTasks() {
    const int64_t startTime = SystemClock::uptimeMillis();
    int handled = 0;

    while (handled < 3) {
        TaskPtr task;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mMainTasks.empty()) break;
            task = mMainTasks.front();
            mMainTasks.pop_front();

            if (task->canceled.load() || mStopping) {
                finishTaskLocked(task);
                mTaskStateCv.notify_all();
                continue;
            }
            task->phase = TaskPhase::MainRunning;
            task->activeThread = std::this_thread::get_id();
        }

        LOGV("task main in. id=%d time=%" PRId64,
            task->id, startTime - task->atime);
        try {
            task->sink->onMain(task->id, task->data);
        }
        catch (const std::exception& e) {
            LOGE("task main exception. id=%d what=%s", task->id, e.what());
        }
        catch (...) {
            LOGE("task main unknown exception. id=%d", task->id);
        }
        LOGV("task main out. id=%d", task->id);

        {
            std::lock_guard<std::mutex> lock(mMutex);
            finishTaskLocked(task);
        }
        mTaskStateCv.notify_all();
        ++handled;

        const int64_t elapsed = SystemClock::uptimeMillis() - startTime;
        if (elapsed > 500) {
            LOGW("Task on main more time. %dms", static_cast<int>(elapsed));
            break;
        }
    }

    bool hasPendingTask = false;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        hasPendingTask = !mStopping && !mMainTasks.empty();
    }
    if (hasPendingTask) wakeMainThread();

    return handled;
}

/// @brief FD可读时在主 Looper 线程处理完成任务
/// @param fd 唤醒FD
/// @param events Looper 事件标记
/// @param context ThreadPool 实例
/// @return 1-继续监听该FD
int ThreadPool::onWake(int fd, int events, void* context) {
    ThreadPool* pool = static_cast<ThreadPool*>(context);
    if (pool == nullptr) return 0;

    if ((events & Looper::EVENT_INPUT) == 0) {
        LOGW("ThreadPool wake fd unexpected event. fd=%d events=%d", fd, events);
        return 1;
    }

    pool->drainWakeFd();
    pool->dispatchMainTasks();
    return 1;
}

/// @brief 通过 eventfd 唤醒主 Looper
void ThreadPool::wakeMainThread() {
    std::lock_guard<std::mutex> wakeLock(mWakeMutex);
    if (mWakeFd < 0) return;

    const uint64_t value = 1;
    ssize_t result = 0;
    do {
        result = write(mWakeFd, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);

    if (result < 0 && errno != EAGAIN) {
        LOGE("ThreadPool wake failed. errno=%d err=%s", errno, strerror(errno));
    }
}

/// @brief 读取并清空 eventfd 计数器
void ThreadPool::drainWakeFd() {
    std::lock_guard<std::mutex> wakeLock(mWakeMutex);
    if (mWakeFd < 0) return;

    uint64_t value = 0;
    ssize_t result = 0;
    do {
        result = read(mWakeFd, &value, sizeof(value));
    } while (result < 0 && errno == EINTR);

    if (result < 0 && errno != EAGAIN) {
        LOGE("ThreadPool wake drain failed. errno=%d err=%s", errno, strerror(errno));
    }
}

/// @brief worker 线程主循环
/// @param workerIndex worker 索引，仅用于日志
/// @note onTask() 返回0时转入主线程队列，返回非0时重新排队。
void ThreadPool::workerLoop(size_t workerIndex) {
    LOGI("new worker. index=%zu", workerIndex);

    while (true) {
        TaskPtr task;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mWorkerCv.wait(lock, [this]() {
                return mStopping || !mWaitTasks.empty();
            });
            if (mStopping && mWaitTasks.empty()) break;

            task = mWaitTasks.front();
            mWaitTasks.pop_front();
            if (task->canceled.load()) {
                finishTaskLocked(task);
                mTaskStateCv.notify_all();
                continue;
            }
            task->phase = TaskPhase::Running;
            task->activeThread = std::this_thread::get_id();
        }

        int result = 0;
        LOGV("task worker in. id=%d worker=%zu", task->id, workerIndex);
        try {
            result = task->sink->onTask(task->id, task->data);
        }
        catch (const std::exception& e) {
            LOGE("task worker exception. id=%d what=%s", task->id, e.what());
            result = 0;
        }
        catch (...) {
            LOGE("task worker unknown exception. id=%d", task->id);
            result = 0;
        }
        LOGV("task worker out. id=%d ret=%d", task->id, result);

        bool wakeMain = false;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (task->canceled.load() || mStopping) {
                finishTaskLocked(task);
                mTaskStateCv.notify_all();
            } else if (result == 0) {
                task->phase = TaskPhase::Completed;
                task->activeThread = std::thread::id();
                mMainTasks.push_back(task);
                wakeMain = true;
                mTaskStateCv.notify_all();
            } else {
                // Retry later and yield this worker to other queued tasks.
                task->phase = TaskPhase::Queued;
                task->activeThread = std::thread::id();
                mWaitTasks.push_back(task);
                mTaskStateCv.notify_all();
                mWorkerCv.notify_one();
            }
        }
        if (wakeMain) wakeMainThread();
    }

    LOGI("worker exit. index=%zu", workerIndex);
}

/// @brief 结束任务并从任务表移除
/// @param task 待结束任务
/// @note 调用时必须持有 mMutex。
void ThreadPool::finishTaskLocked(const TaskPtr& task) {
    task->phase = TaskPhase::Finished;
    task->activeThread = std::thread::id();
    auto found = mTasks.find(task->id);
    if (found != mTasks.end() && found->second == task) mTasks.erase(found);
}

/// @brief 从 worker/主线程等待队列中移除任务
/// @param task 待移除任务
/// @note 调用时必须持有 mMutex。
void ThreadPool::removeQueuedTaskLocked(const TaskPtr& task) {
    auto removeTask = [&task](std::deque<TaskPtr>& queue) {
        queue.erase(std::remove(queue.begin(), queue.end(), task), queue.end());
    };
    removeTask(mWaitTasks);
    removeTask(mMainTasks);
}

/// @brief 生成当前未使用的正整数任务ID
/// @return 新任务ID
/// @note 调用时必须持有 mMutex；超过 INT_MAX 后从1循环。
int ThreadPool::nextTaskIdLocked() {
    do {
        ++mTaskId;
        if (mTaskId > static_cast<uint64_t>(INT_MAX)) mTaskId = 1;
    } while (mTasks.find(static_cast<int>(mTaskId)) != mTasks.end());
    return static_cast<int>(mTaskId);
}
