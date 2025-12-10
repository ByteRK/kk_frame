/*
 * @Author: xlc
 * @Email:
 * @Date: 2025-12-05 19:33:30
 * @LastEditTime: 2025-12-10 14:13:43
 * @FilePath: /kk_frame/src/viewlibs/video_view_x64.cc
 * @Description: 视频播放组件 X64版本
 * @note:  需先使用vcpkg安装ffmpeg
 *         vcpkg install ffmpeg:x64-linux-dynamic
 *
 * Copyright (c) 2025 by xlc, All Rights Reserved.
 *
**/

#if defined(CDROID_X64) || defined(__VSCODE__)

#include "video_view.h"
#include "comm_func.h"
#include <sys/prctl.h>
#include <unistd.h>

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

#define Q_D(Class) Class##Private *d = mPri.get();

// 视频信息
class VideoInfo {
    friend class VideoView;

public:
    static constexpr int __cache_frame_count = 10;
    enum {
        AV_DECODE_RUN = 1,
        AV_DECODE_PAUSE,
        AV_DECODE_EXIT,
    };
    struct AVRgbData {
        int      frameNum;
        uint8_t* rgbData;
    };
    int                       mAvDecode;
    bool                      mDecodeVideo;
    AVFormatContext* pFormatCtx;
    AVStream* videoStream;
    AVCodecParameters* pCodecParams;
    AVCodecContext* pCodecCtx;
    const AVCodec* pCodec;
    int                       videoStreamIdx;
    struct SwsContext* pSwsCtx;
    int                       width;         // 宽 */
    int                       height;        // 高 */
    int                       duration;      // 时长 */
    int                       frameCount;    // 最大帧数 */
    int                       readCount;     // 读取帧数 */
    int                       mPlayFrameNum; // 当前需要播放的帧数 */
    double                    fps;           // 帧数 */
    shared_queue<AVRgbData*>  mVideoFrames;  // 缓存帧 */
    int                       mSeekNum;
    int                       mBufIndex;
    uint8_t* mBuffer;

public:
    VideoInfo() {
        pFormatCtx = 0;
        pCodecParams = 0;
        pCodecCtx = 0;
        pCodec = 0;
        videoStreamIdx = -1;
        pSwsCtx = 0;
        width = 0;
        height = 0;
        duration = 0;
        frameCount = 0;
        readCount = 0;
        fps = 0;
        mAvDecode = AV_DECODE_EXIT;
        mDecodeVideo = false;
        mBufIndex = 0;
        mBuffer = 0;
        mSeekNum = 0;
    }

    ~VideoInfo() {
        mAvDecode = AV_DECODE_EXIT;
        while (mDecodeVideo) usleep(1000);
        if (pFormatCtx) {
            avcodec_free_context(&pCodecCtx);
            avformat_close_input(&pFormatCtx);
            sws_freeContext(pSwsCtx);
        }
        AVRgbData* vd;
        while (mVideoFrames.try_and_pop(vd)) { free(vd); }
        if (mBuffer) free(mBuffer);
    }

protected:
    void readFrame() {
        AVPacket packet;
        AVFrame* pFrame = av_frame_alloc();
        AVFrame* rgbFrame = av_frame_alloc();

        // 设置 RGB 输出缓冲区
        int      numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, width, height, 1);

        // Read packets from the video
        LOGI("begin av_read_frame...");
        while (mAvDecode != AV_DECODE_EXIT && av_read_frame(pFormatCtx, &packet) >= 0) {

            if (packet.stream_index == videoStreamIdx) {

                int response = avcodec_send_packet(pCodecCtx, &packet);
                while (response >= 0) {

                    response = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
                        break;
                    else if (response < 0) {
                        LOGE("Error while receiving a frame. response=%d", response);
                        break;
                    }

                    while (mAvDecode != AV_DECODE_RUN || mVideoFrames.size() >= __min(3, __cache_frame_count)) {
                        if (mAvDecode == AV_DECODE_EXIT) break;
                        usleep(1000);
                    }
                    if (mAvDecode == AV_DECODE_EXIT) break;

                    if (mAvDecode == AV_DECODE_RUN) {
                        readCount++;

                        // LOGV("%04d/%04d %04d", readCount, frameCount, mPlayFrameNum);

                        if (readCount >= mPlayFrameNum) {
                            LOGV("push at %d/%d", readCount, frameCount);

                            unsigned char* rgb_data =
                                mBuffer + ((mBufIndex++) % __cache_frame_count) * (height * width * 4);
                            sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0, height, &rgb_data,
                                rgbFrame->linesize);

                            AVRgbData* frameData = (AVRgbData*)malloc(sizeof(AVRgbData));
                            frameData->frameNum = readCount;
                            frameData->rgbData = rgb_data;
                            mVideoFrames.push(frameData);
                        }
                    }
                }
            }

            av_packet_unref(&packet);

            if (mSeekNum > 0) {
                AVRational time_base = videoStream->time_base;
                double     target_time_in_seconds = mSeekNum / fps;

                int64_t target_time_in_pts =
                    av_rescale_q((int64_t)(target_time_in_seconds * AV_TIME_BASE), AV_TIME_BASE_Q, time_base);

                int ret = av_seek_frame(pFormatCtx, videoStreamIdx, target_time_in_pts, AVSEEK_FLAG_FRAME);
                if (ret < 0) {
                    LOGE("Error seeking to timestamp %" PRId64, target_time_in_pts);
                } else {
                    LOGD("Seeking %d to timestamp %" PRId64 " ret=%d", mSeekNum, target_time_in_pts, ret);
                    avcodec_flush_buffers(pCodecCtx);
                    readCount = mSeekNum;
                    AVRgbData* vd;
                    while (mVideoFrames.try_and_pop(vd)) { free(vd); }
                }
                mSeekNum = 0;
            }
        }
        LOGI("end av_read_frame.");

        av_freep(&buffer);
        av_frame_free(&rgbFrame);
        av_frame_free(&pFrame);
    }

    static void* __read_video(void* lpvoid) {
        LOGI("begin __read_video.");
        VideoInfo* pthis = (VideoInfo*)lpvoid;
        pthis->mAvDecode = AV_DECODE_RUN;
        pthis->mDecodeVideo = true;
        pthis->readFrame();
        pthis->mDecodeVideo = false;
        pthis->mAvDecode = AV_DECODE_EXIT;
        LOGI("end __read_video...");
        return 0;
    }

public:
    bool isEnd() { return readCount >= frameCount || mAvDecode == AV_DECODE_EXIT; }

    AVRgbData* getData() {
        if (mVideoFrames.empty()) return 0;
        AVRgbData* dat;
        if (!mVideoFrames.try_and_pop(dat)) return 0;
        return dat;
    }

    void freeData(AVRgbData* dat) { free(dat); }

    int setFile(const char* filename) {
        if (pFormatCtx) return -1;
        unsigned int i;
        int          video_index;
        int64_t      baseDuration;
        AVRational   timeBase;

        int ret = avformat_open_input(&pFormatCtx, filename, nullptr, nullptr);
        if (ret < 0) {
            printf("avformat_open_input result fail. ret=%d file=%s\n", ret, filename);
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
        videoStream = pFormatCtx->streams[video_index];
        pCodecParams = videoStream->codecpar;
        pCodecCtx = avcodec_alloc_context3(NULL);
        if (avcodec_parameters_to_context(pCodecCtx, pCodecParams) < 0) { printf("parameters to context fail\n"); }
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == NULL) { printf("not find decoder\n"); }
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) { printf("avcodec open fail\n"); }

        pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
            pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

        if (pSwsCtx == NULL) { printf("sws get context fail\n"); }
        // 视频像素 */
        width = videoStream->codecpar->width;
        height = videoStream->codecpar->height;
        mBuffer = (uint8_t*)malloc(__cache_frame_count * (height * width * 4));
        // 获取视频时长 */
        baseDuration = videoStream->duration;                      // 基于时间基数的时长 */
        timeBase = videoStream->time_base;                     // 时间基数 */
        duration = baseDuration * timeBase.num / timeBase.den; // s
        // 帧数 帧率 */
        // 获取视频帧数 */
        frameCount = videoStream->nb_frames;
        fps = av_q2d(videoStream->r_frame_rate);
        mPlayFrameNum = 0;

        LOGI("count=%d fps=%.2f duration=%ds size=%dx%d buf=%p", frameCount, fps, duration, width, height, mBuffer);

    error_end:
        if (ret < 0) { return -1; }
        return 0;
    }

    void decodeVideo() {
        pthread_t tid;
        pthread_create(&tid, NULL, __read_video, this);
    }
};

///////////////////////////////////////////////////////////
class VideoViewPrivate {
    friend class VideoView;

protected:
    enum {
        msg_first_frame = 1,
        msg_next_frame,
        msg_end_frame,
    };
    VideoInfo* mVideoInfo;
    bool       mTaskRun;
    bool       mTaskExit;
    int        mError;
    int64_t     mPlayTime;
    int        mPlayFrameNum;
    int64_t     mFirstTime;

public:
    VideoViewPrivate() {
        mVideoInfo = nullptr;
        reset();
    }
    void reset() {
        mTaskRun = false;
        mTaskExit = true;
        mError = 0;
        mPlayTime = 0;
        mPlayFrameNum = 0;
        mFirstTime = 0;
    }
    ~VideoViewPrivate() { __del(mVideoInfo); }
    int getTimeFrameNum() {
        int timems = SystemClock::uptimeMillis() - mPlayTime;
        int frameNum = timems * mVideoInfo->fps / 1000;
        return mPlayFrameNum + frameNum;
    }
};

///////////////////////////////////////////////////////////
DECLARE_WIDGET(VideoView)
VideoView::VideoView(Context* ctx, const AttributeSet& attrs) : ImageView(ctx, attrs) {
    initViewData();
    setUrl(attrs.getString("url"));
    setLoop(!attrs.getBoolean("oneShot", true));
    setPointsFile(attrs.getString("pointsFile"));
    if (attrs.getBoolean("loadPlay")) postDelayed([this]() { start(); });
}

VideoView::VideoView(int w, int h) : ImageView(w, h) {
    initViewData();
}

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

VideoView::~VideoView() {
    Q_D(VideoView);
    stop();
    d->mTaskRun = false;
    while (!d->mTaskExit) usleep(1000);
    Looper::getMainLooper()->removeEventHandler(this);
}

void VideoView::setUrl(const std::string& url) {
    mUrl = url;
}

void VideoView::setLoop(bool flag) {
    mLoopPlayBack = flag;
}

void VideoView::setVolume(int value) {
    mVolume = value;
}

void VideoView::setPointsFile(const std::string& fpath) {
    if (fpath.empty()) return;
    if (access(fpath.c_str(), F_OK)) {
        LOGE("point file not exists. fpath=%s", fpath.c_str());
        return;
    }

    std::string pointConn;
    char        buffer[1024];
    FILE* fp = fopen(fpath.c_str(), "r");
    int         rlen = fread(buffer, 1, sizeof(buffer), fp);
    while (rlen > 0) {
        pointConn.append(buffer, rlen);
        fread(buffer, 1, sizeof(buffer), fp);
    }
    fclose(fp);

    Point pt;
    mPoints.clear();
    for (const char* p = pointConn.c_str(); *p; p++) {
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

void VideoView::setOnPlayStatusChange(OnPlayStatusChange l) {
    mChangeCallback = l;
}

void VideoView::start() {
    Q_D(VideoView);
    LOGI("status: %d", mStatus);
    mStatus = VS_STARTING;

    d->mTaskRun = false;
    while (!d->mTaskExit) usleep(1000);
    __del(d->mVideoInfo);

    d->reset();
    d->mVideoInfo = new VideoInfo();
    d->mError = d->mVideoInfo->setFile(mUrl.c_str());
    d->mPlayTime = SystemClock::uptimeMillis();

    d->mTaskRun = true;
    d->mTaskExit = false;
    d->mVideoInfo->decodeVideo();

    pthread_t tid;
    pthread_create(&tid, NULL, threadReadFrame, this);

    postDelayed(mRunner, 1000);
}

bool VideoView::pause() {
    LOGI("status: %d", mStatus);
    if (mStatus != VS_PLAYING) return false;
    mStatus = VS_PAUSED;
    return true;
}

bool VideoView::play() {
    LOGI("status: %d", mStatus);
    if (mStatus == VS_PLAYING) return false;
    Q_D(VideoView);
    d->mPlayTime = SystemClock::uptimeMillis();
    mStatus = VS_PLAYING;
    return true;
}

void VideoView::stop() {
    LOGI("status: %d", mStatus);
    if (mStatus == VS_STOPPED) return;
    mStatus = VS_STOPPED;
    removeCallbacks(mRunner);
    Q_D(VideoView);
    d->mTaskRun = false;
}

bool VideoView::isPlay() {
    return mStatus > VS_STOPPED && mStatus < VS_COMPLETED;
}

void VideoView::onDraw(Canvas& canvas) {
    if (!mPoints.empty()) {
        canvas.begin_new_path();
        canvas.move_to(mPoints[0].x, mPoints[0].y);
        for (int i = 1; i < mPoints.size(); i++) { canvas.line_to(mPoints[i].x, mPoints[i].y); }
        canvas.close_path();
        canvas.clip();
    }
    ImageView::onDraw(canvas);
}

/// @brief 页面剥离回调
/// @note 防呆用
void VideoView::onDetachedFromWindow() {
    stop();
    ImageView::onDetachedFromWindow();
}

void VideoView::handlePlayerNotify(int msg) {
    Q_D(VideoView);
    switch (msg) {
    case VideoViewPrivate::msg_first_frame: {
        LOGI("thread post use time %ld", SystemClock::uptimeMillis() - d->mPlayTime);
        mDuration = d->mVideoInfo->duration;
        play();
        if (mChangeCallback) mChangeCallback(*this, mDuration, 0, mStatus);
    }
    case VideoViewPrivate::msg_next_frame: {
        if (mStatus != VS_PLAYING) return;

        do {
            VideoInfo::AVRgbData* vd = d->mVideoInfo->getData();
            if (!vd) break;

            uint8_t* rgbData = vd->rgbData;
            int      width = d->mVideoInfo->width;
            int      height = d->mVideoInfo->height;

            Cairo::RefPtr<Cairo::ImageSurface> img =
                Cairo::ImageSurface::create(Cairo::Surface::Format::RGB24, width, height);
            unsigned char* imageSurfaceData = img->get_data();
            int            stride = img->get_stride();

            // LOGV("play FrameNum=%04d size(%dx%d) stride=%d", vd->frameNum, width, height, stride);

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int offset = y * stride + x * 4;
                    int offsetY = y * width * 3;
                    int offsetX = x * 3;
                    imageSurfaceData[offset + 2] = rgbData[offsetY + offsetX];
                    imageSurfaceData[offset + 1] = rgbData[offsetY + offsetX + 1];
                    imageSurfaceData[offset + 0] = rgbData[offsetY + offsetX + 2];
                }
            }
            setImageBitmap(img);

            d->mPlayFrameNum = vd->frameNum;
            d->mVideoInfo->freeData(vd);
        } while (false);

    } break;
    case VideoViewPrivate::msg_end_frame: {
        LOGD("loop=%d", mLoopPlayBack);
        if (mLoopPlayBack) {
            start();
        } else {
            stop();
            if (mChangeCallback) mChangeCallback(*this, mDuration, mDuration, mStatus);
        }
    } break;
    default: break;
    }
}

void VideoView::onTick() {
    if (mStatus == VS_PLAYING) {
        if (mChangeCallback) {
            Q_D(VideoView);
            int position = d->mPlayFrameNum / d->mVideoInfo->fps;
            mChangeCallback(*this, d->mVideoInfo->duration, position, mStatus);
        }
    }
    postDelayed(mRunner, 1000);
}

void* VideoView::threadReadFrame(void* param) {
    prctl(PR_SET_NAME, "rgb_video");
    VideoView* pthis = static_cast<VideoView*>(param);
    pthis->onTask();
    return 0;
}

void VideoView::onTask() {
    Q_D(VideoView);
    int    play_frame = 0;
    int64_t last_time, nowms, framems;
    last_time = 0;
    framems = 1000 / d->mVideoInfo->fps;
    LOGD("task begin...");
    while (d->mTaskRun) {
        nowms = SystemClock::uptimeMillis();
        if (d->mVideoInfo->mVideoFrames.empty()) {
            usleep(1000);
            continue;
        }
        if (play_frame == 0) {
            d->mFirstTime = nowms;
            pushMessage(VideoViewPrivate::msg_first_frame);
            play_frame++;
            last_time = nowms;
        } else if (mStatus == VS_PLAYING) {
            if (last_time == 0) last_time = d->mPlayTime;
            if (nowms - last_time >= framems - 1) {
                last_time = nowms;
                pushMessage(VideoViewPrivate::msg_next_frame);
                play_frame++;
            }
        } else {
            last_time = 0;
        }
        if (d->mVideoInfo->isEnd()) {
            pushMessage(VideoViewPrivate::msg_end_frame);
            break;
        }
        usleep(1000);
    }
    LOGD("task end...");
    d->mVideoInfo->mAvDecode = VideoInfo::AV_DECODE_EXIT;
    d->mTaskRun = false;
    d->mTaskExit = true;
}

int VideoView::getStatus() {
    return mStatus;
}

void VideoView::seekTime(int sec) {
    Q_D(VideoView);
    if (d->mVideoInfo->mSeekNum > 0) return;
    d->mPlayTime = SystemClock::uptimeMillis() - sec * 1000;
    d->mVideoInfo->mSeekNum = d->mVideoInfo->fps * sec;
    if (d->mVideoInfo->mSeekNum > d->mVideoInfo->frameCount) { d->mVideoInfo->mSeekNum = d->mVideoInfo->frameCount; }
}

void VideoView::seekPercent(double per) {
    Q_D(VideoView);
    if (d->mVideoInfo->mSeekNum > 0) return;
    double lastper = d->mVideoInfo->readCount * 100.0 / d->mVideoInfo->frameCount;
    d->mPlayTime -= (per - lastper) * d->mVideoInfo->frameCount / d->mVideoInfo->fps;
    d->mVideoInfo->mSeekNum = d->mVideoInfo->frameCount * per / 100;
}

void VideoView::pushMessage(int msg) {
    mMsgQueue.push(msg);
}

int VideoView::checkEvents() {
    if (mMsgQueue.empty()) return 0;
    return 1;
}

int VideoView::handleEvents() {
    int msg;
    if (!mMsgQueue.try_and_pop(msg)) return 0;
    handlePlayerNotify(msg);
    return 1;
}

#endif

