/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-08-29 16:14:15
 * @LastEditTime: 2026-07-14 23:01:05
 * @FilePath: /kk_frame/src/app/managers/thread_mgr.h
 * @Description: 线程池管理
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __THREAD_MGR_H__
#define __THREAD_MGR_H__

#include "template/singleton.h"
#include <core/looper.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#define g_threadMgr ThreadPool::instance()

/// @brief 线程池任务接口
/// @note onTask() 在 worker 线程执行，onMain() 在主线程执行。
class ThreadTask {
public:
    virtual ~ThreadTask() = default;

    /// @brief 在 worker 线程执行耗时任务
    /// @param id 任务ID
    /// @param data add() 时传入的用户数据
    /// @return 0-任务完成，非0-重新排队等待下次执行
    virtual int onTask(int id, void* data) = 0;

    /// @brief 任务完成后在主线程执行回调
    /// @param id 任务ID
    /// @param data add() 时传入的用户数据
    virtual void onMain(int id, void* data) = 0;
};

/// @brief 固定 worker 数量的异步任务管理器
/// @note 任务先在 worker 线程调用 onTask()，成功后由主线程调用 onMain()。
/// @note ThreadPool 不接管 sink/data 所指对象的所有权。
class ThreadPool : public Singleton<ThreadPool> {
    friend Singleton<ThreadPool>;

public:
    /// @brief 启动固定数量的 worker 线程
    /// @param count worker 线程数，必须大于0
    /// @note 重复初始化、shutdown() 后重启或资源初始化失败都会 FailFast。
    void init(int count);

    /// @brief 关闭线程池
    /// @note 永久停止接收新任务，取消队列任务，等待运行中回调返回并 join 所有 worker。
    void shutdown();

    /// @brief 添加异步任务
    /// @param sink 任务回调对象，不能为空
    /// @param data 透传给任务回调的用户数据
    /// @return 大于0-任务ID，-1-参数无效或线程池不可用
    /// @note sink/data 至少需存活到 onMain() 完成或 del() 返回。
    int add(ThreadTask* sink, void* data);

    /// @brief 取消任务并建立生命周期屏障。
    /// @param taskId add() 返回的任务ID
    /// @return 0-取消成功，-1-任务不存在，-2-在任务自身回调中取消
    /// @note 返回0时，线程池不会再访问该任务的 sink/data。
    /// @note 返回-2时仅完成取消标记，无法在当前回调内等待自身返回。
    int del(int taskId);

protected:
    ThreadPool();
    ~ThreadPool();

private:
    /// @brief 任务在线程池中的生命周期状态
    enum class TaskPhase {
        Queued,      // 等待 worker 执行
        Running,     // worker 正在执行 onTask()
        Completed,   // onTask() 已完成，等待主线程回调
        MainRunning, // 主线程正在执行 onMain()
        Finished,    // 任务已结束，不再访问 sink/data
    };

    /// @brief 线程池内部任务上下文
    struct TaskData {
        int               id = 0;                    // 任务ID
        int64_t           atime = 0;                 // 入队时间，单位毫秒
        ThreadTask*       sink = nullptr;            // 非拥有的回调对象
        void*             data = nullptr;            // 非拥有的用户数据
        std::atomic<bool> canceled{ false };         // 取消标记，可跨线程读取
        TaskPhase         phase = TaskPhase::Queued; // 由 mMutex 保护
        std::thread::id   activeThread;              // 由 mMutex 保护
    };

    using TaskPtr = std::shared_ptr<TaskData>;

    /// @brief worker 线程主循环
    /// @param workerIndex worker 索引，仅用于日志
    void workerLoop(size_t workerIndex);

    /// @brief eventfd 可读时由主 Looper 调用
    /// @param fd 唤醒FD
    /// @param events Looper 事件标记
    /// @param context ThreadPool 实例
    /// @return 1-继续监听该FD
    static int onWake(int fd, int events, void* context);

    /// @brief 通过 eventfd 唤醒主 Looper
    void wakeMainThread();

    /// @brief 读取并清空 eventfd 计数器
    void drainWakeFd();

    /// @brief 在主线程分批执行已完成任务的 onMain() 回调
    /// @return 本次已处理的回调数量
    int dispatchMainTasks();

    /// @brief 将任务标记为完成并从任务表移除
    /// @note 调用时必须持有 mMutex。
    void finishTaskLocked(const TaskPtr& task);

    /// @brief 从 worker/主线程等待队列中移除任务
    /// @note 调用时必须持有 mMutex。
    void removeQueuedTaskLocked(const TaskPtr& task);

    /// @brief 生成当前未使用的正整数任务ID
    /// @note 调用时必须持有 mMutex。
    int nextTaskIdLocked();

private:
    bool     mIsInit{ false };     // 线程池是否已启动
    bool     mStopping{ false };   // 是否正在关闭
    bool     mTerminated{ false }; // 是否已永久关闭
    int      mCount{ 0 };          // worker 数量
    uint64_t mTaskId{ 0 };         // 任务ID计数器

    std::mutex              mLifecycleMutex; // 串行化 init()/shutdown()
    std::mutex              mMutex;          // 保护运行状态、任务表及队列
    std::mutex              mWakeMutex;      // 保护 eventfd 的写入、读取和关闭
    std::condition_variable mWorkerCv;       // 通知 worker 新任务或关闭事件
    std::condition_variable mTaskStateCv;    // 通知 del() 任务状态已变更

    std::vector<std::thread>         mWorkers;                  // 固定 worker 线程集合
    std::deque<TaskPtr>              mWaitTasks;                // 等待 onTask() 的队列
    std::deque<TaskPtr>              mMainTasks;                // 等待 onMain() 的队列
    std::unordered_map<int, TaskPtr> mTasks;                    // 所有未结束任务
    cdroid::Looper*                  mMainLooper{ nullptr };    // 非拥有的主 Looper
    int                              mWakeFd{ -1 };             // 主线程回调唤醒FD
};

#endif // __THREAD_MGR_H__
