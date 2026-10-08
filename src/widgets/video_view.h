/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-24 10:07:01
 * @LastEditTime: 2026-10-08 14:41:07
 * @FilePath: /kk_frame/src/widgets/video_view.h
 * @Description: 视频播放组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __VIDEO_VIEW_H__
#define __VIDEO_VIEW_H__

#include <widget/imageview.h>
#include <core/looper.h>
#include <core/windowmanager.h>

#include "video_player.h"

/// @brief 视频播放组件
/// @note 应用层交互 + 几何/旋转换算 + 挖洞或绘帧，由 library/video 进行播放实现。
/// @note 后端有两种输出模式：
///       - VM_OVERLAY：画面由硬件图层直接输出，组件负责挖洞；
///       - VM_FRAME  ：后端输出 RGB 帧，组件负责拷贝并绘制；
class VideoView : public ImageView, public video::VideoPlayer::Listener {
public:
    /// @brief 播放状态
    /// @note 与 video::VideoStatus 保持一致
    typedef enum {
        VS_NULL  = video::VS_NULL,
        VS_INIT  = video::VS_INIT,
        VS_PLAY  = video::VS_PLAY,
        VS_PAUSE = video::VS_PAUSE,
        VS_OVER  = video::VS_OVER,
        VS_ERROR = video::VS_ERROR,
    } VideoStatus;

    DECLARE_UIEVENT(void, OnPlayStatusChange, View& v, double duration, double progress, int status);

public:
    VideoView(cdroid::Context* ctx, const AttributeSet& attrs);
    VideoView(int w, int h);
    ~VideoView() override;

private:
    void initViewData();

public:
    static bool        isSupported();
    static const char* getBackend();

public:
    bool play();
    bool pause();
    bool resume();
    void over();
    bool isPlay() const;

    int    getStatus() const;
    double getDuration() const;
    double getProgress() const;
    void   setProgress(double ms);

    void   setURL(const std::string& url);
    void   setLoop(bool loop);
    void   setVolume(int volume);
    void   setPoints(const std::vector<Point>& points);
    void   setPointsFile(const std::string& fpath);
    void   setCoverImage(const std::string& path);

    void   setUnsupportedText(const std::string& text);
    void   setOnPlayStatusChange(OnPlayStatusChange l);

protected:
    void onLayout(bool changed, int l, int t, int w, int h) override;
    void onDraw(Canvas& canvas) override;

    void onVideoStatus(int status, double duration, double position) override;
    void onVideoFrame() override;

private:
    bool ensurePlayer();
    bool isLayoutReady() const;
    bool openMedia();
    bool startPlay();
    bool applyFrame(const video::VideoFrame& frame);
    void syncGeometry();
    void startPolling();
    void stopPolling();
    void onTick();
    void clipRegion(Canvas& canvas);
    void drawUnsupported(Canvas& canvas);
    void drawFrame();
    void notifyStatus(int status);

private:
    static constexpr int POLL_INTERVAL_PLAY_MS = 5;   // 播放中轮询间隔
    static constexpr int POLL_INTERVAL_IDLE_MS = 50;  // 非播放中轮询间隔

private:
    video::VideoPlayer*                mPlayer{ nullptr };
    Runnable                           mTicker;
    std::string                        mURL;
    std::string                        mUnsupportedText{ "视频未支持" };
    std::vector<Point>                 mPoints;
    bool                               mLoadPlay{ false };
    bool                               mLoop{ false };
    bool                               mPendingPlay{ false };
    bool                               mCoverShowing{ false };
    int                                mVolume{ 70 };
    int                                mProgressInterval{ 0 };
    int                                mUnsupportedTextSize{ 30 };
    int                                mStatus{ VS_NULL };
    double                             mDuration{ 0 };
    double                             mProgress{ 0 };
    video::VideoGeometry               mGeometry{ 0, 0, 0, 0, 0, 0, 0 };
    Cairo::RefPtr<Cairo::ImageSurface> mFrameSurface;
    int                                mFrameWidth{ 0 };
    int                                mFrameHeight{ 0 };
    int64_t                            mLastNotifyMs{ 0 };
    bool                               mPolling{ false };
    OnPlayStatusChange                 mChangeCallback;
};

#endif // !__VIDEO_VIEW_H__
