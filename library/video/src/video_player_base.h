/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 15:21:31
 * @FilePath: /kk_frame/library/video/src/video_player_base.h
 * @Description: 播放器公共基类（收敛各后端重复的事件队列与循环播放逻辑）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __VIDEO_PLAYER_BASE_H__
#define __VIDEO_PLAYER_BASE_H__

#include <deque>
#include <mutex>

#include "video_player.h"

namespace video {

    /// @brief 播放器公共基类
    /// @note 底层回调基本来自播放线程，通过事件队列转发到 UI 线程，避免线程冲突
    class VideoPlayerBase : public VideoPlayer {
    private:
        /// @brief 事件类型
        enum EventType {
            EVENT_STATUS = 0, // 状态变化
            EVENT_FRAME       // 新帧就绪（仅 VM_FRAME）
        };

        /// @brief 事件结构体
        struct Event {
            int    type;      // 事件类型
            int    status;    // 状态码
            double duration;  // 视频总时长，单位毫秒
            double position;  // 当前播放进度，单位毫秒
        };

        mutable std::mutex mMutex;                // 保护事件队列与监听器（const 查询也要用）
        std::deque<Event>  mEvents;               // 待派发事件
        Listener*          mListener{ nullptr };  // 监听器（不持有其生命周期）
        bool               mLoopFlag{ false };    // 循环开关
        bool               mLoopPending{ false }; // 已结束但等待在 UI 线程重启

    public:
        VideoPlayerBase();
        ~VideoPlayerBase() override;

    public:
        bool setLoop(bool loop) override;
        void setListener(Listener* listener) override;
        bool hasEvents() override;
        void dispatchEvents() override;

    protected:
        bool isLoop() const;
        void postStatus(int status, double duration, double position);
        void postFrame();
        void clearEvents();
        void handlePlayerEnd(double duration, double position);

        /// @brief 从头重新播放（由各后端实现）
        /// @return 是否已成功发起重播
        virtual bool restart() = 0;
    };

} // namespace video

#endif // !__VIDEO_PLAYER_BASE_H__
