#include "video_view.h"

#include <cdplayer.h>
#include <unistd.h>

#define DISABLE_VIDEO_VIEW

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

#if defined(PRODUCT_X64) || defined(DISABLE_VIDEO_VIEW)
#define SUPPORT_FFMPEG_YUV 0
#define SUPPORT_FFMPEG_RGB 0
#else
#define SUPPORT_FFMPEG_YUV 1
#define SUPPORT_FFMPEG_RGB 0
#endif

//////////////////////////////////////////////////////////////////

#if SUPPORT_FFMPEG_RGB
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif
#endif

// 视频信息
class VideoInfo {
public:
    struct AVRgbData {
        int   idx;
        short width;
        short height;
#if SUPPORT_FFMPEG_RGB
        AVFrame *pFrameRGB;
#endif
    };

public:
    VideoInfo() {
#if SUPPORT_FFMPEG_RGB
        pFormatCtx     = 0;
        pCodecParams   = 0;
        pCodecCtx      = 0;
        pCodec         = 0;
        videoStreamIdx = -1;
        pSwsCtx        = 0;
        pFrame         = 0;
#endif
        width      = 0;
        height     = 0;
        duration   = 0;
        frameCount = 0;
        readCount  = 0;
        fps        = 0;
        lastFrame  = 0;
        bzero(showTimes, sizeof(showTimes));
    }

    ~VideoInfo() {
#if SUPPORT_FFMPEG_RGB
        if (pFormatCtx) {
            av_frame_free(&pFrame);
            sws_freeContext(pSwsCtx);
            avcodec_free_context(&pCodecCtx);
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
        }
        for (AVRgbData *vd : videoFrames) {
            if (vd->pFrameRGB) av_frame_free(&vd->pFrameRGB);
            free(vd);
        }
        videoFrames.clear();
#endif
    }

private:
    void readFrame() {
#if SUPPORT_FFMPEG_RGB
        if (!pFrame) pFrame = av_frame_alloc();

        AVPacket packet, *pPacket;
        bool     unrefPacket;
        pPacket     = &packet;
        unrefPacket = false;

        while (av_read_frame(pFormatCtx, pPacket) >= 0) {
            // 视频帧
            if (pPacket->stream_index == videoStreamIdx) {
                // 解码
                int ret = avcodec_send_packet(pCodecCtx, pPacket);
                if (ret < 0) {
                    // 发送数据包失败
                    // 错误处理
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        // 需要更多数据或解码完成
                        break;
                    } else if (ret < 0) {
                        // 解码失败
                        // 错误处理
                        break;
                    }

                    // 在这里处理解码后的视频帧 frame
                    AVFrame *pFrameRGB = av_frame_alloc();

                    // 设置输出缓冲区
                    uint8_t *outBuffer = (uint8_t *)av_malloc(
                        av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1));
                    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, outBuffer, AV_PIX_FMT_RGB24,
                                         pCodecCtx->width, pCodecCtx->height, 1);
                    sws_scale(pSwsCtx, (const uint8_t *const *)pFrame->data, pFrame->linesize, 0, pFrame->height,
                              pFrameRGB->data, pFrameRGB->linesize);
                    // 将数据以二进制的形式写入文件中
                    // fwrite(pframeRGB->data[0],pCodecCtx->height * pCodecCtx->width*3,1,fp);

                    AVRgbData *rgbData = (AVRgbData *)calloc(1, sizeof(AVRgbData));
                    rgbData->width     = pCodecCtx->width;
                    rgbData->height    = pCodecCtx->height;
                    rgbData->pFrameRGB = pFrameRGB;
                    rgbData->idx       = ++readCount;
                    videoFrames.push_back(rgbData);
                }

                unrefPacket = true;
                break;
            }
            av_packet_unref(pPacket);
        }
        if (unrefPacket) av_packet_unref(pPacket);
#endif
    }

public:
    int hasFrame() {
        if (readCount >= frameCount) return 0;

        if (videoFrames.empty()) {
            return 1; // 读取下一帧
        }

        int64_t nowms    = SystemClock::uptimeMillis();
        int     frame_ms = 1000 / fps;
        if (nowms - lastFrame < frame_ms) {
            // 两次显示时间间隔
            if (showTimes[0] > 0 && showTimes[1] > 0) {
                int d = showTimes[1] - showTimes[0];
                if (d > frame_ms) {
                    if (nowms - lastFrame >= frame_ms - (d - frame_ms)) { return 1; }
                }
            }

            return 0;
        }
        return 1;
    }

    AVRgbData *getData() {
        if (videoFrames.empty()) {
            readFrame();
            LOGV("read at %d/%d", readCount, frameCount);
            return 0;
        }
        AVRgbData *dat = videoFrames.front();
        videoFrames.pop_front();
        return dat;
    }

    void freeData(AVRgbData *dat) {
        LOGV("show at %d/%d", dat->idx, frameCount);
#if SUPPORT_FFMPEG_RGB
        if (dat->pFrameRGB) av_frame_free(&dat->pFrameRGB);
#endif
        free(dat);
        lastFrame = SystemClock::uptimeMillis();
        if (showTimes[0] > 0 && showTimes[1] > 0) {
            showTimes[0] = showTimes[1];
            showTimes[1] = lastFrame;
            // LOGW("%ld-%ld=%ld", showTimes[1], showTimes[0], showTimes[1] - showTimes[0]);
        } else if (showTimes[0] > 0) {
            showTimes[1] = lastFrame;
        } else {
            showTimes[0] = lastFrame;
        }
    }

    int setFile(const char *filename) {
#if SUPPORT_FFMPEG_RGB
        if (pFormatCtx) return -1;
        unsigned int i;
        int          video_index;
        int64_t      baseDuration;
        AVRational   timeBase;
        AVStream    *videoStream;

        int ret = avformat_open_input(&pFormatCtx, filename, nullptr, nullptr);
        if (ret < 0) {
            printf("avformat_open_input result fail. ret=%d\n", ret);
            return -1;
        }

        ret = avformat_find_stream_info(pFormatCtx, nullptr);
        if (ret < 0) {
            printf("avformat_find_stream_info result fail. ret=%d\n", ret);
            goto error_end;
        }

        video_index = -1;
        for (i = 0; i < pFormatCtx->nb_streams; i++) {
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_index = i;
                break;
            }
        }
        if (video_index == -1) {
            printf("avformat_find_stream_info get video fail. count=%u\n", pFormatCtx->nb_streams);
            ret = -101;
            goto error_end;
        }

        videoStreamIdx = video_index;
        videoStream    = pFormatCtx->streams[video_index];
        pCodecParams   = videoStream->codecpar;
        pCodecCtx      = avcodec_alloc_context3(NULL);
        if (avcodec_parameters_to_context(pCodecCtx, pCodecParams) < 0) {
            // 处理错误
            printf("parameters to context fail\n");
        }
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == NULL) {
            // 找不到解码器
            printf("not find decoder\n");
        }
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
            // 处理错误
            printf("avcodec open fail\n");
        }
        pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
                                 pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        if (pSwsCtx == NULL) {
            // 处理错误
            printf("sws get context fail\n");
        }
        // 视频像素
        width  = videoStream->codecpar->width;
        height = videoStream->codecpar->height;
        // 获取视频时长
        baseDuration = videoStream->duration;                      // 基于时间基数的时长
        timeBase     = videoStream->time_base;                     // 时间基数
        duration     = baseDuration * timeBase.num / timeBase.den; // s
        // 帧数 帧率
        // 获取视频帧数
        frameCount = videoStream->nb_frames;
        fps        = av_q2d(videoStream->r_frame_rate);

    error_end:
        if (ret < 0) { return -1; }
#else
        GFXGetDisplaySize(0, (UINT*)&width, (UINT*)&height);
        duration = 0;
#endif
        return 0;
    }

public:
#if SUPPORT_FFMPEG_RGB
    AVFormatContext   *pFormatCtx;
    AVCodecParameters *pCodecParams;
    AVCodecContext    *pCodecCtx;
    const AVCodec     *pCodec;
    int                videoStreamIdx;
    struct SwsContext *pSwsCtx;
    AVFrame           *pFrame;
#endif
    int                    width;        // 宽
    int                    height;       // 高
    int                    duration;     // 时长
    int                    frameCount;   // 最大帧数
    int                    readCount;    // 读取帧数
    int                    fps;          // 帧率
    int64_t                lastFrame;    // 上一帧显示时间
    int64_t                showTimes[2]; // 最后两帧时间
    std::list<AVRgbData *> videoFrames;  // 缓存帧
};

//////////////////////////////////////////////////////////////////
DECLARE_WIDGET(VideoView)
VideoView::VideoView(int w, int h) : ImageView(w, h) {
    initViewData();
}

VideoView::VideoView(Context *ctx, const AttributeSet &attrs) : ImageView(ctx, attrs) {
    initViewData();

    mXMLWidth = attrs.getLayoutDimension("layout_width", -1);
    mXMLHeight = attrs.getLayoutDimension("layout_height", -1);
    LOGE("INFO VIDEOVIEW GET SIZE FORM XML: %d %d", mXMLWidth, mXMLHeight);

    mURL      = attrs.getString("url");
    mLoadPlay = attrs.getBoolean("loadPlay", mLoadPlay);
    mOneShot  = attrs.getBoolean("oneShot", mOneShot);
    mCheckTime= attrs.getInt("checkTime", mCheckTime);
    setPointsFile(attrs.getString("pointsFile"));

#if SUPPORT_FFMPEG_RGB
    setBackgroundColor(Color::GRAY);
#endif
}

VideoView::~VideoView() {
#if SUPPORT_FFMPEG_YUV
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
    }
#endif

#if SUPPORT_FFMPEG_RGB
    __del(mVideoInfo);
    Looper::getForThread()->removeEventHandler(this);
#endif
}

void VideoView::initViewData() {
    mError    = 0;
    mMaxTime  = 0;
    mCurTime  = 0;
    mHandle   = 0;
    mStatus   = VS_NULL;
    mLastCode = AV_NOTHING;
    mLoadPlay = false;
    mOneShot  = true;
    mCheckTime = 50;

    mTicker         = std::bind(&VideoView::onTick, this);
    mChangeCallback = nullptr;
    mDuration       = 0;
    mProgress       = 0;
    mVideoInfo      = 0;

#if SUPPORT_FFMPEG_RGB
    mLastCode  = mStatus;
    Looper::getForThread()->addEventHandler(this);
#endif
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

void VideoView::onDraw(Canvas &canvas) {
    if (mStatus == VS_NULL) return;
#if SUPPORT_FFMPEG_YUV
    if (mPoints.empty()) {
        canvas.rectangle(0, 0, getWidth(), getHeight());
    } else {
        canvas.begin_new_path();
        canvas.move_to(mPoints[0].x, mPoints[0].y);
#if 1
        for (int i = 1; i < mPoints.size(); i++) {
            double x0 = mPoints[i - 1].x;
            double y0 = mPoints[i - 1].y;
            double x1 = mPoints[i].x;
            double y1 = mPoints[i].y;
            double xc = (x0 + x1) / 2.0;
            double yc = (y0 + y1) / 2.0;
            canvas.curve_to(x0, y0, xc, yc, x1, y1);
        }
#else
        for (int i = 1; i < mPoints.size(); i++) {
            canvas.line_to(mPoints[i].x, mPoints[i].y);
        }
#endif
        canvas.close_path();
    }
    canvas.clip();
    canvas.set_operator(Cairo::Context::Operator::CLEAR);
    canvas.paint();
#else
    ImageView::onDraw(canvas);
#endif
}

void VideoView::onTick() {
    bool change = false;

    if (mError) {
        LOGE("Play error, code=%d url=%s", mError, mURL.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_OVER);
        return;
    }

#if SUPPORT_FFMPEG_YUV
    if (!mHandle) {
        LOGE("Handle is null, url=%s", mURL.c_str());
        if (mChangeCallback) mChangeCallback(*this, 0, 0, VS_OVER);
        return;
    }
    /* 获取状态 */
    int status = MPGetStatus(mHandle);
    if (status != mLastCode) {
        int videoStatus = mStatus;
        if (status < 0) {
            mError = status;
        } else if (status == AV_NOTHING || status == AV_PLAY_LOOP) {
            videoStatus = VS_PLAY;
        } else if (status & AV_PLAY_PAUSE) {
            videoStatus = VS_PAUSE;
        } else if (status & AV_PLAY_COMPLETE) {
            videoStatus = VS_OVER;
        } else {
            mError = status;
        }
        LOGI("MPGetStatus[%d-%d] VideoStatus[%d-%d]", mLastCode, status, mStatus, videoStatus);
        change    = true;
        mStatus   = videoStatus;
        mLastCode = status;
    }

#elif SUPPORT_FFMPEG_RGB
    /* 获取状态 */
    if (mStatus != mLastCode) {
        LOGI("status change. from=%d to=%d", mLastCode, mStatus);
        mLastCode = mStatus;
        change    = true;
    }

#endif

    if (mDuration == 0) {
        mDuration = getDuration();
        change    = true;
    }
    double progress;
#if SUPPORT_FFMPEG_YUV
    MPGetPosition(mHandle, &progress);
#elif SUPPORT_FFMPEG_RGB
    if (mVideoInfo->frameCount > 0) {
        progress = mVideoInfo->readCount * mVideoInfo->duration / mVideoInfo->frameCount;
    }
#endif
    if (progress != mProgress) {
        mProgress = progress;
        change    = true;
    }

    if (mChangeCallback && change) { mChangeCallback(*this, mDuration, mProgress, mStatus); }
    View::postDelayed(mTicker, mCheckTime);
}

bool VideoView::play() {
    if (mStatus != VS_NULL && mStatus != VS_OVER) {
        LOGW("status not null. status=%d", mStatus);
        return false;
    }

#if SUPPORT_FFMPEG_YUV
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
    }
#endif

    initVideo();

#if SUPPORT_FFMPEG_YUV
    MPPlay(mHandle);
#else
    mStatus = VS_PLAY;
#endif

    invalidate(true);
    View::postDelayed(mTicker, 1);

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

#if SUPPORT_FFMPEG_YUV
    MPPause(mHandle);
#else
    mStatus = VS_PAUSE;
#endif
    invalidate();
    return true;
}

bool VideoView::resume() {
    if (mStatus != VS_PAUSE) {
        LOGW("video not pause. status=%d", mStatus);
        return false;
    }
#if SUPPORT_FFMPEG_YUV
    MPResume(mHandle);
#else
    mStatus = VS_PLAY;
#endif
    invalidate();
    return true;
}

void VideoView::over() {
#if SUPPORT_FFMPEG_YUV
    if (mHandle) {
        if (mStatus > VS_INIT && mStatus < VS_OVER) { MPStop(mHandle); }
        MPClose(mHandle);
        mHandle = 0;
        mStatus = VS_NULL;
        invalidate();
    }
#elif SUPPORT_FFMPEG_RGB
    __del(mVideoInfo);
    mStatus = VS_NULL;
#endif
    removeCallbacks(mTicker);
}

void VideoView::setURL(const std::string &url) {
    mURL = url;
}

void VideoView::setPointsFile(const std::string &fpath){
    if (fpath.empty()) return;
    if (access(fpath.c_str(), F_OK)) {
        LOGE("point file not exists. fpath=%s", fpath.c_str());
        return;
    }
    char buffer[4096];
    FILE *fp = fopen(fpath.c_str(), "r");
    int rlen = fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);
    buffer[rlen - 1] = '\0';
    if (rlen == sizeof(buffer) - 1) LOGW("buffer maybe not enough!!!");
    Point pt;
    for (char *p = buffer; *p; p++) {
        if (*p == '{') {
            pt.x = atoi(p + 1);
        } else if (*p == ',' && *(p + 1) >= '0' && *(p + 1) <= '9') {
            pt.y = atoi(p + 1);
            mPoints.push_back(pt);
        }
    }
    LOGI("Read point over. count=%d", mPoints.size());
}

void VideoView::setPoints(std::vector<Point> &points) {
    mPoints = points;
}

double VideoView::getDuration() {
#if SUPPORT_FFMPEG_YUV
    MPGetDuration(mHandle, &mDuration);
#elif SUPPORT_FFMPEG_RGB
    mDuration = mVideoInfo->duration;
#endif
    return mDuration;
}

double VideoView::getProgress() {
    return mProgress;
}

void VideoView::setProgress(double dp)
{
#if SUPPORT_FFMPEG_YUV
    MPSeek(mHandle,dp);
#endif
}

int VideoView::getStatus() {
    return mStatus;
}

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
    LOGV("x=%d y=%d w=%d h=%d", x, y, w, h);

    w = w <= 0 ? mXMLWidth : w;
    h = h <= 0 ? mXMLHeight : h;

    int loc[2] = {0};
    getLocationInWindow(loc);

    const int rotate = WindowManager::getInstance().getDisplayRotation();
    Point screenSize;
    WindowManager::getInstance().getDefaultDisplay().getSize(screenSize);

    LOGI("url=%s x=%d y=%d w=%d h=%d loc[0]=%d loc[1]=%d rotate=%d sw=%d sh=%d"
        , mURL.c_str(), x, y, w, h, loc[0], loc[1], rotate, screenSize.x, screenSize.y);

#if SUPPORT_FFMPEG_YUV
    UINT sw, sh;
    GFXGetDisplaySize(0, &sw, &sh);
    mHandle = MPOpen(mURL.c_str());
    /* MPSetWindow(mHandle, loc[0], loc[1], h, w);// x y w h偏移可以改x y,例如：loc[0]+160 */
    // MPSetWindow(mHandle, screenSize.y - loc[1] - h, loc[0], h, w);
    MPSetWindow(mHandle, loc[0], loc[1], w, h);
    /* 视频旋转 方向与UI相反 */
    switch (rotate) {
        case Display::ROTATION_270:
            MPRotate(mHandle, AV_ROTATE_90);
            break;
        case Display::ROTATION_90:
            MPSetWindow(mHandle, loc[1], sh - w - loc[0], h, w);
            MPRotate(mHandle, AV_ROTATE_270);
            break;
        case Display::ROTATION_180:
            break;
    }
    // if (!mOneShot) MPLoop(mHandle, true);
    // MPVideoOnly(mHandle, true);
#elif SUPPORT_FFMPEG_RGB
    __del(mVideoInfo);
    mVideoInfo = new VideoInfo();
    mError = mVideoInfo->setFile(mURL.c_str());
#endif
    mStatus = VS_INIT;
}

int VideoView::checkEvents() {
#if SUPPORT_FFMPEG_RGB
    if (mStatus == VS_PLAY) {
        return mVideoInfo->hasFrame();
    } else {
        return 0;
    }
#endif
    return 0;
}

int VideoView::handleEvents() {
#if SUPPORT_FFMPEG_RGB
    VideoInfo::AVRgbData *vd = mVideoInfo->getData();
    if (!vd) return 0;

    uint8_t *rgbData = vd->pFrameRGB->data[0]; // RGB数据
    int      width   = vd->width;              // 图像宽度
    int      height  = vd->height;             // 图像高度

    if (!rgbData) {
        mVideoInfo->freeData(vd);
        return 0;
    }

    Cairo::RefPtr<Cairo::ImageSurface> img = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
    // 获取ImageSurface的数据
    unsigned char *imageSurfaceData = img->get_data();
    int            stride           = img->get_stride();

    LOGD("index=%d width=%d height=%d data=%p stride=%d", vd->idx, vd->width, vd->height, rgbData, stride);

    // 将RGB数据写入ImageSurface
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 计算当前像素的偏移量
            int offset = y * stride + x * 4;
            // 将RGB数据写入ImageSurface
            imageSurfaceData[offset + 3] = 0xFF;
            imageSurfaceData[offset + 2] = rgbData[y * width * 3 + x * 3];     // 红色通道
            imageSurfaceData[offset + 1] = rgbData[y * width * 3 + x * 3 + 1]; // 绿色通道
            imageSurfaceData[offset + 0] = rgbData[y * width * 3 + x * 3 + 2]; // 蓝色通道
        }
    }

    setImageBitmap(img);
    mVideoInfo->freeData(vd);

    return 1;
#endif
    return 0;
}
