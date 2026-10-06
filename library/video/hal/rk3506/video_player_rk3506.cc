/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 15:24:28
 * @FilePath: /kk_frame/library/video/hal/rk3506/video_player_rk3506.cc
 * @Description: RK3506 视频后端（直连 Rockchip RKADK player，硬件 VO 图层输出）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "video_backend.h"
#include "video_player_base.h"

#include <cdlog.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

#include <sys/prctl.h>

#include <rkadk/rkadk_media_comm.h>
#include <rkadk/rkadk_player.h>

namespace video {
    namespace {

#define RK_VIDEO_DEFAULT_SOUNDCARD "hw:0,0"
#define RK_VIDEO_DEFAULT_FRAMERATE 30
#define RK_VIDEO_FRAME_BUF_CNT     4

        static int envInt(const char* name, int defValue) {
            const char* value = getenv(name);
            return (value && *value) ? atoi(value) : defValue;
        }

        class VideoPlayerRkadk;

        /// @brief player 句柄 -> 实例，用于把 rkadk 事件回调分派回对应实例
        static std::mutex                                sRegLock;
        static std::map<RKADK_MW_PTR, VideoPlayerRkadk*> sRegistry;

        /**
         * @brief RK3506 后端
         *
         * rkadk player 自己持有 VO 图层并把解码结果直接送给显示，所以：
         *  - 输出模式是 VM_OVERLAY，上层只负责挖洞；
         *  - 窗口与旋转只在 RKADK_PLAYER_Create 时被消费，几何变化必须重建播放器；
         *  - Create/Prepare 比较耗时，放在子线程里做，避免卡住 UI。
         */
        class VideoPlayerRkadk : public VideoPlayerBase {
        public:
            VideoPlayerRkadk();
            ~VideoPlayerRkadk() override;

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
            static void onPlayerEvent(RKADK_MW_PTR player, RKADK_PLAYER_EVENT_E event, RKADK_VOID* data);
            /// @brief rkadk 事件入口（播放线程，禁止调用 rkadk 接口）
            void        onEvent(int event);
            void        setStatus(int status);

            /// @brief 用当前几何/音量填充播放器配置（调用前须持有 mLock）
            void fillCfgLocked();
            /// @brief 子线程：Create -> SetDataSource -> Prepare -> Play
            void prepareThread();
            /// @brief 销毁播放器（调用方保证子线程已结束）
            void destroyPlayer();
            /// @brief 把 UI 坐标换算成 VO 窗口（含旋转转置）
            void makeVoWindow(RKADK_U32& rotation, int& x, int& y, int& w, int& h) const;

        private:
            mutable std::mutex mApiLock;                  ///< 保护 player/几何/音量
            RKADK_MW_PTR       mPlayer{ nullptr };
            RKADK_PLAYER_CFG_S mCfg;                      ///< 必须常驻：pSoundCard 需保持有效
            std::string        mSoundCard{ RK_VIDEO_DEFAULT_SOUNDCARD };
            std::string        mUrl;
            std::atomic<int>    mStatus{ VS_NULL };        ///< 事件线程会写，用原子量避免加锁
            mutable std::atomic<double> mDuration{ 0 };
            int                mVolume{ 70 };
            VideoGeometry      mGeometry{ 0, 0, 0, 0, 0, 0, 0 };
            std::thread        mWorker;
            std::atomic<bool>  mPreparing{ false };
            std::atomic<bool>  mAbort{ false };
        };

        VideoPlayerRkadk::VideoPlayerRkadk() {
            memset(&mCfg, 0, sizeof(mCfg));
            const char* soundCard = getenv("MP_SOUND_CARD");
            if (soundCard && *soundCard) mSoundCard = soundCard;
            mVolume = envInt("MP_VOLUME", 70);
        }

        VideoPlayerRkadk::~VideoPlayerRkadk() {
            stop();
        }

        /* ---------------------------------- 事件注册表 ---------------------------------- */

        static void registerInstance(RKADK_MW_PTR player, VideoPlayerRkadk* instance) {
            std::lock_guard<std::mutex> lock(sRegLock);
            sRegistry[player] = instance;
        }

        static void unregisterInstance(RKADK_MW_PTR player) {
            std::lock_guard<std::mutex> lock(sRegLock);
            sRegistry.erase(player);
        }

        void VideoPlayerRkadk::onPlayerEvent(RKADK_MW_PTR player, RKADK_PLAYER_EVENT_E event, RKADK_VOID* data) {
            (void)data;
            VideoPlayerRkadk* self = nullptr;
            {
                std::lock_guard<std::mutex> lock(sRegLock);
                std::map<RKADK_MW_PTR, VideoPlayerRkadk*>::iterator it = sRegistry.find(player);
                if (it != sRegistry.end()) self = it->second;
            }
            if (self) self->onEvent(static_cast<int>(event));
        }

        void VideoPlayerRkadk::onEvent(int event) {
            /* 本函数运行在 rkadk 事件线程：只允许入队 */
            switch (event) {
            case RKADK_PLAYER_EVENT_PREPARED:
            case RKADK_PLAYER_EVENT_SOF:
                setStatus(VS_INIT);
                break;
            case RKADK_PLAYER_EVENT_PLAY:
                setStatus(VS_PLAY);
                break;
            case RKADK_PLAYER_EVENT_PAUSED:
                setStatus(VS_PAUSE);
                break;
            case RKADK_PLAYER_EVENT_EOF:
                /* 循环由基类统一处理 */
                handlePlayerEnd(mDuration.load(), 0);
                break;
            case RKADK_PLAYER_EVENT_ERROR:
                LOGE("rkadk play error");
                setStatus(VS_ERROR);
                break;
            default:
                /* STATE_CHANGED / STOPPED / SEEK_END 不改变播放状态 */
                break;
            }
        }

        void VideoPlayerRkadk::setStatus(int status) {
            /* 本函数可能在 rkadk 事件线程执行，因此不取 mApiLock，只用原子量 */
            if (status == mStatus.load()) return;
            mStatus = status;
            postStatus(status, mDuration.load(), 0);
        }

        /* ---------------------------------- 播放控制 ---------------------------------- */

        bool VideoPlayerRkadk::open(const std::string& url) {
            stop();
            if (url.empty()) {
                LOGE("video open failed: empty url");
                return false;
            }

            std::lock_guard<std::mutex> lock(mApiLock);
            mUrl = url;
            mDuration = 0;
            mStatus = VS_INIT;
            LOGI("video open: %s backend=%s", url.c_str(), backendName());
            return true;
        }

        bool VideoPlayerRkadk::play() {
            {
                std::lock_guard<std::mutex> lock(mApiLock);
                if (mUrl.empty()) return false;
                if (mPlayer) {
                    if (mStatus == VS_PAUSE) {
                        /* 暂停中再次 play 视为继续 */
                        if (RKADK_PLAYER_Play(mPlayer) != RKADK_SUCCESS) return false;
                        setStatus(VS_PLAY);
                        return true;
                    }
                    return RKADK_PLAYER_Play(mPlayer) == RKADK_SUCCESS;
                }
                if (mPreparing) return true;
                mPreparing = true;
                mAbort = false;
            }

            /* 上一个子线程必须先结束，避免重复创建播放器 */
            if (mWorker.joinable()) mWorker.join();
            mWorker = std::thread(&VideoPlayerRkadk::prepareThread, this);
            return true;
        }

        void VideoPlayerRkadk::prepareThread() {
            prctl(PR_SET_NAME, "video_prepare", 0, 0, 0);
            LOGI("rkadk player creating: %s", mUrl.c_str());

            RKADK_MW_PTR       player = nullptr;
            RKADK_PLAYER_CFG_S cfg;
            {
                std::lock_guard<std::mutex> lock(mApiLock);
                fillCfgLocked();
                cfg = mCfg;
            }
            cfg.pfnPlayerCallback = onPlayerEvent;
            cfg.stAudioCfg.pSoundCard = mSoundCard.c_str();

            /* Create/Prepare 会阻塞，放到这里做，期间不持锁以免卡住 UI 线程 */
            if (RKADK_PLAYER_Create(&player, &cfg) != RKADK_SUCCESS) {
                LOGE("RKADK_PLAYER_Create failed: %s", mUrl.c_str());
                mPreparing = false;
                setStatus(VS_ERROR);
                return;
            }
            if (mAbort
                || RKADK_PLAYER_SetDataSource(player, mUrl.c_str()) != RKADK_SUCCESS
                || RKADK_PLAYER_Prepare(player) != RKADK_SUCCESS) {
                LOGE("rkadk player prepare failed: %s", mUrl.c_str());
                RKADK_PLAYER_Destroy(player);
                mPreparing = false;
                if (!mAbort) setStatus(VS_ERROR);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(mApiLock);
                if (mAbort) {
                    RKADK_PLAYER_Destroy(player);
                    mPreparing = false;
                    return;
                }
                mPlayer = player;
            }
            registerInstance(player, this);

            if (RKADK_PLAYER_Play(player) != RKADK_SUCCESS) {
                LOGE("RKADK_PLAYER_Play failed: %s", mUrl.c_str());
                destroyPlayer();
                mPreparing = false;
                setStatus(VS_ERROR);
                return;
            }

            mPreparing = false;
            setStatus(VS_PLAY);
            LOGI("rkadk player started: %s", mUrl.c_str());
        }

        bool VideoPlayerRkadk::pause() {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr || mStatus != VS_PLAY) return false;
            if (RKADK_PLAYER_Pause(mPlayer) != RKADK_SUCCESS) return false;
            setStatus(VS_PAUSE);
            return true;
        }

        bool VideoPlayerRkadk::resume() {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr) return false;
            if (RKADK_PLAYER_Play(mPlayer) != RKADK_SUCCESS) return false;
            setStatus(VS_PLAY);
            return true;
        }

        bool VideoPlayerRkadk::stop() {
            /* 先让子线程退出（它会自行销毁半成品），再回收播放器 */
            mAbort = true;
            if (mWorker.joinable()) mWorker.join();

            destroyPlayer();

            std::lock_guard<std::mutex> lock(mApiLock);
            mDuration = 0;
            mStatus = VS_NULL;
            return true;
        }

        void VideoPlayerRkadk::destroyPlayer() {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr) return;
            unregisterInstance(mPlayer);
            RKADK_PLAYER_Stop(mPlayer);
            RKADK_PLAYER_Destroy(mPlayer);
            mPlayer = nullptr;
        }

        bool VideoPlayerRkadk::seek(double ms) {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr) return false;
            return RKADK_PLAYER_Seek(mPlayer, static_cast<RKADK_S64>(ms)) == RKADK_SUCCESS;
        }

        bool VideoPlayerRkadk::setGeometry(const VideoGeometry& geo) {
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(mApiLock);
                if (mGeometry.x == geo.x && mGeometry.y == geo.y && mGeometry.w == geo.w && mGeometry.h == geo.h
                    && mGeometry.rotation == geo.rotation && mGeometry.screenWidth == geo.screenWidth
                    && mGeometry.screenHeight == geo.screenHeight)
                    return true;
                mGeometry = geo;
                changed = (mPlayer != nullptr);
            }
            if (!changed) return true;

            /* rkadk 只在 Create 时消费窗口参数，几何变化必须重建播放器 */
            LOGI("video geometry changed, rebuild rkadk player");
            const bool playing = (getStatus() == VS_PLAY);
            destroyPlayer();          /* 注意：内部会 unregister，事件不会再回调到已销毁实例 */
            mAbort = false;
            if (playing) { play(); return true; }

            std::lock_guard<std::mutex> lock(mApiLock);
            mStatus = VS_INIT;
            return true;
        }

        bool VideoPlayerRkadk::setVolume(int volume) {
            if (volume < 0) volume = 0;
            if (volume > 100) volume = 100;

            std::lock_guard<std::mutex> lock(mApiLock);
            mVolume = volume;
            if (mPlayer == nullptr) return true;
            return RKADK_PLAYER_SetAoVolume(mPlayer, volume) == RKADK_SUCCESS;
        }

        bool VideoPlayerRkadk::restart() {
            {
                std::lock_guard<std::mutex> lock(mApiLock);
                if (mPlayer == nullptr) return false;
                if (RKADK_PLAYER_Seek(mPlayer, 0) != RKADK_SUCCESS) return false;
                if (RKADK_PLAYER_Play(mPlayer) != RKADK_SUCCESS) return false;
                mStatus = VS_PLAY;
            }
            return true;
        }

        int VideoPlayerRkadk::getStatus() const {
            return mStatus.load();
        }

        double VideoPlayerRkadk::getDuration() const {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr) return mDuration.load();

            RKADK_U32 duration = 0;
            if (RKADK_PLAYER_GetDuration(mPlayer, &duration) == RKADK_SUCCESS)
                mDuration = static_cast<double>(duration);   /* 单位 ms */
            return mDuration.load();
        }

        double VideoPlayerRkadk::getPosition() const {
            std::lock_guard<std::mutex> lock(mApiLock);
            if (mPlayer == nullptr) return 0;
            return static_cast<double>(RKADK_PLAYER_GetCurrentPosition(mPlayer)); /* 单位 ms */
        }

        bool VideoPlayerRkadk::pullFrame(VideoFrame& frame) {
            /* OVERLAY 模式由 VO 图层直接输出，没有可供上层绘制的帧 */
            (void)frame;
            return false;
        }

        void VideoPlayerRkadk::releaseFrame(VideoFrame& frame) {
            (void)frame;
        }

        /* ---------------------------------- 配置与几何 ---------------------------------- */

        void VideoPlayerRkadk::makeVoWindow(RKADK_U32& rotation, int& x, int& y, int& w, int& h) const {
            /* 面板方向与 UI 相反，视频图层需要按旋转角把窗口转置并重新定位 */
            rotation = 0;
            x = mGeometry.x;
            y = mGeometry.y;
            w = mGeometry.w;
            h = mGeometry.h;

            switch (mGeometry.rotation) {
            case 90: {
                rotation = 3;
                std::swap(w, h);
                const int t = x;
                x = y;
                y = mGeometry.screenHeight - t - h;
            } break;
            case 180:
                rotation = 2;
                x = mGeometry.screenWidth - x - w;
                y = mGeometry.screenHeight - y - h;
                break;
            case 270: {
                rotation = 1;
                std::swap(w, h);
                const int t = y;
                y = x;
                x = mGeometry.screenWidth - t - w;
            } break;
            default:
                break;
            }
        }

        void VideoPlayerRkadk::fillCfgLocked() {
            RKADK_PLAYER_FRAME_INFO_S* frm = &mCfg.stFrmInfo;
            int x = 0, y = 0, w = 0, h = 0;
            RKADK_U32 rotation = 0;

            memset(&mCfg, 0, sizeof(mCfg));
            makeVoWindow(rotation, x, y, w, h);
            if (w <= 0) w = mGeometry.screenWidth;
            if (h <= 0) h = mGeometry.screenHeight;

            frm->u32FrmInfoX = static_cast<RKADK_U32>(x);
            frm->u32FrmInfoY = static_cast<RKADK_U32>(y);
            frm->u32DispWidth = static_cast<RKADK_U32>(w);
            frm->u32DispHeight = static_cast<RKADK_U32>(h);
            frm->u32ImgWidth = static_cast<RKADK_U32>(w);
            frm->u32ImgHeight = static_cast<RKADK_U32>(h);
            frm->u32VoFormat = VO_FORMAT_RGB888;
            frm->u32VoLay = static_cast<RKADK_U32>(envInt("MP_VO_LAY", -1));
            frm->u32VoDev = static_cast<RKADK_U32>(envInt("MP_VO_DEV", -1));
            frm->u32VoChn = static_cast<RKADK_U32>(envInt("MP_VO_CHN", 1));
            frm->u32BorderColor = 0x0000FA;
            frm->bMirror = RKADK_FALSE;
            frm->bFlip = RKADK_FALSE;
            frm->u32Rotation = rotation;
            frm->u32EnIntfType = static_cast<RKADK_VO_INTF_TYPE_E>(envInt("MP_VO_INTF", DISPLAY_TYPE_MIPI));
            frm->enIntfSync = RKADK_VO_OUTPUT_DEFAULT;
            frm->enVoSpliceMode = SPLICE_MODE_RGA;
            frm->u32DispBufLen = 2;

            frm->stSyncInfo.bIdv = RKADK_TRUE;
            frm->stSyncInfo.bIhs = RKADK_TRUE;
            frm->stSyncInfo.bIvs = RKADK_TRUE;
            frm->stSyncInfo.bSynm = RKADK_TRUE;
            frm->stSyncInfo.bIop = RKADK_TRUE;
            frm->stSyncInfo.u16FrameRate = RK_VIDEO_DEFAULT_FRAMERATE;
            frm->stSyncInfo.u16PixClock = 65000;
            frm->stSyncInfo.u16Hact = 1200;
            frm->stSyncInfo.u16Hbb = 24;
            frm->stSyncInfo.u16Hfb = 240;
            frm->stSyncInfo.u16Hpw = 136;
            frm->stSyncInfo.u16Hmid = 0;
            frm->stSyncInfo.u16Vact = 1200;
            frm->stSyncInfo.u16Vbb = 200;
            frm->stSyncInfo.u16Vfb = 194;
            frm->stSyncInfo.u16Vpw = 6;

            mCfg.bEnableVideo = RKADK_TRUE;
            mCfg.bEnableAudio = RKADK_TRUE;
            mCfg.bEnableBlackBackground = RKADK_TRUE;
            mCfg.pfnPlayerCallback = onPlayerEvent;
            mCfg.stAudioCfg.pSoundCard = mSoundCard.c_str();
            mCfg.stAudioCfg.u32SpeakerVolume = static_cast<RKADK_U32>(mVolume);
            mCfg.stVdecCfg.u32FrameBufCnt = RK_VIDEO_FRAME_BUF_CNT;

            LOGI("vo window [%d,%d,%d,%d] rotation=%u intf=%d", x, y, w, h, rotation, frm->u32EnIntfType);
        }

    } // namespace

    VideoMode backendMode() {
        return VM_OVERLAY;
    }

    const char* backendName() {
        return "rkadk";
    }

    VideoPlayer* backendCreate() {
        return new VideoPlayerRkadk();
    }

} // namespace video
