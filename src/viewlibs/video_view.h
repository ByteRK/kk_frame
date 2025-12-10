/*
 * @Author: xlc
 * @Email: hookjc15@gmail.com
 * @Date: 2025-12-05 19:33:30
 * @LastEditTime: 2025-12-10 19:24:51
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
#include <widget/imageview.h>
#include <core/windowmanager.h>
#include <shared_queue.h>

class VideoViewPrivate;
class VideoView : public ImageView, public EventHandler {
public:
    // 播放状态
    typedef enum {
        VS_STOPPED = 0,   // 停止中
        VS_STARTING,      // 初始化中
        VS_PLAYING,       // 播放中
        VS_PAUSED,        // 暂停中
        VS_COMPLETED,     // 已结束
    } VideoStatus;

    // 回调设定
    DECLARE_UIEVENT(void, OnPlayStatusChange, View& v, double duration, double progress, int status);

public:
    VideoView(Context* ctx, const AttributeSet& attrs);
    VideoView(int w, int h);
    ~VideoView();

    void   start();                                       // 开始
    bool   play();                                        // 播放
    bool   pause();                                       // 暂停
    void   stop();                                        // 结束

    bool   isPlay();                                      // 是否正在播放
    int    getStatus();                                   // 播放状态
    double getProgress();                                 // 播放进度
    double getDuration();                                 // 视频时长

    void   setUrl(const std::string& url);                // 设置播放地址
    void   setLoop(bool flag);                            // 设置循环播放
    void   setVolume(int volume);                         // 设置音量
    void   setPoints(std::vector<Point>& points);         // 设置打孔范围
    void   setPointsFile(const std::string& fpath);       // 设置打孔范围文件
    void   setOnPlayStatusChange(OnPlayStatusChange l);   // 设置播放状态改变回调

    void   seekTime(int sec);                             // 跳转到指定时间
    void   seekPercent(double percent);                   // 跳转到指定百分比
    void   pushMessage(int msg);                          // 发送消息

protected:
    virtual void onDraw(Canvas& ctx)override;             // 绘制
    virtual int  checkEvents()override;                   // 检查事件
    virtual int  handleEvents()override;                  // 处理事件
    virtual void onDetachedFromWindow()override;          // 处理窗口移除
    void         handlePlayerNotify(int msg);             // 处理播放器通知

protected:
    void initViewData();                          // 初始化参数
    void initVideo();                             // 初始化视频
    void onTick();                                // 定时器回调
    void onTask();                                // 任务回调

private:
    std::shared_ptr<VideoViewPrivate> mPri;       // 私有数据指针(兼容多平台)
    static void* threadReadFrame(void* param);    // 读取帧线程(x64用)

protected:
    VideoStatus        mStatus;                   // 播放状态
    std::string        mUrl;                      // 播放地址
    bool               mLoopPlayBack;             // 循环播放
    int                mVolume;                   // 音量
    int                mMsg;                      // 消息
    double             mDuration;                 // 视频时长
    Runnable           mRunner;                   // 定时器
    OnPlayStatusChange mChangeCallback;           // 播放状态改变回调
    std::vector<Point> mPoints;                   // 打孔范围坐标点(制空则打矩形孔)
    shared_queue<int>  mMsgQueue;                 // 消息队列

};

#endif // __VIDEO_VIEW_H__
