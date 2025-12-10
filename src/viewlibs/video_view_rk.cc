/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-05 19:33:30
 * @LastEditTime: 2025-12-08 17:19:21
 * @FilePath: /kk_frame/src/viewlibs/video_view_rk.cc
 * @Description: 视频播放组件 RK芯片版本
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#if defined CDROID_RK3506 || defined __VSCODE__

#include "video_view.h"

#include <cdplayer.h>
#include <unistd.h>

enum {
    RKADK_ROTATE_NONE,
    RKADK_ROTATE_90,
    RKADK_ROTATE_180,
    RKADK_ROTATE_270
};

typedef enum {
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
} RKADK_PLAYER_STATE_E;

//////////////////////////////////////////////////////////////////

// 视频信息
class VideoInfo {
public:
    VideoInfo() { }
    ~VideoInfo() { }
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

    mURL = attrs.getString("url");
    mLoadPlay = attrs.getBoolean("loadPlay", mLoadPlay);
    mOneShot = attrs.getBoolean("oneShot", mOneShot);
    mRunnerTime = attrs.getInt("checkTime", mRunnerTime);
    setPointsFile(attrs.getString("pointsFile"));
}

/// @brief 析构数据
VideoView::~VideoView() {
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
    }
}

/// @brief 初始化页面数据
void VideoView::initViewData() {
    mVideoInfo = nullptr;
    mHandle = nullptr;
    mLoadPlay = false;
    mRunner = [this]() {
        if (mDelayPlay)startPlay();
        else onTick();
    };
    mRunnerTime = 50;
    mDelayPlay = false;

    mURL = "";

    mError = 0;
    mStatus = VS_NULL;
    mLastCode = RKADK_PLAYER_STATE_IDLE;
    mProgress = 0;
    mDuration = 0;
    mChangeCallback = nullptr;
    mOneShot = true;
}

/// @brief 开始播放
void VideoView::startPlay() {
    // 停止上一个视频
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
    }

    // 初始化、复位、播放
    initVideo();
    mDelayPlay = false;
    MPPlay(mHandle);

    // 刷新正常页面
    invalidate(true);
    removeCallbacks(mRunner);
    View::postDelayed(mRunner, 1);
}

/// @brief 测量完成，用于自动播放视频
/// @param change 
/// @param l 
/// @param t 
/// @param w 
/// @param h 
void VideoView::onLayout(bool change, int l, int t, int w, int h) {
    if (mStatus != VS_NULL) {
        LOGE("status not null. status=%d", mStatus);
        return;
    }

    // 自动播放
    if (mLoadPlay && !mURL.empty()) {
        LOGI("layout play video. url=%s", mURL.c_str());
        play();
    }
}

/// @brief UI图层绘制
/// @param canvas 
void VideoView::onDraw(Canvas& canvas) {
    if (mStatus == VS_NULL) return;

    // 对UI层打孔
    if (mPoints.empty()) { // 矩形孔
        canvas.rectangle(0, 0, getWidth(), getHeight());
    } else { // 异形孔
        canvas.begin_new_path();
        canvas.move_to(mPoints[0].x, mPoints[0].y);
        for (int i = 1; i < mPoints.size(); i++) {
            double x0 = mPoints[i - 1].x;
            double y0 = mPoints[i - 1].y;
            double x1 = mPoints[i].x;
            double y1 = mPoints[i].y;
            double xc = (x0 + x1) / 2.0;
            double yc = (y0 + y1) / 2.0;
            canvas.curve_to(x0, y0, xc, yc, x1, y1);
        }
        canvas.close_path();
    }

    // 剪切
    canvas.clip();
    canvas.set_operator(Cairo::Context::Operator::CLEAR);
    canvas.paint();
}

/// @brief 
void VideoView::onTick() {
    bool change = false;

    // 故障处理
    if (mError) {
        LOGE("Play error, code=%d url=%s", mError, mURL.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_OVER);
        return;
    }

    // 句柄异常
    if (!mHandle) {
        LOGE("Handle is null, url=%s", mURL.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_OVER);
        return;
    }

    /* 获取状态 */
    int status = MPGetStatus(mHandle);
    if (status != mLastCode) {
        int videoStatus = mStatus;
        switch (status) {
        case RKADK_PLAYER_STATE_PLAY: {
            videoStatus = VS_PLAY;
        }   break;
        case RKADK_PLAYER_STATE_PAUSE: {
            videoStatus = VS_PAUSE;
        }   break;
        case RKADK_PLAYER_STATE_INIT:
        case RKADK_PLAYER_STATE_PREPARED:
        case RKADK_PLAYER_STATE_EOF:
        case RKADK_PLAYER_STATE_STOP: {
            videoStatus = VS_OVER;
        }   break;
        default:
            mError = status;
            break;
        }

        LOGI("MPGetStatus[%d-%d] VideoStatus[%d-%d]", mLastCode, status, mStatus, videoStatus);
        change = true;
        mStatus = videoStatus;
        mLastCode = status;
    }

    // 获取视频时长
    if (mDuration == 0) {
        mDuration = getDuration();
        change = true;
    }

    // 获取播放进度
    double progress;
    MPGetPosition(mHandle, &progress);
    if (progress != mProgress) {
        mProgress = progress;
        change = true;
    }

    // 状态变更回调
    if (mChangeCallback && change) { mChangeCallback(*this, mDuration, mProgress, mStatus); }
    View::postDelayed(mRunner, mRunnerTime);
}

/// @brief 开始播放
bool VideoView::play() {
    if (mStatus != VS_NULL && mStatus != VS_OVER) {
        LOGW("status not null. status=%d", mStatus);
        return false;
    }

    // 延迟播放，使组件先行测量
    mDelayPlay = true;
    removeCallbacks(mRunner);
    View::postDelayed(mRunner, 20);

    return true;
}

/// @brief 是否播放中
/// @return 
bool VideoView::isPlay() {
    return mStatus >= VS_PLAY;
}

/// @brief 暂停播放
/// @return 
bool VideoView::pause() {
    if (mStatus != VS_INIT && mStatus != VS_PLAY) {
        LOGW("video not play. status=%d", mStatus);
        return false;
    }

    MPPause(mHandle);
    invalidate();
    return true;
}

/// @brief 恢复播放
/// @return 
bool VideoView::resume() {
    if (mStatus != VS_PAUSE) {
        LOGW("video not pause. status=%d", mStatus);
        return false;
    }
    MPResume(mHandle);
    invalidate();
    return true;
}

/// @brief 结束播放
void VideoView::over() {
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
        mDuration = 0;
        invalidate();
    }
    removeCallbacks(mRunner);
}

/// @brief 设置URL
/// @param url 
void VideoView::setURL(const std::string& url) {
    mURL = url;
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

/// @brief 设置打孔范围
/// @param points 
void VideoView::setPoints(std::vector<Point>& points) {
    mPoints = points;
}

/// @brief 获取时长
/// @return 
double VideoView::getDuration() {
    MPGetDuration(mHandle, &mDuration);
    return mDuration;
}

/// @brief 获取进度
/// @return 
double VideoView::getProgress() {
    return mProgress;
}

/// @brief 设置进度
/// @param dp 
void VideoView::setProgress(double dp) {
    MPSeek(mHandle, dp);
}

/// @brief 获取当前状态
/// @return 
int VideoView::getStatus() {
    return mStatus;
}

/// @brief 初始化视频
void VideoView::initVideo() {
    if (mStatus != VS_NULL) {
        LOGW("status not null. status=%d", mStatus);
        return;
    }

    int x, y, w, h;

    x = getLeft();
    y = getTop();
    w = getWidth();
    h = getHeight();

    w = w <= 0 ? 480 : w;
    h = h <= 0 ? 800 : h;

    /* 获取窗口位置 */
    int loc[2] = { 0 };
    getLocationInWindow(loc);

    /* 获取旋转角度 */
    int rotate = WindowManager::getInstance().getDisplayRotation();
    if (getenv("DISPLAY_ROTATE")) {
        int hwRotate = atoi(getenv("DISPLAY_ROTATE")) % 360;
        if (hwRotate % 90 == 0)  rotate -= hwRotate / 90;
    }

    /* 获取屏幕大小 */
    Point screenSize;
    WindowManager::getInstance().getDefaultDisplay().getSize(screenSize);

    LOGV("url=%s x=%d y=%d w=%d h=%d loc[0]=%d loc[1]=%d rotate=%d sw=%d sh=%d"
        , mURL.c_str(), x, y, w, h, loc[0], loc[1], rotate, screenSize.x, screenSize.y);

    /* 获取显示大小 */
    UINT sw, sh;
    GFXGetDisplaySize(0, &sw, &sh);

    /* 开启视频 */
    mHandle = MPOpen(mURL.c_str());


#define SET_VEDIO_WINDOWS(x,y,w,h) \
        MPSetWindow(mHandle, x, y, w, h); \
        LOGI("MPSetWindow(%d, %d, %d, %d)", x, y, w, h);

    /* 视频旋转 方向与UI相反 */
    switch (rotate) {
    case Display::ROTATION_0: {
        LOGI("MPRotate(%d, RKADK_ROTATE_NONE)", mHandle);
        MPRotate(mHandle, RKADK_ROTATE_NONE);
        SET_VEDIO_WINDOWS(loc[0], loc[1], w, h);
    }   break;
    case Display::ROTATION_90: {
        LOGI("MPRotate(%d, RKADK_ROTATE_270)", mHandle);
        MPRotate(mHandle, RKADK_ROTATE_270);
        SET_VEDIO_WINDOWS(loc[1], sh - w - loc[0], h, w);
    }   break;
    case Display::ROTATION_180: {
        LOGI("MPRotate(%d, RKADK_ROTATE_180)", mHandle);
        MPRotate(mHandle, RKADK_ROTATE_180);
        SET_VEDIO_WINDOWS(loc[0], loc[1], w, h);
    }   break;
    case Display::ROTATION_270: {
        LOGI("MPRotate(%d, RKADK_ROTATE_90)", mHandle);
        MPRotate(mHandle, RKADK_ROTATE_90);
        SET_VEDIO_WINDOWS(sw - h - loc[1], loc[0], h, w);
    }   break;
    default: {
        LOGE("unknow rotate=%d", rotate);
    }   break;
    }

#undef SET_VEDIO_WINDOWS

    /* 记录状态 */
    mStatus = VS_INIT;
}

int VideoView::checkEvents() {
    return 0;
}

int VideoView::handleEvents() {
    return 0;
}

#endif /* CDROID_RK3506 */