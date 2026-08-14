/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-23 13:57:00
 * @LastEditTime: 2026-08-14 15:00:35
 * @FilePath: /kk_frame/src/widgets/image_animation_view.h
 * @Description: 照片动画类 - 基于帧序列的 PNG 动画播放控件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __IMAGE_ANIMATION_VIEW_H__
#define __IMAGE_ANIMATION_VIEW_H__

#include <widget/imageview.h>
#include <animation/valueanimator.h>
#include <functional>
#include <memory>
#include <string>

class ImageAnimationView : public ImageView {
public:
    typedef std::function<std::string(int)> FrameNameProvider;
    typedef std::function<void()> FinishCallback;

private:
    typedef std::function<void()> EndAction;

    int                             mFrameCount{ 0 };
    int                             mCurrentFrame{ 0 };
    int                             mFPS{ 24 };
    std::string                     mAnimationPath{};
    std::unique_ptr<ValueAnimator>  mAnimator;
    FrameNameProvider               mFrameProvider;

    // 两步动画支持：start 播放一次后进入 repeat 循环
    std::string                     mStartPath{};
    std::string                     mRepeatPath{};
    int                             mStartFrameCount{ 0 };
    int                             mRepeatFrameCount{ 0 };

    // 单次播放支持
    bool                            mOnce{ false };
    FinishCallback                  mFinishCallback;

    // 播放代次：每次启动/取消动画时递增，用于使已过期的动画回调失效
    int                             mGeneration{ 0 };

    // 播放状态：启动成功置 true；取消或单次播放自然结束后置 false
    bool                            mPlaying{ false };

public:
    explicit ImageAnimationView(int w, int h);
    ImageAnimationView(Context* ctx, const AttributeSet& attrs);
    ~ImageAnimationView();

public:
    void startAnimation();
    void cancelAnimation();
    void setAnimationPath(const std::string& path, FrameNameProvider provider, int fps = 24);
    void setAnimationPath(const std::string& start, const std::string& repeat, FrameNameProvider provider, int fps = 24);
    /// @brief 设置动画路径，loop 为 false 时仅播放一次，播放结束后触发完成回调
    void setAnimationPath(const std::string& path, FrameNameProvider provider, int fps, bool loop);
    /// @brief 设置两步动画路径，loop 为 false 时 start 与 repeat 各播放一次后触发完成回调
    void setAnimationPath(const std::string& start, const std::string& repeat, FrameNameProvider provider, int fps, bool loop);
    /// @brief 设置单次播放完成回调，仅在 loop=false 且动画自然播放结束时触发（取消/打断不会触发）
    void setFinishCallback(FinishCallback callback);
    /// @brief 返回当前动画路径：两步动画返回循环段(repeat)路径，单路径动画返回动画路径；未配置时返回空
    std::string getRepeatPath() const;
    /// @brief 判断动画是否正在播放；单帧循环视为持续播放，单次播放自然结束后返回 false
    bool isPlaying() const;

protected:
    virtual void onDetachedFromWindow() override;

private:
    void initAnimator();
    int  countPNGFiles(const std::string& path);
    void finishAnimation();

    /// @brief 是否存在完整的 start + repeat 两阶段配置
    bool hasStartPhase() const;
    /// @brief 播放 start 阶段，结束后自动进入 repeat 阶段
    void playStartPhase();
    /// @brief 播放 repeat 阶段，generation 不匹配时直接忽略（动画已被打断）
    void playRepeatPhase(int generation);
    /// @brief 配置并启动一次动画播放；frameCount > 1 时调用
    void playPhase(int frameCount, int repeatCount, EndAction onEnd);
    /// @brief 显示指定帧图片（provider 返回空文件名时忽略）
    void showFrame(int index);
};

#endif // !__IMAGE_ANIMATION_VIEW_H__
