/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-30 10:00:00
 * @LastEditTime: 2026-10-06 15:03:20
 * @FilePath: /kk_frame/library/video/include/video_player.h
 * @Description: 视频播放统一控制接口
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __VIDEO_PLAYER_H__
#define __VIDEO_PLAYER_H__

#include <cstdint>
#include <string>

/// @brief 视频播放命名空间
/// @note 组件层只依赖本头文件；具体实现由 CMakeLists.txt 按 CDROID_CHIPSET 进行选择
///       - rk3506→rkadk、x64→FFmpeg、其它芯片→MP* 兜底、关闭/未支持→占位
namespace video {

    /// @brief 播放状态枚举
    typedef enum {
        VS_NULL = 0,    // 未打开
        VS_INIT,        // 已打开/就绪
        VS_PLAY,        // 播放中
        VS_PAUSE,       // 已暂停
        VS_OVER,        // 播放结束
        VS_ERROR,       // 播放出错
    } VideoStatus;

    /// @brief 画面输出模式枚举
    typedef enum {
        VM_NONE = 0,    // 当前平台不支持
        VM_OVERLAY,     // 直接将画面输出到硬件图层，上层挖洞
        VM_FRAME,       // 输出 RGB 帧，上层负责绘制
    } VideoMode;

    /// @brief 帧像素格式枚举
    typedef enum {
        VF_RGB24 = 0,   // 每像素 3 字节，内存顺序 R,G,B
        VF_BGR24,       // 每像素 3 字节，内存顺序 B,G,R
        VF_BGRA32,      // 每像素 4 字节，内存顺序 B,G,R,A（与 cairo RGB24 图面布局一致，可整行直接拷贝）
    } VideoFormat;

    /// @brief 视频窗口信息
    typedef struct {
        int x, y, w, h;   // 控件在窗口中的位置与尺寸
        int rotation;     // 屏幕旋转角度：0/90/180/270
        int screenWidth;  // 屏幕宽（未旋转）
        int screenHeight; // 屏幕高（未旋转）
    } VideoGeometry;

    /// @brief 单帧图像，生命周期由 releaseFrame 结束
    typedef struct {
        const uint8_t* data;    // 像素首地址
        int            width;   // 宽
        int            height;  // 高
        int            stride;  // 行字节数
        int            format;  // VideoFormat
        double         pts;     // 时间戳(ms)
        void*          priv;    // 内部句柄，交还 releaseFrame
    } VideoFrame;

    /// @brief 视频播放器统一接口
    /// @note 除 hasEvents/dispatchEvents 外，所有接口都可在任意线程调用
    /// @note Listener 回调只会在 dispatchEvents 所在线程（即调用方线程）触发，便于上层直接操作
    class VideoPlayer {
    public:
        /// @brief 事件监听类
        class Listener {
        public:
            virtual ~Listener() { }
            virtual void onVideoStatus(int status, double duration, double position) = 0; // 状态回调
            virtual void onVideoFrame() { }                                               // 帧回调（VM_FRAME 模式下有新帧可取）
        };
    
    public:
        virtual ~VideoPlayer() { }

    public:
        static VideoPlayer* create();        // 创建当前平台的播放器
        static bool         isSupported();   // 当前平台是否支持视频播放
        static VideoMode    getMode();       // 当前后端的输出模式
        static const char*  getBackend();    // 当前后端名称

        virtual bool open(const std::string& url) = 0;          // 打开媒体（重复调用会先释放旧资源）
        virtual bool play() = 0;                                // 开始播放
        virtual bool pause() = 0;                               // 暂停
        virtual bool resume() = 0;                              // 继续
        virtual bool stop() = 0;                                // 停止并释放播放资源（对象仍可再次 open/play）
        virtual bool seek(double ms) = 0;                       // 跳转（ms）
        virtual bool setGeometry(const VideoGeometry& geo) = 0; // 设置视频窗口，任意时刻可调用
        virtual bool setVolume(int volume) = 0;                 // 设置音量 [0,100]
        virtual bool setLoop(bool loop) = 0;                    // 设置循环播放

        virtual int    getStatus()   const = 0;  // 获取当前状态
        virtual double getDuration() const = 0;  // 获取总时长（ms）
        virtual double getPosition() const = 0;  // 获取当前播放进度（ms）

        virtual bool pullFrame(VideoFrame& frame) = 0;      // 取一帧（仅 VM_FRAME），返回 false 表示当前无新帧
        virtual void releaseFrame(VideoFrame& frame) = 0;   // 归还 pullFrame 取得的帧

        virtual void setListener(Listener* listener) = 0;   // 设置事件监听

        virtual bool hasEvents() = 0;          // 上层轮询，检查是否有待派发的事件
        virtual void dispatchEvents() = 0;     // 派发事件（通过 Listener 回调）
    };

} // namespace video

#endif // !__VIDEO_PLAYER_H__
