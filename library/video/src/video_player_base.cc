/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 15:23:19
 * @FilePath: /kk_frame/library/video/src/video_player_base.cc
 * @Description: 播放器公共基类实现
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_player_base.h"

#include <cdlog.h>

namespace video {

    VideoPlayerBase::VideoPlayerBase() { }

    VideoPlayerBase::~VideoPlayerBase() {
        clearEvents();
    }

    /// @brief 设置是否循环播放
    /// @param loop 循环开关
    /// @return true 成功
    bool VideoPlayerBase::setLoop(bool loop) {
        std::lock_guard<std::mutex> lock(mMutex);
        mLoopFlag = loop;
        return true;
    }

    /// @brief 设置事件监听
    /// @param listener 
    void VideoPlayerBase::setListener(Listener* listener) {
        std::lock_guard<std::mutex> lock(mMutex);
        mListener = listener;
    }

    /// @brief 是否存在事件
    /// @return 
    bool VideoPlayerBase::hasEvents() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLoopPending || !mEvents.empty();
    }

    /// @brief 派发事件
    void VideoPlayerBase::dispatchEvents() {
        Listener* listener = nullptr;
        bool      loopRestart = false;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mEvents.empty() && !mLoopPending) return;
            listener = mListener;
            loopRestart = mLoopPending;
            mLoopPending = false;
        }

        // 注意：必须在释放锁之后再回调/重启，避免与后端自身加锁形成死锁
        if (loopRestart) {
            const bool ok = restart();
            LOGI("video loop restart: %s", ok ? "ok" : "failed");
            // 重播成功则不上报结束，直接回到播放中
            postStatus(ok ? VS_PLAY : VS_OVER, getDuration(), 0);
        }

        if (listener == nullptr) {
            // 没有监听也要把事件取走，避免队列无限增长
            std::lock_guard<std::mutex> lock(mMutex);
            mEvents.clear();
            return;
        }

        std::deque<Event> events;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            events.swap(mEvents);
        }
        for (const Event& event : events) {
            if (event.type == EVENT_FRAME) {
                listener->onVideoFrame();
            } else {
                listener->onVideoStatus(event.status, event.duration, event.position);
            }
        }
    }

    /// @brief 是否循环播放
    /// @return true 循环，false 不循环
    bool VideoPlayerBase::isLoop() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLoopFlag;
    }

    /// @brief 事件入队：状态变化
    /// @param status 状态
    /// @param duration 视频总时长，单位毫秒
    /// @param position 当前播放进度，单位毫秒
    void VideoPlayerBase::postStatus(int status, double duration, double position) {
        std::lock_guard<std::mutex> lock(mMutex);
        mEvents.push_back(Event{ EVENT_STATUS, status, duration, position });
    }

    /// @brief 事件入队：新帧就绪（仅 VM_FRAME）
    void VideoPlayerBase::postFrame() {
        std::lock_guard<std::mutex> lock(mMutex);
        mEvents.push_back(Event{ EVENT_FRAME, 0, 0, 0 });
    }

    /// @brief 丢弃所有待派发事件
    void VideoPlayerBase::clearEvents() {
        std::lock_guard<std::mutex> lock(mMutex);
        mEvents.clear();
        mLoopPending = false;
    }

    /// @brief 播放结束处理
    /// @param duration 视频总时长，单位毫秒
    /// @param position 当前播放进度，单位毫秒
    void VideoPlayerBase::handlePlayerEnd(double duration, double position) {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mLoopFlag) {
                // 循环播放：不上报结束，登记待重启，由 dispatchEvents 在 UI 线程执行
                mLoopPending = true;
                return;
            }
        }
        postStatus(VS_OVER, duration, position);
    }

} // namespace video
