/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-06 22:53:00
 * @LastEditTime: 2026-07-06 23:11:08
 * @FilePath: /kk_frame/src/app/managers/audio_mgr.h
 * @Description: ALSA 音频播放管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __AUDIO_MGR_H__
#define __AUDIO_MGR_H__

#include "template/singleton.h"
#include <core/looper.h>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>

#define g_audio AudioMgr::instance()

/// @brief ALSA 音频播放管理器（单例）。
///
/// 设计要点：
/// - 生产者-消费者模型：主线程通过 play()/writeStream() 投递任务，
///   独立播放线程 playbackLoop() 负责 ALSA 设备的打开/写入/关闭。
/// - 状态变更通过 eventfd + Looper 异步回调到主线程，避免跨线程回调死锁。
/// - 所有对 mMutex 保护的成员访问必须加锁，getter 使用 mutable mutex。
/// - 通过 mRequestId 实现请求取消：新请求递增 mRequestId，播放线程检测不匹配则丢弃旧请求。
class AudioMgr : public Singleton<AudioMgr> {
    friend class Singleton<AudioMgr>;

public:
    /// @brief PCM 音频格式描述。
    struct PcmFormat {
        uint32_t sampleRate{ 44100 };   ///< 采样率 (Hz)
        uint16_t channels{ 2 };         ///< 声道数
        uint16_t bitsPerSample{ 16 };   ///< 位深度 (8/16/24/32)
    };

    /// @brief 播放状态机。
    enum class State {
        Stopped,    ///< 未播放 / 已停止
        Loading,    ///< 正在加载（等待播放线程接管）
        Playing,    ///< 正在播放
        Paused,     ///< 已暂停
        Completed,  ///< 播放完成（自然结束或流结束）
        Error,      ///< 发生错误
    };

    /// @brief 状态变更监听器。回调在 Looper 线程执行，注意线程安全。
    class AudioListener {
    public:
        virtual ~AudioListener() { }
        /// @brief 播放状态变化时回调（在注册的 Looper 线程执行）。
        virtual void onAudioStateChanged(State oldState, State newState) = 0;
    };

    ~AudioMgr();

    // ---- 播放控制 ----

    /// @brief 异步播放 PCM WAV 文件，会停止当前音频。
    /// @param filePath WAV 文件路径。
    /// @param loop 是否循环播放。
    bool play(const std::string& filePath, bool loop = false);

    /// @brief 开始播放原始 PCM 数据流，会停止当前音频。
    /// @param format PCM 格式参数。
    /// @param maxBufferedBytes 待播放数据的最大缓冲字节数，超限后 writeStream() 会拒绝写入。
    bool startStream(const PcmFormat& format, size_t maxBufferedBytes = 1024 * 1024);
    /// @brief 复制 PCM 数据到播放队列。仅流模式有效。
    /// @return 成功返回写入字节数；队列满或参数无效返回 0。
    size_t writeStream(const void* data, size_t size);
    /// @brief 标记数据流结束，队列中已有数据播放完后进入 Completed 状态。
    bool finishStream();

    /// @brief 暂停播放（仅 Playing 状态下有效）。
    bool pause();
    /// @brief 恢复播放（仅 Paused 状态下有效）。
    bool resume();
    /// @brief 停止播放，重置所有状态。
    void stop();

    // ---- 监听器（线程安全） ----

    /// @note 回调由 eventfd 投递到主 Looper 线程，监听器析构前必须注销。
    void addListener(AudioListener* listener);
    void removeListener(AudioListener* listener);

    // ---- 播放属性（线程安全） ----

    /// @brief 跳转到指定毫秒位置。仅文件模式、Playing/Paused 状态下有效。
    bool seek(int64_t positionMs);
    void setLoop(bool loop);
    bool isLoop() const;

    /// @brief 软件音量，范围 0.0 ~ 1.0，在播放线程中逐帧应用。
    void setVolume(float volume);
    float getVolume() const;

    // ---- 状态查询（线程安全） ----

    State getState() const;
    bool isPlaying() const;
    /// @brief 当前播放位置（毫秒）。
    int64_t getPosition() const;
    /// @brief 总时长（毫秒）。流模式下完成前为 0，完成后为实际播放时长。
    int64_t getDuration() const;
    std::string getCurrentFile() const;
    std::string getLastError() const;

    /// @brief 检测 ALSA 支持。由 ENABLE_AUDIO 宏控制。
    static bool isSupported();

protected:
    AudioMgr();

private:
    /// @brief 音频来源类型。
    enum class SourceType { File, Stream };

    /// @brief 状态变更事件，用于 eventfd 跨线程投递。
    struct StateEvent {
        State oldState;
        State newState;
    };

    /// @brief 播放线程主循环：等待 Loading 状态 → 打开 ALSA → 写入数据 → 关闭 ALSA。
    void playbackLoop();
    /// @brief 修改状态并返回旧状态。调用方必须持有 mMutex。
    State setStateLocked(State state);
    /// @brief 将状态变更事件推入队列并通过 eventfd 唤醒 Looper 线程。
    void postStateChanged(State oldState, State newState);
    /// @brief 在 Looper 线程中分发所有待处理的状态事件给监听器。
    void dispatchStateEvents();
    /// @brief 排空 eventfd，避免积压唤醒。
    void drainWakeFd();
    /// @brief eventfd 可读回调（静态），由 Looper 调用。
    static int onWake(int fd, int events, void* context);

private:
    // ---- 播放器核心状态（mMutex 保护） ----
    mutable std::mutex      mMutex;             ///< 主互斥锁，保护以下所有成员
    std::condition_variable mCondition;         ///< 播放线程等待/唤醒条件变量
    std::thread             mThread;            ///< 播放线程
    bool                    mExit{ false };     ///< 退出标志，析构时置 true
    uint64_t                mRequestId{ 0 };    ///< 请求 ID，递增以取消旧请求
    State                   mState{ State::Stopped };  ///< 当前播放状态
    std::string             mFilePath;          ///< 当前播放文件路径
    std::string             mLastError;         ///< 最后一次错误信息
    std::set<AudioListener*> mListeners;        ///< 监听器集合（mListenerMutex 保护）
    bool                    mLoop{ false };     ///< 是否循环播放
    float                   mVolume{ 1.0f };    ///< 软件音量 (0.0 ~ 1.0)
    int64_t                 mPositionMs{ 0 };   ///< 当前播放位置 (ms)
    int64_t                 mDurationMs{ 0 };   ///< 总时长 (ms)
    int64_t                 mSeekMs{ -1 };      ///< 跳转目标位置 (ms)，-1 表示无跳转请求
    SourceType              mSourceType{ SourceType::File };  ///< 音频来源类型
    PcmFormat               mStreamFormat;      ///< 流模式下的 PCM 格式
    std::deque<std::vector<char>> mStreamBuffers;  ///< 流模式数据缓冲队列
    size_t                  mStreamBufferedBytes{ 0 };       ///< 当前缓冲字节数
    size_t                  mStreamMaxBufferedBytes{ 1024 * 1024 }; ///< 最大缓冲字节数
    bool                    mStreamFinished{ false };        ///< 流数据是否已全部写入

    // ---- 事件分发（独立锁，避免与 mMutex 死锁） ----
    std::recursive_mutex    mListenerMutex;     ///< 监听器列表锁（递归锁，允许回调中注销）
    std::mutex              mEventMutex;        ///< 事件队列锁
    std::queue<StateEvent>  mStateEvents;       ///< 待分发状态事件队列
    cdroid::Looper*         mCallbackLooper{ nullptr };  ///< 主线程 Looper，用于 eventfd 回调
    int                     mWakeFd{ -1 };      ///< eventfd 文件描述符，用于跨线程唤醒
};

#endif // __AUDIO_MGR_H__
