/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-23 13:57:00
 * @LastEditTime: 2026-07-23 15:10:44
 * @FilePath: /ZRPro2/src/widgets/image_animation_view.h
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
#include <string>

class ImageAnimationView : public ImageView {
public:
    typedef std::function<std::string(int)> FrameNameProvider;

private:
    int               mFrameCount{ 0 };
    int               mCurrentFrame{ 0 };
    std::string       mAnimationPath{};
    ValueAnimator*    mAnimator{ nullptr };
    FrameNameProvider mFrameProvider;

public:
    explicit ImageAnimationView(int w, int h);
    ImageAnimationView(Context* ctx, const AttributeSet& attrs);
    ~ImageAnimationView();

public:
    void startAnimation();
    void cancelAnimation();
    void setAnimationPath(const std::string& path, FrameNameProvider provider);

protected:
    virtual void onDetachedFromWindow() override;

private:
    void initAnimator();
    int  countPNGFiles(const std::string& path);
};

#endif // !__IMAGE_ANIMATION_VIEW_H__
