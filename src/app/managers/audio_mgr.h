/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-06
 * @FilePath: /kk_frame/src/app/managers/audio_mgr.h
 * @Description: ALSA 音频播放管理器
 */

#ifndef __AUDIO_MGR_H__
#define __AUDIO_MGR_H__

#include "template/singleton.h"
#include <condition_variable>
#include <cstdint>
#include <mutex>
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

    ~AudioMgr();

    /// @brief 异步播放 PCM WAV 文件，会停止当前音频。
    bool play(const std::string& filePath, bool loop = false);
    bool pause();
    bool resume();
    void stop();

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
    void playbackLoop();
    void setErrorLocked(const std::string& error);

private:
    mutable std::mutex      mMutex;
    std::condition_variable mCondition;
    std::thread             mThread;
    bool                    mExit{ false };
    uint64_t                mRequestId{ 0 };
    State                   mState{ State::Stopped };
    std::string             mFilePath;
    std::string             mLastError;
    bool                    mLoop{ false };
    float                   mVolume{ 1.0f };
    int64_t                 mPositionMs{ 0 };
    int64_t                 mDurationMs{ 0 };
    int64_t                 mSeekMs{ -1 };
};

#endif // __AUDIO_MGR_H__
