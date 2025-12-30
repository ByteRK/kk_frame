/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-24 10:07:01
 * @LastEditTime: 2025-12-29 13:52:17
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
#include <core/windowmanager.h>

class VideoInfo;
class VideoView : public ImageView, public EventHandler {
public:
    typedef enum {
        VS_NULL = 0,
        VS_INIT,  // 初始化完成
        VS_PLAY,  // 播放中
        VS_PAUSE, // 暂停中
        VS_OVER,  // 结束
    } VideoStatus;

    DECLARE_UIEVENT(void, OnPlayStatusChange, View &v, double duration, double progress, int status);

public:
    VideoView(Context *ctx, const AttributeSet &attrs);
    VideoView(int w, int h);
    ~VideoView();
    
    bool   play();                 // 开始
    bool   isPlay();               // 已经开始
    bool   pause();                // 暂停
    bool   resume();               // 继续
    void   over();                 // 结束
    double getDuration();          // 视频时长
    double getProgress();          // 播放进度
    void   setProgress(double dp); // 设置播放进度
    int    getStatus();            // 播放状态
    void   setURL(const std::string &url);
    void   setPointsFile(const std::string &fpath);
    void   setPoints(std::vector<Point> &points);

    void   setOnPlayStatusChange(OnPlayStatusChange l){ mChangeCallback = l;}

protected:
    virtual void onLayout(bool change,int l,int t,int w,int h);
    virtual void onDraw(Canvas& ctx);
    virtual int  checkEvents();
    virtual int  handleEvents();

protected:
    void initViewData();
    void onTick();
    void initVideo();

protected:
    int                mXMLWidth = -1;
    int                mXMLHeight = -1;

    int                mError;
    HANDLE             mHandle;
    Runnable           mTicker;
    std::string        mURL;
    std::vector<Point> mPoints;
    int                mMaxTime;
    int                mCurTime;
    int                mStatus;
    int                mLastCode;
    bool               mLoadPlay; /* 加载后播放 */
    double             mProgress;
    double             mDuration;
    VideoInfo         *mVideoInfo;
    OnPlayStatusChange mChangeCallback;
    bool               mOneShot;
    int                mCheckTime;
};

#endif // !__VIDEO_VIEW_H__
