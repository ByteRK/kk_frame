/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-24 10:07:01
 * @LastEditTime: 2026-10-06 14:25:20
 * @FilePath: /kk_frame/src/widgets/video_view.cc
 * @Description: 视频播放组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "video_view.h"
#include "env_utils.h"

#include <cstring>
#include <unistd.h>

/**
xml sample
<VideoView
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:url="test.mp4"
    android:loadPlay="true"
    android:loop="false"
    android:volume="70"/>
*/

DECLARE_WIDGET(VideoView)

VideoView::VideoView(int w, int h) : ImageView(w, h) {
    initViewData();
}

VideoView::VideoView(cdroid::Context* ctx, const AttributeSet& attrs) : ImageView(ctx, attrs) {
    initViewData();

    mURL = attrs.getString("url");
    mLoadPlay = attrs.getBoolean("loadPlay", mLoadPlay);
    mLoop = attrs.getBoolean("loop", !attrs.getBoolean("oneShot", true));
    mVolume = attrs.getInt("volume", mVolume);
    mProgressInterval = attrs.getInt("progressInterval", mProgressInterval);
    mUnsupportedText = attrs.getString("unsupportedText", mUnsupportedText);
    mUnsupportedTextSize = attrs.getDimensionPixelSize("unsupportedTextSize", mUnsupportedTextSize);
    setPointsFile(attrs.getString("pointsFile"));

    LOGI("video view created: url=%s loadPlay=%d loop=%d volume=%d", mURL.c_str(), mLoadPlay, mLoop, mVolume);
}

VideoView::~VideoView() {
    stopTicker();
    if (mPlayer) {
        mPlayer->setListener(nullptr);
        mPlayer->stop();
        delete mPlayer;
        mPlayer = nullptr;
    }
    if (mRegistered) {
        Looper* looper = Looper::getForThread();
        if (looper) looper->removeEventHandler(this);
        mRegistered = false;
    }
}

/// @brief 初始化控件
void VideoView::initViewData() {
    mTicker = std::bind(&VideoView::onTick, this);

    // 状态回调依赖Looper
    Looper* looper = Looper::getForThread();
    if (looper) {
        looper->addEventHandler(this);
        mRegistered = true;
    } else {
        LOGW("video view has no looper, status callback will not be dispatched");
    }
}

/// @brief 当前平台是否支持视频
/// @return true 支持，false 不支持
/// @note 不支持时控件显示提示文案
bool VideoView::isSupported() {
    return video::VideoPlayer::isSupported();
}

/// @brief 当前使用的视频后端名称
/// @return 后端名称
/// @note 日志需要使用
const char* VideoView::getBackend() {
    return video::VideoPlayer::getBackend();
}

/// @brief 开始播放
/// @return true 已受理（含等待布局后的延迟起播），false 失败
/// @note 会重新打开当前 URL；控件尚未测量完成时会等到 onLayout 再起播
bool VideoView::play() {
    if (!ensurePlayer()) {
        LOGW("video not supported on this platform: backend=%s", getBackend());
        invalidate();
        return false;
    }
    if (mURL.empty()) {
        LOGE("video url is empty");
        return false;
    }

    // GONE 状态下刚切为 VISIBLE、或尚未完成首次布局时，实测宽高仍为 0，
    // 此时不能把几何下发给底层，等 onLayout 拿到实测尺寸后再起播
    if (!isLayoutReady()) {
        LOGI("video play pending: view is not laid out yet, url=%s", mURL.c_str());
        mPendingPlay = true;
        mStatus = VS_INIT;
        return true;
    }

    return startPlay();
}

/// @brief 暂停
/// @return true 成功，false 失败
bool VideoView::pause() {
    if (mPlayer == nullptr) return false;
    stopTicker();
    return mPlayer->pause();
}

/// @brief 继续
/// @return true 成功，false 失败
bool VideoView::resume() {
    if (mPlayer == nullptr) return false;
    return mPlayer->resume();
}

/// @brief 停止播放
/// @note 会释放播放资源，并取消挂起的起播请求
void VideoView::over() {
    stopTicker();
    mPendingPlay = false;
    if (mPlayer) mPlayer->stop();

    mStatus = VS_NULL;
    mDuration = 0;
    mProgress = 0;
    invalidate();
}

/// @brief 是否处于播放中
/// @return true 播放中，false 暂停或停止
bool VideoView::isPlay() const {
    return mStatus == VS_PLAY || mStatus == VS_PAUSE;
}

/// @brief 获取播放状态
/// @return 播放状态
int VideoView::getStatus() const {
    return mStatus;
}

/// @brief 视频时长
/// @return 视频总时长，单位毫秒
double VideoView::getDuration() const {
    return mPlayer ? mPlayer->getDuration() : mDuration;
}

/// @brief 播放进度
/// @return 当前播放进度，单位毫秒
double VideoView::getProgress() const {
    return mPlayer ? mPlayer->getPosition() : mProgress;
}

/// @brief 跳转到指定进度
/// @param ms 指定进度，单位毫秒
void VideoView::setProgress(double ms) {
    mProgress = ms;
    if (mPlayer) mPlayer->seek(ms);
}

/// @brief 设置播放路径
/// @param url 视频路径
void VideoView::setURL(const std::string& url) {
    mURL = url;
}

/// @brief 是否循环播放
/// @param loop true 循环，false 不循环
void VideoView::setLoop(bool loop) {
    mLoop = loop;
    if (mPlayer) mPlayer->setLoop(loop);
}

/// @brief 设置音量
/// @param volume 音量，范围 0~100
void VideoView::setVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    mVolume = volume;
    if (mPlayer) mPlayer->setVolume(volume);
}

/// @brief 自定义视频窗口形状
/// @param points 形状点，至少 3 个点
void VideoView::setPoints(std::vector<Point>& points) {
    mPoints = points;
}

/// @brief 自定义视频窗口形状（文件读取）
/// @param fpath 文件路径
void VideoView::setPointsFile(const std::string& fpath) {
    if (fpath.empty()) return;
    if (access(fpath.c_str(), F_OK)) {
        LOGE("point file not exists. fpath=%s", fpath.c_str());
        return;
    }

    char buffer[4096];
    FILE* fp = fopen(fpath.c_str(), "r");
    if (fp == nullptr) {
        LOGE("point file open failed. fpath=%s", fpath.c_str());
        return;
    }
    const int rlen = static_cast<int>(fread(buffer, 1, sizeof(buffer) - 1, fp));
    fclose(fp);
    if (rlen <= 0) return;
    buffer[rlen] = '\0';
    if (rlen == static_cast<int>(sizeof(buffer) - 1)) LOGW("buffer maybe not enough!!!");

    Point pt;
    mPoints.clear();
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

/// @brief 设置平台不支持时显示的文案
/// @param text 文案
void VideoView::setUnsupportedText(const std::string& text) {
    mUnsupportedText = text;
    invalidate();
}

/// @brief 设置播放状态回调
/// @param l 回调函数
void VideoView::setOnPlayStatusChange(OnPlayStatusChange l) {
    mChangeCallback = l;
}

/// @brief 测量/布局回调
/// @param changed 是否尺寸变化
/// @param l 左边界
/// @param t 上边界
/// @param w 宽度
/// @param h 高度
void VideoView::onLayout(bool changed, int l, int t, int w, int h) {
    ImageView::onLayout(changed, l, t, w, h);

    syncGeometry();

    // 尺寸就绪后补发挂起的播放请求
    if (mPendingPlay && isLayoutReady()) {
        mPendingPlay = false;
        LOGI("video play resumed after layout: url=%s", mURL.c_str());
        startPlay();
        return;
    }

    if (mLoadPlay && !mURL.empty() && mStatus == VS_NULL) play();
}

/// @brief 页面绘制
/// @param canvas 画布
void VideoView::onDraw(Canvas& canvas) {
    if (!video::VideoPlayer::isSupported()) {
        drawUnsupported(canvas);
        return;
    }

    if (video::VideoPlayer::getMode() == video::VM_FRAME) {
        if (mFrameSurface == nullptr) return;
        canvas.save();
        clipRegion(canvas);
        ImageView::onDraw(canvas);
        canvas.restore();
        return;
    }

    // VM_OVERLAY：画面由硬件图层直接输出，这里把控件区域挖空即可
    if (mPlayer == nullptr || mStatus == VS_NULL) return;
    canvas.save();
    clipRegion(canvas);
    canvas.set_operator(Cairo::Context::Operator::CLEAR);
    canvas.paint();
    canvas.restore();
}

/// @brief 检查事件
/// @return 
int VideoView::checkEvents() {
    if (mPlayer && mPlayer->hasEvents()) return 1;
    return 0;
}

/// @brief 处理事件
/// @return 
int VideoView::handleEvents() {
    if (mPlayer == nullptr) return 0;
    mPlayer->dispatchEvents();
    return 1;
}

/// @brief 视频状态改变
/// @param status 播放状态
/// @param duration 视频总时长，单位毫秒
/// @param position 当前播放进度，单位毫秒
void VideoView::onVideoStatus(int status, double duration, double position) {
    mStatus = status;
    if (duration > 0) mDuration = duration;
    if (position > 0) mProgress = position;

    switch (status) {
    case VS_PLAY:
        startTicker();
        break;
    case VS_PAUSE:
    case VS_OVER:
    case VS_ERROR:
        stopTicker();
        if (status == VS_OVER) mProgress = mDuration;
        break;
    default:
        break;
    }

    notifyStatus(status);
    invalidate(false);
}

/// @brief 视频帧回调
void VideoView::onVideoFrame() {
    drawFrame();
}

/// @brief 可用性确认
/// @return true 可用，false 不可用
bool VideoView::ensurePlayer() {
    if (!video::VideoPlayer::isSupported()) return false;
    if (mPlayer) return true;

    mPlayer = video::VideoPlayer::create();
    if (mPlayer == nullptr) return false;

    mPlayer->setListener(this);
    mPlayer->setLoop(mLoop);
    mPlayer->setVolume(mVolume);
    if (mGeometry.w > 0 && mGeometry.h > 0) mPlayer->setGeometry(mGeometry);

    LOGI("video player created: backend=%s mode=%d", getBackend(), video::VideoPlayer::getMode());
    return true;
}

/// @brief 测量/布局是否已就绪
/// @return true 宽高均大于 0
bool VideoView::isLayoutReady() const {
    return getWidth() > 0 && getHeight() > 0;
}

/// @brief 真正发起播放（调用前需确保尺寸已就绪）
/// @return true 成功，false 失败
bool VideoView::startPlay() {
    if (mPlayer == nullptr) return false;

    stopTicker();
    syncGeometry();   // 窗口参数必须先于起播下发

    if (!mPlayer->open(mURL)) {
        LOGE("video open failed: %s", mURL.c_str());
        mStatus = VS_ERROR;
        notifyStatus(VS_ERROR);
        invalidate();
        return false;
    }

    mPlayer->setGeometry(mGeometry);
    mStatus = VS_INIT;
    mDuration = 0;
    mProgress = 0;

    if (!mPlayer->play()) {
        LOGW("video play failed: %s", mURL.c_str());
        mStatus = VS_ERROR;
        notifyStatus(VS_ERROR);
        return false;
    }

    invalidate(true);
    return true;
}

/// @brief 同步控件尺寸位置参数到底层播放器
void VideoView::syncGeometry() {
    // 未完成测量/布局时不做任何处理，由 onLayout 再次触发
    int w = getWidth();
    int h = getHeight();
    if (w <= 0 || h <= 0) return;

    int location[2] = { 0, 0 };
    getLocationInWindow(location);

    Point     screenSize;
    Display&  display = WindowManager::getInstance().getDefaultDisplay();
    display.getRealSize(screenSize);

    video::VideoGeometry geometry;
    geometry.x = location[0];
    geometry.y = location[1];
    geometry.w = w;
    geometry.h = h;
    geometry.rotation = WindowManager::getInstance().getDisplayRotation() * 90;
    geometry.screenWidth = screenSize.x;
    geometry.screenHeight = screenSize.y;

    int rotation = 0;
    if (
        EnvUtils::getInt("VIDEO_ROTATION", rotation) &&
        (rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270)
        ) {
        geometry.rotation = rotation;
        LOGI("video rotation override: %d", rotation);
    }

    if (mGeometry.x == geometry.x && mGeometry.y == geometry.y
        && mGeometry.w == geometry.w && mGeometry.h == geometry.h
        && mGeometry.rotation == geometry.rotation
        && mGeometry.screenWidth == geometry.screenWidth
        && mGeometry.screenHeight == geometry.screenHeight)
        return;

    mGeometry = geometry;
    LOGI("video geometry: [%d,%d,%d,%d] rotation=%d screen=%dx%d",
        geometry.x, geometry.y, geometry.w, geometry.h,
        geometry.rotation, geometry.screenWidth, geometry.screenHeight);
    if (mPlayer) mPlayer->setGeometry(geometry);
}

/// @brief 启动Tick心跳
void VideoView::startTicker() {
    if (mProgressInterval <= 0 || mTicking) return;
    mTicking = true;
    postDelayed(mTicker, mProgressInterval);
}

/// @brief 停止Tick心跳
void VideoView::stopTicker() {
    mTicking = false;
    removeCallbacks(mTicker);
}

/// @brief Tick心跳回调
void VideoView::onTick() {
    mTicking = false;
    if (mStatus != VS_PLAY || mProgressInterval <= 0) return;

    if (mPlayer) mProgress = mPlayer->getPosition();
    notifyStatus(VS_PLAY);
    startTicker();
}

/// @brief 绘制裁剪区域
/// @param canvas 画布
void VideoView::clipRegion(Canvas& canvas) {
    if (mPoints.empty()) {
        canvas.rectangle(0, 0, getWidth(), getHeight());
    } else {
        canvas.begin_new_path();
        canvas.move_to(mPoints[0].x, mPoints[0].y);
        for (size_t i = 1; i < mPoints.size(); i++) {
            const double x0 = mPoints[i - 1].x;
            const double y0 = mPoints[i - 1].y;
            const double x1 = mPoints[i].x;
            const double y1 = mPoints[i].y;
            canvas.curve_to(x0, y0, (x0 + x1) / 2.0, (y0 + y1) / 2.0, x1, y1);
        }
        canvas.close_path();
    }
    canvas.clip();
}

/// @brief 绘制不支持的提示文本
/// @param canvas 画布
void VideoView::drawUnsupported(Canvas& canvas) {
    const Rect rect = getClientRect();
    canvas.save();
    canvas.set_color(0xFFFFFFFFu);
    canvas.set_font_size(mUnsupportedTextSize);
    canvas.draw_text(rect, mUnsupportedText, Gravity::CENTER);
    canvas.restore();
}

/// @brief 绘制视频帧
void VideoView::drawFrame() {
    if (mPlayer == nullptr) return;

    video::VideoFrame frame;
    if (!mPlayer->pullFrame(frame)) return;

    if (frame.format != video::VF_BGRA32 || frame.data == nullptr) {
        LOGW("unsupported video frame format: %d", frame.format);
        mPlayer->releaseFrame(frame);
        return;
    }

    if (mFrameSurface == nullptr || mFrameWidth != frame.width || mFrameHeight != frame.height) {
        mFrameWidth = frame.width;
        mFrameHeight = frame.height;
        mFrameSurface = Cairo::ImageSurface::create(Cairo::Surface::Format::RGB24, mFrameWidth, mFrameHeight);
        if (mFrameSurface == nullptr) {
            mPlayer->releaseFrame(frame);
            return;
        }
        // BGRA32 与 cairo RGB24 图面内存布局一致，可直接整行拷贝
        setImageBitmap(mFrameSurface);
    }

    unsigned char* dst = mFrameSurface->get_data();
    const int      dstStride = mFrameSurface->get_stride();
    for (int y = 0; y < frame.height; y++) {
        std::memcpy(dst + static_cast<size_t>(y) * dstStride,
            frame.data + static_cast<size_t>(y) * frame.stride,
            static_cast<size_t>(frame.width) * 4);
    }
    mFrameSurface->mark_dirty();

    mPlayer->releaseFrame(frame);
    invalidate();
}

/// @brief 通知状态变化
/// @param status 播放状态
void VideoView::notifyStatus(int status) {
    if (mChangeCallback) mChangeCallback(*this, mDuration, mProgress, status);
}
