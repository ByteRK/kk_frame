/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-05 19:33:30
 * @LastEditTime: 2025-12-10 19:24:24
 * @FilePath: /kk_frame/src/viewlibs/video_view_rk.cc
 * @Description: 视频播放组件 RK芯片版本
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#if defined(CDROID_RK3506) || defined(__VSCODE__)

#include "video_view.h"
#include <cdplayer.h>
#include <unistd.h>

enum {
    RKADK_ROTATE_NONE,
    RKADK_ROTATE_90,
    RKADK_ROTATE_180,
    RKADK_ROTATE_270
};

enum {
    RKADK_PLAYER_STATE_IDLE = 0,    /**< The player state before init . */
    RKADK_PLAYER_STATE_INIT,        /**< The player is in the initial state. It changes
                                    to the initial state after being SetDataSource. */
    RKADK_PLAYER_STATE_PREPARED,    /**< The player is in the prepared state. */
    RKADK_PLAYER_STATE_PLAY,        /**< The player is in the playing state. */
    RKADK_PLAYER_STATE_EOF,         /**< The player is in the playered end state. */
    RKADK_PLAYER_STATE_PAUSE,       /**< The player is in the pause state. */
    RKADK_PLAYER_STATE_STOP,        /**< The player is in the stop state. */
    RKADK_PLAYER_STATE_ERR,         /**< The player is in the err state(reserved). */
    RKADK_PLAYER_STATE_BUTT
};

#define VVP(value) VideoViewPrivate *value = mPri.get();

//////////////////////////////////////////////////////////////////

// 视频信息
class VideoViewPrivate {
public:
    HANDLE     mHandle;
    int        mStatus;
    double     mProgress;
    VideoViewPrivate() {
        mHandle = nullptr;
        mStatus = -1;
    }
    ~VideoViewPrivate() {
    }
    void close() {
        if (mHandle) MPClose(mHandle);
        mHandle = nullptr;
        mStatus = -1;
        mProgress = 0;
    }
};

//////////////////////////////////////////////////////////////////
DECLARE_WIDGET(VideoView)

/// @brief 初始化视频
/// @param w 
/// @param h 
VideoView::VideoView(int w, int h) : ImageView(w, h) {
    initViewData();
}

/// @brief XML初始化
/// @param ctx 
/// @param attrs 
VideoView::VideoView(Context* ctx, const AttributeSet& attrs) : ImageView(ctx, attrs) {
    initViewData();
    setUrl(attrs.getString("url"));
    setLoop(!attrs.getBoolean("oneShot", true));
    setPointsFile(attrs.getString("pointsFile"));
    if (attrs.getBoolean("loadPlay")) postDelayed([this]() { start(); });
}

/// @brief 析构数据
VideoView::~VideoView() {
    VVP(pri);
    pri->close();
    mStatus = VS_STOPPED;
}

/// @brief 开始
void VideoView::start() {
    stop();
    LOGI("start cur-status: %d", mStatus);
    mStatus = VS_STARTING;
    removeCallbacks(mRunner);
    postDelayed(mRunner, 1000);
    initVideo();
    invalidate();
}

/// @brief 播放
/// @return 
bool VideoView::play() {
    if (mStatus == VS_PLAYING) return false;
    LOG(INFO) << "play cur-status:" << mStatus;
    mStatus = VS_PLAYING;
    VVP(pri);
    MPPlay(pri->mHandle);
    invalidate(true);
    removeCallbacks(mRunner);
    View::postDelayed(mRunner, 1);
    return true;
}

/// @brief 暂停
/// @return 
bool VideoView::pause() {
    if (mStatus != VS_PLAYING) return false;
    LOG(INFO) << "pause cur-status:" << mStatus;
    mStatus = VS_PAUSED;
    VVP(pri);
    MPPause(pri->mHandle);
    return true;
}

/// @brief 结束
void VideoView::stop() {
    if (mStatus == VS_STOPPED) return;
    LOG(INFO) << "stop cur-status:" << mStatus;
    mStatus = VS_STOPPED;
    removeCallbacks(mRunner);
    VVP(pri);
    pri->close();
    invalidate();
}

/// @brief 是否正在播放
bool VideoView::isPlay() {
    return mStatus > VS_STARTING && mStatus < VS_COMPLETED;
}

/// @brief 获取状态
/// @return 
int VideoView::getStatus() {
    return mStatus;
}

/// @brief 获取当前播放进度
/// @return 
double VideoView::getProgress() {
    VVP(pri);
    return pri->mProgress;
}

/// @brief 获取视频时长
/// @return 
double VideoView::getDuration() {
    return mDuration;
}

/// @brief 设置视频地址
/// @param url 
void VideoView::setUrl(const std::string& url) {
    mUrl = url;
}

/// @brief 设置是否循环播放
/// @param flag 
void VideoView::setLoop(bool flag) {
    mLoopPlayBack = flag;
}

/// @brief 设置音量
/// @param volume 
void VideoView::setVolume(int volume) {
    if (volume >= 0 && volume <= 100) { mVolume = volume; }
}

/// @brief 设置打孔范围
/// @param points 
void VideoView::setPoints(std::vector<Point>& points) {
    mPoints = points;
}

/// @brief 设置打孔范围文件
/// @param fpath 
void VideoView::setPointsFile(const std::string& fpath) {
    if (fpath.empty()) return;
    if (access(fpath.c_str(), F_OK)) {
        LOGE("point file not exists. fpath=%s", fpath.c_str());
        return;
    }
    char buffer[4096];
    FILE* fp = fopen(fpath.c_str(), "r");
    int rlen = fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);
    buffer[rlen - 1] = '\0';
    if (rlen == sizeof(buffer) - 1) LOGW("buffer maybe not enough!!!");
    Point pt;
    for (char* p = buffer; *p; p++) {
        if (*p == '{') {
            pt.x = atoi(p + 1);
        } else if (*p == ',' && *(p + 1) >= '0' && *(p + 1) <= '9') {
            pt.y = atoi(p + 1);
            mPoints.push_back(pt);
        }
    }
    LOGI("Read point over. count=%d", mPoints.size());
}

/// @brief 设置播放状态回调
/// @param l 
void VideoView::setOnPlayStatusChange(OnPlayStatusChange l) {
    mChangeCallback = l;
}

/// @brief 设置播放进度
/// @param sec 
void VideoView::seekTime(int sec) {
    VVP(pri);
    if (pri->mHandle && mDuration > 0 && sec >= 0) {
        MPSeek(pri->mHandle, sec * 1000);
    }
}

/// @brief 设置播放进度百分比
/// @param percent 
void VideoView::seekPercent(double percent) {
    VVP(pri);
    if (pri->mHandle && mDuration > 0 && percent > 0) {
        MPSeek(pri->mHandle, mDuration * percent / 100);
    }
}

/// @brief 发送消息(非视频线程使用)
/// @param msg 
void VideoView::pushMessage(int msg) {
    mMsgQueue.push(msg);
}

/// @brief UI图层绘制
/// @param canvas 
void VideoView::onDraw(Canvas& canvas) {
    if (mStatus == VS_STOPPED) {
        ImageView::onDraw(canvas);
        return;
    }

    if (mPoints.empty()) {
        const int width = getWidth();
        const int height = getHeight();
        if (mRadii[0] || mRadii[1] || mRadii[2] || mRadii[3]) {
            const double degrees = M_PI / 180.f;
            canvas.begin_new_sub_path();
            canvas.arc(width - mRadii[1], mRadii[1], mRadii[1], -90 * degrees, 0 * degrees);
            canvas.arc(width - mRadii[2], height - mRadii[2], mRadii[2], 0 * degrees, 90 * degrees);
            canvas.arc(mRadii[3], height - mRadii[3], mRadii[3], 90 * degrees, 180 * degrees);
            canvas.arc(mRadii[0], mRadii[0], mRadii[0], 180 * degrees, 270 * degrees);
            canvas.close_path();
            canvas.clip();
        }
    } else {
        canvas.begin_new_sub_path();
        if (mPoints.size() > 0) {
            canvas.move_to(mPoints[0].x, mPoints[0].y);
            for (size_t i = 1; i < mPoints.size(); i++) { canvas.line_to(mPoints[i].x, mPoints[i].y); }
            canvas.close_path();
            canvas.clip();
        }
    }

    canvas.set_operator(Cairo::Context::Operator::CLEAR);
    canvas.paint();
}

/// @brief 检查事件
/// @return 
int VideoView::checkEvents() {
    if (mMsgQueue.empty()) return 0;
    return 1;
}

/// @brief 处理事件
/// @return 
int VideoView::handleEvents() {
    int msg;
    if (!mMsgQueue.try_and_pop(msg)) return 0;
    handlePlayerNotify(msg);
    return 1;
}

/// @brief 处理窗口移除
void VideoView::onDetachedFromWindow() {
    stop();
    ImageView::onDetachedFromWindow();
}

/// @brief 消息处理
/// @param msg 
void VideoView::handlePlayerNotify(int msg) {
    if (msg == mMsg) return;
    mMsg = msg;
    LOGI("player event msg. msg=%d", msg);
}

/// @brief 初始化页面数据
void VideoView::initViewData() {
    mStatus = VS_STOPPED;
    mUrl = "";
    mLoopPlayBack = false;
    mVolume = 100;
    mMsg = -1;
    mDuration = 0;
    mRunner = std::bind(&VideoView::onTick, this);
    mChangeCallback = nullptr;
    mPri = std::make_shared<VideoViewPrivate>();
    Looper::getMainLooper()->addEventHandler(this);
}

/// @brief 初始化视频
void VideoView::initVideo() {
    if (mStatus != VS_STARTING) {
        LOGW("status not VS_STARTING. status=%d", mStatus);
        return;
    }

    /* 获取窗口位置 */
    int xy[2] = { 0 };
    getLocationInWindow(xy);
    int w(getWidth()), h(getHeight());

    /* 获取旋转角度 */
    int rotate = WindowManager::getInstance().getDisplayRotation();
    if (getenv("DISPLAY_ROTATE")) {
        int hwRotate = atoi(getenv("DISPLAY_ROTATE")) % 360;
        if (hwRotate % 90 == 0)  rotate -= hwRotate / 90;
    }

    /* 获取屏幕大小 */
    Point screenSize;
    WindowManager::getInstance().getDefaultDisplay().getSize(screenSize);

    LOGW("url=%s w=%d h=%d xy[0]=%d xy[1]=%d rotate=%d sw=%d sh=%d"
        , mUrl.c_str(), w, h, xy[0], xy[1], rotate, screenSize.x, screenSize.y);

    /* 获取显示大小 */
    UINT sw, sh;
    GFXGetDisplaySize(0, &sw, &sh);

    /* 开启视频 */
    VVP(pri);
    pri->mHandle = MPOpen(mUrl.c_str());

#define SET_VEDIO_WINDOWS(x,y,w,h) \
        MPSetWindow(pri->mHandle, x, y, w, h); \
        LOGI("MPSetWindow(%d, %d, %d, %d)", x, y, w, h);

    /* 视频旋转 方向与UI相反 */
    switch (rotate) {
    case Display::ROTATION_0: {
        LOGI("MPRotate(%d, RKADK_ROTATE_NONE)", pri->mHandle);
        MPRotate(pri->mHandle, RKADK_ROTATE_NONE);
        SET_VEDIO_WINDOWS(xy[0], xy[1], w, h);
    }   break;
    case Display::ROTATION_90: {
        LOGI("MPRotate(%d, RKADK_ROTATE_270)", pri->mHandle);
        MPRotate(pri->mHandle, RKADK_ROTATE_270);
        SET_VEDIO_WINDOWS(xy[1], sh - w - xy[0], h, w);
    }   break;
    case Display::ROTATION_180: {
        LOGI("MPRotate(%d, RKADK_ROTATE_180)", pri->mHandle);
        MPRotate(pri->mHandle, RKADK_ROTATE_180);
        SET_VEDIO_WINDOWS(xy[0], xy[1], w, h);
    }   break;
    case Display::ROTATION_270: {
        LOGI("MPRotate(%d, RKADK_ROTATE_90)", pri->mHandle);
        MPRotate(pri->mHandle, RKADK_ROTATE_90);
        SET_VEDIO_WINDOWS(sw - h - xy[1], xy[0], h, w);
    }   break;
    default: {
        LOGE("unknow rotate=%d", rotate);
    }   break;
    }

#undef SET_VEDIO_WINDOWS
}

/// @brief 主心跳
void VideoView::onTick() {
    VVP(pri);
    bool change = false;

    // 句柄异常
    if (!pri->mHandle) {
        LOGE("Handle is null, url=%s", mUrl.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_STOPPED);
        return;
    }

    /* 获取状态 */
    int status = MPGetStatus(pri->mHandle);
    if (status != pri->mStatus) {
        VideoView::VideoStatus videoStatus = VS_STOPPED;
        switch (status) {
        case RKADK_PLAYER_STATE_PREPARED: {
            videoStatus = VS_STARTING;
            mDuration = getDuration();
        }   break;
        case RKADK_PLAYER_STATE_PLAY: {
            play();
            videoStatus = VS_PLAYING;
        }   break;
        case RKADK_PLAYER_STATE_PAUSE: {
            videoStatus = VS_PAUSED;
        }   break;
        case RKADK_PLAYER_STATE_EOF: {
            if (mLoopPlayBack) { start(); return; }
            videoStatus = VS_COMPLETED;
        }   break;
        case RKADK_PLAYER_STATE_INIT:
        case RKADK_PLAYER_STATE_STOP: {
            videoStatus = VS_STOPPED;
        }   break;
        default: {
            LOGE("unknow status=%d", status);
        }   break;
        }

        LOGI("MPGetStatus[%d-%d] VideoStatus[%d-%d]", pri->mStatus, status, mStatus, videoStatus);
        change = true;
        mStatus = videoStatus;
        pri->mStatus = status;
    }

    // 获取播放进度
    double progress;
    MPGetPosition(pri->mHandle, &progress);
    if (progress != pri->mProgress) {
        pri->mProgress = progress;
        change = true;
    }

    // 状态变更回调
    if (mChangeCallback && change) { mChangeCallback(*this, mDuration, progress, mStatus); }
    if (mStatus != VS_COMPLETED) View::postDelayed(mRunner, 100);
}

/// @brief 异步线程
void VideoView::onTask() {
}

/// @brief 读取帧
/// @param param 
/// @return 
void* VideoView::threadReadFrame(void* param) {
    return nullptr;
}

#endif /* CDROID_RK3506 */