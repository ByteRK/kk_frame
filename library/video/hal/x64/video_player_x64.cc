/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-09-30 17:51:30
 * @FilePath: /kk_frame/library/video/hal/x64/video_player_x64.cc
 * @Description: x64 视频后端（FFmpeg 软解 + 帧输出，供开发机预览）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_backend.h"
#include "video_player_base.h"

#include <cdlog.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include <sys/prctl.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

namespace video {
    namespace {

#define X64_VIDEO_FRAME_SLOTS 4

        static double nowMs() {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        }

        /**
         * @brief x64 后端：FFmpeg 软解
         *
         * 解码结果以 VF_BGRA32 交给上层（内存布局与 cairo 的 RGB24 图面一致，可以整行直接拷贝），
         * 所以输出模式是 VM_FRAME，由组件层负责绘制与缩放。
         * 本后端只处理视频轨（无音频输出），播放节奏由解码帧的 pts 节流保证。
         */
        class VideoPlayerFfmpeg : public VideoPlayerBase {
        public:
            VideoPlayerFfmpeg();
            ~VideoPlayerFfmpeg() override;

            bool open(const std::string& url) override;
            bool play() override;
            bool pause() override;
            bool resume() override;
            bool stop() override;
            bool seek(double ms) override;
            bool setGeometry(const VideoGeometry& geo) override;
            bool setVolume(int volume) override;

            int    getStatus() const override;
            double getDuration() const override;
            double getPosition() const override;

            bool pullFrame(VideoFrame& frame) override;
            void releaseFrame(VideoFrame& frame) override;

        protected:
            bool restart() override;

        private:
            struct ReadyFrame {
                int    slot;
                double pts;
            };

            bool openMedia(const std::string& url);
            void closeMedia();
            void setStatus(int status);

            /// @brief 解码线程主体
            void decodeThread();
            /// @brief 处理挂起的跳转请求，返回 true 表示已处理
            bool handlePendingSeek();
            /// @brief 执行跳转（解码线程内调用）
            bool applySeek(double ms);
            /// @brief 申请空闲帧位，返回 -1 表示被暂停/跳转/退出打断
            int  waitFreeSlot();
            /// @brief 把解码帧缩放并写入帧位
            void writeSlot(int slot, const AVFrame* frame);
            /// @brief 取帧的显示时间戳（ms）
            double framePts(const AVFrame* frame) const;
            /// @brief 按 pts 节流（不持锁，避免阻塞上层取帧）
            void paceToPts(double pts);

        private:
            /* 媒体信息：只在 open/stop（调用线程）与解码线程之间靠 join 同步，无需加锁 */
            AVFormatContext* mFormatCtx{ nullptr };
            AVCodecContext*  mCodecCtx{ nullptr };
            SwsContext*      mSwsCtx{ nullptr };
            AVStream*        mVideoStream{ nullptr };
            const AVCodec*   mCodec{ nullptr };
            int              mStreamIndex{ -1 };
            int              mWidth{ 0 };
            int              mHeight{ 0 };
            int              mStride{ 0 };
            double           mDuration{ 0 };
            std::string      mUrl;

            /* 帧位与队列 */
            std::vector<std::vector<uint8_t>> mSlots;
            std::deque<int>                   mFreeSlots;
            std::deque<ReadyFrame>            mReady;
            std::mutex                        mQueueLock;
            std::condition_variable           mQueueCond;

            /* 线程与状态 */
            std::mutex          mApiLock;
            std::thread         mThread;
            std::atomic<int>    mStatus{ VS_NULL };
            std::atomic<bool>   mExit{ false };
            std::atomic<bool>   mPaused{ false };
            std::atomic<bool>   mAtEof{ false };
            std::atomic<double> mSeekRequest{ -1 };
            std::atomic<double> mPlayBase{ 0 };  ///< 播放基准：wallclock - position
            std::atomic<double> mPosition{ 0 };
        };

        VideoPlayerFfmpeg::VideoPlayerFfmpeg() { }

        VideoPlayerFfmpeg::~VideoPlayerFfmpeg() {
            stop();
        }

        /* ---------------------------------- 媒体打开/释放 ---------------------------------- */

        bool VideoPlayerFfmpeg::openMedia(const std::string& url) {
            if (avformat_open_input(&mFormatCtx, url.c_str(), nullptr, nullptr) < 0) {
                LOGE("ffmpeg open input failed: %s", url.c_str());
                return false;
            }
            if (avformat_find_stream_info(mFormatCtx, nullptr) < 0) {
                LOGE("ffmpeg find stream info failed: %s", url.c_str());
                closeMedia();
                return false;
            }

            mStreamIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mCodec, 0);
            if (mStreamIndex < 0) {
                LOGE("ffmpeg no video stream: %s", url.c_str());
                closeMedia();
                return false;
            }

            mVideoStream = mFormatCtx->streams[mStreamIndex];
            mCodecCtx = avcodec_alloc_context3(mCodec);
            if (mCodecCtx == nullptr
                || avcodec_parameters_to_context(mCodecCtx, mVideoStream->codecpar) < 0
                || avcodec_open2(mCodecCtx, mCodec, nullptr) < 0) {
                LOGE("ffmpeg open codec failed: %s", url.c_str());
                closeMedia();
                return false;
            }

            mWidth = mCodecCtx->width;
            mHeight = mCodecCtx->height;
            /* 16 字节对齐，方便上层整行拷贝 */
            mStride = (mWidth * 4 + 15) & ~15;
            mSwsCtx = sws_getContext(mWidth, mHeight, mCodecCtx->pix_fmt, mWidth, mHeight, AV_PIX_FMT_BGRA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (mSwsCtx == nullptr) {
                LOGE("ffmpeg create sws context failed: %s", url.c_str());
                closeMedia();
                return false;
            }

            mDuration = (mFormatCtx->duration > 0)
                ? static_cast<double>(mFormatCtx->duration) * 1000.0 / AV_TIME_BASE
                : 0;

            mSlots.assign(X64_VIDEO_FRAME_SLOTS, std::vector<uint8_t>());
            for (size_t i = 0; i < mSlots.size(); ++i)
                mSlots[i].resize(static_cast<size_t>(mStride) * mHeight);

            {
                std::lock_guard<std::mutex> lock(mQueueLock);
                mFreeSlots.clear();
                mReady.clear();
                for (int i = 0; i < X64_VIDEO_FRAME_SLOTS; ++i) mFreeSlots.push_back(i);
            }

            mAtEof = false;
            mPaused = false;
            mPosition = 0;
            mPlayBase = nowMs();

            LOGI("ffmpeg opened: %s %dx%d stride=%d duration=%.0fms", url.c_str(), mWidth, mHeight, mStride, mDuration);
            return true;
        }

        void VideoPlayerFfmpeg::closeMedia() {
            if (mSwsCtx) {
                sws_freeContext(mSwsCtx);
                mSwsCtx = nullptr;
            }
            if (mCodecCtx) avcodec_free_context(&mCodecCtx);
            if (mFormatCtx) avformat_close_input(&mFormatCtx);

            mVideoStream = nullptr;
            mCodec = nullptr;
            mStreamIndex = -1;
            mWidth = mHeight = mStride = 0;
            mDuration = 0;
        }

        /* ---------------------------------- 播放控制 ---------------------------------- */

        bool VideoPlayerFfmpeg::open(const std::string& url) {
            stop();
            if (url.empty()) {
                LOGE("video open failed: empty url");
                return false;
            }
            if (!openMedia(url)) return false;

            mUrl = url;
            mStatus = VS_INIT;
            return true;
        }

        bool VideoPlayerFfmpeg::play() {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mFormatCtx == nullptr) return false;

            if (mAtEof) mSeekRequest = 0;   /* 结束后再次起播 -> 从头开始 */
            mPaused = false;
            mPlayBase = nowMs() - mPosition.load();

            if (!mThread.joinable()) {
                mExit = false;
                mThread = std::thread(&VideoPlayerFfmpeg::decodeThread, this);
            }
            mQueueCond.notify_all();
            setStatus(VS_PLAY);
            return true;
        }

        bool VideoPlayerFfmpeg::pause() {
            if (mStatus.load() != VS_PLAY) return false;
            mPaused = true;
            setStatus(VS_PAUSE);
            return true;
        }

        bool VideoPlayerFfmpeg::resume() {
            if (mStatus.load() != VS_PAUSE) return false;
            mPlayBase = nowMs() - mPosition.load();
            mPaused = false;
            mQueueCond.notify_all();
            setStatus(VS_PLAY);
            return true;
        }

        bool VideoPlayerFfmpeg::stop() {
            mExit = true;
            mSeekRequest = -1;
            mQueueCond.notify_all();
            if (mThread.joinable()) mThread.join();

            closeMedia();
            {
                std::lock_guard<std::mutex> lock(mQueueLock);
                mFreeSlots.clear();
                mReady.clear();
                mSlots.clear();
            }

            mPaused = false;
            mAtEof = false;
            mPosition = 0;
            setStatus(VS_NULL);
            return true;
        }

        bool VideoPlayerFfmpeg::seek(double ms) {
            if (mFormatCtx == nullptr) return false;
            if (ms < 0) ms = 0;
            mSeekRequest = ms;
            mAtEof = false;
            mQueueCond.notify_all();
            return true;
        }

        bool VideoPlayerFfmpeg::setGeometry(const VideoGeometry& geo) {
            /* FRAME 模式：缩放与裁剪由组件层在绘制时完成，这里只记录 */
            (void)geo;
            return true;
        }

        bool VideoPlayerFfmpeg::setVolume(int volume) {
            /* 本后端只做视频软解，没有音频输出通道 */
            (void)volume;
            return false;
        }

        bool VideoPlayerFfmpeg::restart() {
            mPaused = false;
            mAtEof = false;
            mPlayBase = nowMs();
            mPosition = 0;
            mSeekRequest = 0;
            mStatus = VS_PLAY;
            mQueueCond.notify_all();
            return true;
        }

        int VideoPlayerFfmpeg::getStatus() const {
            return mStatus.load();
        }

        double VideoPlayerFfmpeg::getDuration() const {
            return mDuration;
        }

        double VideoPlayerFfmpeg::getPosition() const {
            return mPosition.load();
        }

        /* ---------------------------------- 帧交互 ---------------------------------- */

        bool VideoPlayerFfmpeg::pullFrame(VideoFrame& frame) {
            std::lock_guard<std::mutex> lock(mQueueLock);
            if (mReady.empty()) return false;

            const ReadyFrame ready = mReady.front();
            mReady.pop_front();

            const std::vector<uint8_t>& buffer = mSlots[ready.slot];
            frame.data = buffer.data();
            frame.width = mWidth;
            frame.height = mHeight;
            frame.stride = mStride;
            frame.format = VF_BGRA32;
            frame.pts = ready.pts;
            frame.priv = reinterpret_cast<void*>(static_cast<intptr_t>(ready.slot));

            mQueueCond.notify_all();
            return true;
        }

        void VideoPlayerFfmpeg::releaseFrame(VideoFrame& frame) {
            if (frame.priv == nullptr) return;

            const int slot = static_cast<int>(reinterpret_cast<intptr_t>(frame.priv));
            frame.data = nullptr;
            frame.priv = nullptr;
            if (slot < 0 || slot >= static_cast<int>(mSlots.size())) return;

            {
                std::lock_guard<std::mutex> lock(mQueueLock);
                mFreeSlots.push_back(slot);
            }
            mQueueCond.notify_all();
        }

        /* ---------------------------------- 解码线程 ---------------------------------- */

        void VideoPlayerFfmpeg::decodeThread() {
            prctl(PR_SET_NAME, "video_decode", 0, 0, 0);
            LOGI("ffmpeg decode thread start: %s", mUrl.c_str());

            AVPacket* packet = av_packet_alloc();
            AVFrame*  frame = av_frame_alloc();

            while (!mExit) {
                if (handlePendingSeek()) continue;

                if (mPaused || mAtEof) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                if (av_read_frame(mFormatCtx, packet) < 0) {
                    /* 读到结尾：循环则跳回开头，否则上报结束并原地等待 */
                    if (isLoop()) {
                        mSeekRequest = 0;
                    } else {
                        mAtEof = true;
                        mPosition = mDuration;
                        handlePlayerEnd(mDuration, mDuration);
                    }
                    continue;
                }
                if (packet->stream_index != mStreamIndex) {
                    av_packet_unref(packet);
                    continue;
                }

                const int sent = avcodec_send_packet(mCodecCtx, packet);
                av_packet_unref(packet);
                if (sent < 0) continue;

                int ret = 0;
                while (!mExit && (ret = avcodec_receive_frame(mCodecCtx, frame)) >= 0) {
                    const double pts = framePts(frame);
                    const int    slot = waitFreeSlot();
                    if (slot < 0) {
                        av_frame_unref(frame);
                        break;
                    }
                    writeSlot(slot, frame);
                    av_frame_unref(frame);

                    {
                        std::lock_guard<std::mutex> lock(mQueueLock);
                        mReady.push_back(ReadyFrame{ slot, pts });
                    }
                    postFrame();
                    paceToPts(pts);
                }
            }

            av_frame_free(&frame);
            av_packet_free(&packet);
            LOGI("ffmpeg decode thread exit: %s", mUrl.c_str());
        }

        bool VideoPlayerFfmpeg::handlePendingSeek() {
            const double target = mSeekRequest.exchange(-1);
            if (target < 0) return false;
            if (!applySeek(target)) LOGE("ffmpeg seek failed: %.0fms", target);
            return true;
        }

        int VideoPlayerFfmpeg::waitFreeSlot() {
            while (!mExit) {
                if (mPaused || mSeekRequest.load() >= 0) return -1;

                std::unique_lock<std::mutex> lock(mQueueLock);
                if (!mFreeSlots.empty()) {
                    const int slot = mFreeSlots.front();
                    mFreeSlots.pop_front();
                    return slot;
                }
                mQueueCond.wait_for(lock, std::chrono::milliseconds(10));
            }
            return -1;
        }

        void VideoPlayerFfmpeg::writeSlot(int slot, const AVFrame* frame) {
            uint8_t* dst[4] = { mSlots[slot].data(), nullptr, nullptr, nullptr };
            int      dstStride[4] = { mStride, 0, 0, 0 };
            sws_scale(mSwsCtx, frame->data, frame->linesize, 0, mHeight, dst, dstStride);
        }

        double VideoPlayerFfmpeg::framePts(const AVFrame* frame) const {
            if (mVideoStream == nullptr) return 0.0;
            int64_t pts = frame->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) pts = frame->pts;
            if (pts == AV_NOPTS_VALUE) return 0.0;
            return static_cast<double>(pts) * av_q2d(mVideoStream->time_base) * 1000.0;
        }

        bool VideoPlayerFfmpeg::applySeek(double ms) {
            if (mFormatCtx == nullptr || mCodecCtx == nullptr) return false;

            const int64_t target = static_cast<int64_t>(ms * AV_TIME_BASE / 1000.0);
            if (av_seek_frame(mFormatCtx, -1, target, AVSEEK_FLAG_BACKWARD) < 0) return false;
            avcodec_flush_buffers(mCodecCtx);

            {
                std::lock_guard<std::mutex> lock(mQueueLock);
                while (!mReady.empty()) {
                    mFreeSlots.push_back(mReady.front().slot);
                    mReady.pop_front();
                }
            }

            mAtEof = false;
            mPosition = ms;
            if (!mPaused) mPlayBase = nowMs() - ms;
            LOGI("ffmpeg seek to %.0fms", ms);
            return true;
        }

        void VideoPlayerFfmpeg::paceToPts(double pts) {
            /* 不持锁轮询等待，避免阻塞上层取帧；每 2~5ms 检查一次暂停/跳转/退出 */
            while (!mExit && !mPaused && mSeekRequest.load() < 0) {
                const double remain = mPlayBase.load() + pts - nowMs();
                if (remain <= 0) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(remain > 5 ? 5 : 2));
            }
            if (!mExit && !mPaused && mSeekRequest.load() < 0) mPosition = pts;
        }

        void VideoPlayerFfmpeg::setStatus(int status) {
            if (status == mStatus.load()) return;
            mStatus = status;
            postStatus(status, mDuration, mPosition.load());
        }

    } // namespace

    VideoMode backendMode() {
        return VM_FRAME;
    }

    const char* backendName() {
        return "ffmpeg";
    }

    VideoPlayer* backendCreate() {
        return new VideoPlayerFfmpeg();
    }

} // namespace video
