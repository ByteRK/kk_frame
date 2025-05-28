/*
 * 视频播放组件 media_player
 */

 #ifndef __mp_video_h__
 #define __mp_video_h__
 
 #include <common.h>
 #include <comm_func.h>
 // #include <comm_view.h>
 
 
 // #include "libs.h"
 #include <widget/imageview.h>
 
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
 
     DECLARE_UIEVENT(void, OnPlayStatusChange, View &v, int duration, int progress, int status);
 
 public:
     VideoView(Context *ctx, const AttributeSet &attrs);
     VideoView(int w, int h);
     ~VideoView();
 
     bool   play();        // 开始
     bool   isPlay();      // 已经开始
     bool   pause();       // 暂停
     bool   resume();      // 继续
     void   over();        // 结束
     double getDuration(); // 视频时长
     double getProgress(); // 播放进度
     int    getStatus();   // 状态
     void   setURL(const std::string &url);
     void   setPointsFile(const std::string &fpath);
     void   setPoints(std::vector<Point> &points);
     void   setOnPlayStatusChange(OnPlayStatusChange l);
 
 protected:
     virtual void onDraw(Canvas &ctx);
     virtual int  checkEvents();
     virtual int  handleEvents();
 
 protected:
     void initViewData();
     void onTick();
     void initVideo();
     int  getTimeFrameNum();
     void onTask();
     static void *threadReadFrame(void *param);
 
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
     VideoInfo *        mVideoInfo;
     OnPlayStatusChange mOnPlayStatusChange;
     bool               mOneShot;
     int                mCheckTime;
     int64_t            mPlayTime;
     int                mPlayFrameNum;
     bool               mTaskRun;
     bool               mTaskExit;
 };
 
 #endif
 
 