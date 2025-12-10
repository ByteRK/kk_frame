/*
 * @Author: xlc
 * @Email: 
 * @Date: 2025-12-05 19:33:30
 * @LastEditTime: 2025-12-08 17:12:01
 * @FilePath: /kk_frame/src/viewlibs/video_view.h
 * @Description: 视频播放组件 video_view
 * 
 *  <VideoView
 *    android:layout_width="800dp"
 *    android:layout_height="480dp"
 *    android:layout_marginTop="120dp"
 *    android:layout_marginLeft="320dp"
 *    android:url="test.mp4"
 *    android:loadPlay="true"/>
 * 
 * Copyright (c) 2025 by xlc, All Rights Reserved. 
 * 
**/

#ifndef __VIDEO_VIEW_H__
#define __VIDEO_VIEW_H__

#include <common.h>
#include <comm_func.h>
#include <widget/imageview.h>
#include <core/windowmanager.h>

class VideoInfo;
class VideoView : public ImageView, public EventHandler {
public:
    // 播放状态
    typedef enum {
        VS_NULL = 0, // 无状态
        VS_INIT,     // 初始化完成
        VS_PLAY,     // 播放中
        VS_PAUSE,    // 暂停中
        VS_OVER,     // 结束
    } VideoStatus;

    DECLARE_UIEVENT(void, OnPlayStatusChange, View& v, double duration, double progress, int status);

public:
    VideoView(Context* ctx, const AttributeSet& attrs);
    VideoView(int w, int h);
    ~VideoView();

    bool   play();                                     // 开始
    bool   isPlay();                                   // 已经开始
    bool   pause();                                    // 暂停
    bool   resume();                                   // 继续
    void   over();                                     // 结束
    double getDuration();                              // 视频时长
    double getProgress();                              // 播放进度
    void   setProgress(double dp);                     // 设置播放进度
    int    getStatus();                                // 播放状态
    void   setURL(const std::string& url);             // 设置播放地址
    void   setPointsFile(const std::string& fpath);    // 设置打孔范围文件
    void   setPoints(std::vector<Point>& points);      // 设置打孔范围

    void   setOnPlayStatusChange(OnPlayStatusChange l) { mChangeCallback = l; } // 设置播放状态改变回调

protected:
    virtual void onLayout(bool change, int l, int t, int w, int h)override;     // 布局完成
    virtual void onDraw(Canvas& ctx)override;                                   // 绘制
    virtual int  checkEvents()override;                                         // 检查事件
    virtual int  handleEvents()override;                                        // 处理事件

protected:
    void initViewData();    // 初始化数据
    void startPlay();       // 开始播放
    void onTick();          // 定时器回调
    void initVideo();       // 初始化视频

protected:
    VideoInfo*         mVideoInfo;          // 视频信息
    HANDLE             mHandle;             // 视频句柄
    bool               mLoadPlay;           // 加载后自动播放
    Runnable           mRunner;             // 定时器
    int                mRunnerTime;         // 定时器间隔
    bool               mDelayPlay;          // 延迟播放

    std::string        mURL;                // 播放地址
    std::vector<Point> mPoints;             // 打孔范围坐标点（制空则打矩形孔）

    int                mError;              // 错误码
    int                mStatus;             // 播放状态
    int                mLastCode;           // 上次播放状态
    double             mProgress;           // 播放进度
    double             mDuration;           // 视频时长
    OnPlayStatusChange mChangeCallback;     // 播放状态改变回调
    bool               mOneShot;

};

#endif // __VIDEO_VIEW_H__
