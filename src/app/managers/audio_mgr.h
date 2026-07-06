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
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

#define g_audio AudioMgr::instance()

class AudioMgr : public Singleton<AudioMgr> {
    friend class Singleton<AudioMgr>;

public:
    enum class State {
        Stopped,
        Loading,
        Playing,
        Paused,
        Completed,
        Error,
    };

    class AudioListener {
    public:
        virtual ~AudioListener() { }
        virtual void onAudioStateChanged(State oldState, State newState) = 0;
    };

    ~AudioMgr();

    /// @brief 异步播放 PCM WAV 文件，会停止当前音频。
    bool play(const std::string& filePath, bool loop = false);
    bool pause();
    bool resume();
    void stop();

    /// @note 回调由 eventfd 投递到主 Looper 线程，监听器析构前必须注销。
    void addListener(AudioListener* listener);
    void removeListener(AudioListener* listener);

    /// @brief 跳转到指定毫秒位置。
    bool seek(int64_t positionMs);
    void setLoop(bool loop);
    bool isLoop() const;

    /// @brief 软件音量，范围 0.0 ~ 1.0。
    void setVolume(float volume);
    float getVolume() const;

    State getState() const;
    bool isPlaying() const;
    int64_t getPosition() const;
    int64_t getDuration() const;
    std::string getCurrentFile() const;
    std::string getLastError() const;

    static bool isSupported();

protected:
    AudioMgr();

private:
    struct StateEvent {
        State oldState;
        State newState;
    };

    void playbackLoop();
    State setStateLocked(State state);
    void postStateChanged(State oldState, State newState);
    void dispatchStateEvents();
    void drainWakeFd();
    static int onWake(int fd, int events, void* context);

private:
    mutable std::mutex      mMutex;
    std::condition_variable mCondition;
    std::thread             mThread;
    bool                    mExit{ false };
    uint64_t                mRequestId{ 0 };
    State                   mState{ State::Stopped };
    std::string             mFilePath;
    std::string             mLastError;
    std::set<AudioListener*> mListeners;
    bool                    mLoop{ false };
    float                   mVolume{ 1.0f };
    int64_t                 mPositionMs{ 0 };
    int64_t                 mDurationMs{ 0 };
    int64_t                 mSeekMs{ -1 };

    std::recursive_mutex    mListenerMutex;
    std::mutex              mEventMutex;
    std::queue<StateEvent>  mStateEvents;
    cdroid::Looper*         mCallbackLooper{ nullptr };
    int                     mWakeFd{ -1 };
};

#endif // __AUDIO_MGR_H__
