#if defined CDROID_RK3506 || defined __VSCODE__

#include "video_view.h"

#include <cdplayer.h>
#include <unistd.h>

/*
xml sample 1280*480
<VideoView
    android:layout_width="640dp"
    android:layout_height="360dp"
    android:layout_marginTop="120dp"
    android:layout_marginLeft="320dp"
    android:url="test.mp4"
    android:loadPlay="true"/>
    */

enum { AV_ROTATE_NONE, AV_ROTATE_90, AV_ROTATE_180, AV_ROTATE_270 };

#define AV_NOTHING (0x0000)
#define AV_AUDIO_COMPLETE (0x0001)
#define AV_VIDEO_COMPLETE (0x0002)
#define AV_PLAY_PAUSE (0x0004)
#define AV_ACODEC_ERROR (0x0008)
#define AV_VCODEC_ERROR (0x0010)
#define AV_NOSYNC (0x0020)
#define AV_READ_TIMEOUT (0x0040)
#define AV_NO_NETWORK (0x0080)
#define AV_INVALID_FILE (0x0100)
#define AV_AUDIO_MUTE (0x0200)
#define AV_AUDIO_PAUSE (0x0400)
#define AV_PLAY_LOOP (0x0800)

#define AV_PLAY_COMPLETE (AV_AUDIO_COMPLETE | AV_VIDEO_COMPLETE)
#define AV_PLAY_ERROR                                                                                                  \
    (AV_ACODEC_ERROR | AV_VCODEC_ERROR | AV_NOSYNC | AV_READ_TIMEOUT | AV_NO_NETWORK | AV_INVALID_FILE)

//////////////////////////////////////////////////////////////////

// 视频信息
class VideoInfo {
public:
    VideoInfo() { }
    ~VideoInfo() { }
};

//////////////////////////////////////////////////////////////////
DECLARE_WIDGET(VideoView)

VideoView::VideoView(int w, int h) : ImageView(w, h) {
    initViewData();
}

VideoView::VideoView(Context* ctx, const AttributeSet& attrs) : ImageView(ctx, attrs) {
    initViewData();

    mXMLWidth = attrs.getLayoutDimension("layout_width", -1);
    mXMLHeight = attrs.getLayoutDimension("layout_height", -1);
    LOGE("INFO VIDEOVIEW GET SIZE FORM XML: %d %d", mXMLWidth, mXMLHeight);

    mURL = attrs.getString("url");
    mLoadPlay = attrs.getBoolean("loadPlay", mLoadPlay);
    mOneShot = attrs.getBoolean("oneShot", mOneShot);
    mCheckTime = attrs.getInt("checkTime", mCheckTime);
    setPointsFile(attrs.getString("pointsFile"));
}

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
    mError = 0;
    mMaxTime = 0;
    mCurTime = 0;
    mHandle = 0;
    mStatus = VS_NULL;
    mLastCode = AV_NOTHING;
    mLoadPlay = false;
    mOneShot = true;
    mCheckTime = 50;

    mTicker = [this]() {if (mDelayPlay == 1)delayPlay();else onTick();};
    mChangeCallback = nullptr;
    mDuration = 0;
    mProgress = 0;
    mVideoInfo = nullptr;

    mDelayPlay = 0;
}

void VideoView::delayPlay() {
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
    }
    initVideo();

    MPPlay(mHandle);

    invalidate(true);
    mDelayPlay = 0;
    removeCallbacks(mTicker);
    View::postDelayed(mTicker, 1);
}

void VideoView::onLayout(bool change, int l, int t, int w, int h) {
    if (mStatus != VS_NULL) {
        LOGE("status not null. status=%d", mStatus);
        return;
    }

    if (mLoadPlay && !mURL.empty()) {
        LOGI("layout play video. url=%s", mURL.c_str());
        play();
    }
}

void VideoView::onDraw(Canvas& canvas) {
    if (mStatus == VS_NULL) return;
    if (mPoints.empty()) {
        canvas.rectangle(0, 0, getWidth(), getHeight());
    } else {
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
    canvas.clip();
    canvas.set_operator(Cairo::Context::Operator::CLEAR);
    canvas.paint();
}

void VideoView::onTick() {
    bool change = false;

    if (mError) {
        LOGE("Play error, code=%d url=%s", mError, mURL.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_OVER);
        return;
    }

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
        case 3: {
            videoStatus = VS_PLAY;
        }   break;
        case 5: {
            videoStatus = VS_PAUSE;
        }   break;
        case 1:
        case 2:
        case 4:
        case 6: {
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

    if (mDuration == 0) {
        mDuration = getDuration();
        change = true;
    }
    double progress;
    MPGetPosition(mHandle, &progress);
    if (progress != mProgress) {
        mProgress = progress;
        change = true;
    }

    if (mChangeCallback && change) { mChangeCallback(*this, mDuration, mProgress, mStatus); }
    View::postDelayed(mTicker, mCheckTime);
}

bool VideoView::play() {
    if (mStatus != VS_NULL && mStatus != VS_OVER) {
        LOGW("status not null. status=%d", mStatus);
        return false;
    }

    mDelayPlay = 1;
    removeCallbacks(mTicker);
    View::postDelayed(mTicker, 20);

    return true;
}

bool VideoView::isPlay() {
    return mStatus >= VS_PLAY;
}

bool VideoView::pause() {
    if (mStatus != VS_INIT && mStatus != VS_PLAY) {
        LOGW("video not play. status=%d", mStatus);
        return false;
    }

    MPPause(mHandle);
    invalidate();
    return true;
}

bool VideoView::resume() {
    if (mStatus != VS_PAUSE) {
        LOGW("video not pause. status=%d", mStatus);
        return false;
    }
    MPResume(mHandle);
    invalidate();
    return true;
}

void VideoView::over() {
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
        invalidate();
    }
    removeCallbacks(mTicker);
}

void VideoView::setURL(const std::string& url) {
    mURL = url;
}

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
    LOGE("x=%d y=%d w=%d h=%d", x, y, w, h);

    w = w <= 0 ? mXMLWidth : w;
    h = h <= 0 ? mXMLHeight : h;
    LOGE("x=%d y=%d w=%d h=%d", x, y, w, h);

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

    LOGI("url=%s x=%d y=%d w=%d h=%d loc[0]=%d loc[1]=%d rotate=%d sw=%d sh=%d"
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
        LOGI("MPRotate(%d, AV_ROTATE_NONE)", mHandle);
        MPRotate(mHandle, AV_ROTATE_NONE);
        SET_VEDIO_WINDOWS(loc[0], loc[1], w, h);
    }   break;
    case Display::ROTATION_90: {
        LOGI("MPRotate(%d, AV_ROTATE_270)", mHandle);
        MPRotate(mHandle, AV_ROTATE_270);
        SET_VEDIO_WINDOWS(loc[1], sh - w - loc[0], h, w);
    }   break;
    case Display::ROTATION_180: {
        LOGI("MPRotate(%d, AV_ROTATE_180)", mHandle);
        MPRotate(mHandle, AV_ROTATE_180);
        SET_VEDIO_WINDOWS(loc[0], loc[1], w, h);
    }   break;
    case Display::ROTATION_270: {
        LOGI("MPRotate(%d, AV_ROTATE_90)", mHandle);
        MPRotate(mHandle, AV_ROTATE_90);
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