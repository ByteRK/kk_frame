/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 15:34:26
 * @FilePath: /kk_frame/library/video/hal/generic/video_player_generic.cc
 * @Description: 视频通用后端（走 cdroid 的 MP* 接口，作为无专用实现芯片的兜底）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_backend.h"
#include "video_player_base.h"

#include <cdlog.h>
#include <cdplayer.h>

#include <atomic>

namespace video {
    namespace {

        /**
         * @brief MP* 兜底后端
         *
         * 适用于 cdroid porting 层已实现 MP* 的芯片（sigma/d211/r818/ssd2351 等）。
         * MP* 把画面直接送到硬件图层，因此输出模式为 VM_OVERLAY，上层只需挖洞。
         *
         * 注意：
         *  - 各芯片 MPGetStatus() 的语义并不统一（sigma 返回厂商状态，d211/r818/ssd2351 直接返回 0），
         *    所以播放状态一律以 MPSetCallback 上报的 MPMESSAGE 为准；
         *  - 回调运行在播放线程，只允许入队与读缓存，不得回调 MP* 接口（HAL 明确要求）。
         */
        class VideoPlayerMp : public VideoPlayerBase {
        public:
            VideoPlayerMp();
            ~VideoPlayerMp() override;

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
            static void onPlayerMessage(HANDLE handle, MPMESSAGE msg, long param, void* userdata);
            /// @brief 播放线程回调入口（禁止调用 MP*）
            void        onMessage(int msg);
            /// @brief 仅使用缓存值切换状态并上报
            void        setStatus(int status);
            /// @brief 刷新时长/进度缓存（只能在调用线程执行）
            void        refreshCache();
            static int  toMpRotation(int rotation);

        private:
            HANDLE        mHandle{ nullptr };
            std::atomic<int> mStatus{ VS_NULL };          ///< 播放线程会写，用原子量避免加锁
            mutable std::atomic<double> mDuration{ 0 };   ///< 单位 ms
            mutable std::atomic<double> mPosition{ 0 };   ///< 单位 ms
            VideoGeometry mGeometry{ 0, 0, 0, 0, 0, 0, 0 };
        };

        VideoPlayerMp::VideoPlayerMp() { }

        VideoPlayerMp::~VideoPlayerMp() {
            stop();
        }

        int VideoPlayerMp::toMpRotation(int rotation) {
            switch (rotation) {
            case 90:  return 1;
            case 180: return 2;
            case 270: return 3;
            default:  return 0;
            }
        }

        bool VideoPlayerMp::open(const std::string& url) {
            stop();
            if (url.empty()) {
                LOGE("video open failed: empty url");
                return false;
            }

            mHandle = MPOpen(url.c_str());
            if (mHandle == nullptr) {
                LOGE("video open failed: %s", url.c_str());
                return false;
            }

            MPSetCallback(mHandle, onPlayerMessage, this);

            /* 几何先于起播下发（部分实现只在创建播放器时消费窗口参数） */
            MPSetWindow(mHandle, mGeometry.x, mGeometry.y, mGeometry.w, mGeometry.h);
            MPRotate(mHandle, toMpRotation(mGeometry.rotation));

            mStatus = VS_INIT;
            LOGI("video open: %s backend=%s", url.c_str(), backendName());
            return true;
        }

        bool VideoPlayerMp::play() {
            if (mHandle == nullptr) return false;
            MPPlay(mHandle);
            refreshCache();
            setStatus(VS_PLAY);
            return true;
        }

        bool VideoPlayerMp::pause() {
            if (mHandle == nullptr || mStatus != VS_PLAY) return false;
            MPPause(mHandle);
            setStatus(VS_PAUSE);
            return true;
        }

        bool VideoPlayerMp::resume() {
            if (mHandle == nullptr || mStatus != VS_PAUSE) return false;
            MPResume(mHandle);
            refreshCache();
            setStatus(VS_PLAY);
            return true;
        }

        bool VideoPlayerMp::stop() {
            if (mHandle == nullptr) {
                mStatus = VS_NULL;
                return false;
            }
            MPStop(mHandle);
            MPClose(mHandle);
            mHandle = nullptr;
            mDuration = 0.0;
            mPosition = 0.0;
            setStatus(VS_NULL);
            return true;
        }

        bool VideoPlayerMp::seek(double ms) {
            if (mHandle == nullptr) return false;
            mPosition = ms;
            return MPSeek(mHandle, ms) == 0;
        }

        bool VideoPlayerMp::setGeometry(const VideoGeometry& geo) {
            mGeometry = geo;
            if (mHandle == nullptr) return false;

            /* MP* 在窗口/旋转变化时会自行重建播放器并按原状态恢复播放 */
            MPSetWindow(mHandle, geo.x, geo.y, geo.w, geo.h);
            MPRotate(mHandle, toMpRotation(geo.rotation));
            return true;
        }

        bool VideoPlayerMp::setVolume(int volume) {
            if (volume < 0) volume = 0;
            if (volume > 100) volume = 100;
            if (mHandle == nullptr) return false;
            return MPSetVolume(mHandle, volume) == 0;
        }

        int VideoPlayerMp::getStatus() const {
            return mStatus.load();
        }

        void VideoPlayerMp::refreshCache() {
            if (mHandle == nullptr) return;
            double duration = 0;
            double position = 0;
            MPGetDuration(mHandle, &duration);
            MPGetPosition(mHandle, &position);
            mDuration = duration;
            mPosition = position;
        }

        double VideoPlayerMp::getDuration() const {
            const_cast<VideoPlayerMp*>(this)->refreshCache();
            return mDuration.load();
        }

        double VideoPlayerMp::getPosition() const {
            const_cast<VideoPlayerMp*>(this)->refreshCache();
            return mPosition.load();
        }

        bool VideoPlayerMp::pullFrame(VideoFrame& frame) {
            /* OVERLAY 模式由硬件图层直接输出，没有可供上层绘制的帧 */
            (void)frame;
            return false;
        }

        void VideoPlayerMp::releaseFrame(VideoFrame& frame) {
            (void)frame;
        }

        bool VideoPlayerMp::restart() {
            if (mHandle == nullptr) return false;
            MPSeek(mHandle, 0);
            MPPlay(mHandle);
            mPosition = 0.0;
            mStatus = VS_PLAY;
            return true;
        }

        void VideoPlayerMp::setStatus(int status) {
            if (status == mStatus.load()) return;
            mStatus = status;
            postStatus(status, mDuration.load(), mPosition.load());
        }

        void VideoPlayerMp::onPlayerMessage(HANDLE handle, MPMESSAGE msg, long param, void* userdata) {
            (void)handle;
            (void)param;
            VideoPlayerMp* self = static_cast<VideoPlayerMp*>(userdata);
            if (self) self->onMessage(static_cast<int>(msg));
        }

        void VideoPlayerMp::onMessage(int msg) {
            switch (msg) {
            case MP_PREPARED:
                setStatus(VS_INIT);
                break;
            case MP_STARTED:
                setStatus(VS_PLAY);
                break;
            case MP_PAUSED:
                setStatus(VS_PAUSE);
                break;
            case MP_END:
                /* 循环由基类统一处理 */
                handlePlayerEnd(mDuration.load(), mPosition.load());
                break;
            case MP_ERROR:
                LOGE("video play error, handle=%p", mHandle);
                setStatus(VS_ERROR);
                break;
            default:
                /* MP_BUFFERING / MP_SEEKED 等不改变播放状态 */
                break;
            }
        }

    } // namespace

    VideoMode backendMode() {
        return VM_OVERLAY;
    }

    const char* backendName() {
        return "mp";
    }

    VideoPlayer* backendCreate() {
        return new VideoPlayerMp();
    }

} // namespace video
