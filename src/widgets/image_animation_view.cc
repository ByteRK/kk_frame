/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-23 13:57:05
 * @LastEditTime: 2026-07-23 15:16:58
 * @FilePath: /ZRPro2/src/widgets/image_animation_view.cc
 * @Description: 照片动画类 - 基于帧序列的 PNG 动画播放控件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "image_animation_view.h"
#include <dirent.h>
#include <cstring>

DECLARE_WIDGET(ImageAnimationView)

ImageAnimationView::ImageAnimationView(int w, int h)
    : ImageView(w, h) {
    initAnimator();
}

ImageAnimationView::ImageAnimationView(Context* ctx, const AttributeSet& attrs)
    : ImageView(ctx, attrs) {
    initAnimator();
}

ImageAnimationView::~ImageAnimationView() {
    cancelAnimation();
    delete mAnimator;
}

void ImageAnimationView::startAnimation() {
    if (mAnimationPath.empty() || mFrameCount <= 0 || !mFrameProvider || !mAnimator) {
        LOGW("Animation path is empty or frame count is zero or frame provider is null or animator is null");
        return;
    }

    if (mAnimator->isRunning()) {
        LOGD("Animation is already running");
        return;
    }

    mCurrentFrame = 0;
    mAnimator->setIntValues({ 0, mFrameCount - 1 });
    mAnimator->setDuration(mFrameCount * 1000 / 24.f);
    mAnimator->start();
}

void ImageAnimationView::cancelAnimation() {
    if (mAnimator && mAnimator->isRunning()) {
        mAnimator->cancel();
    }
    mCurrentFrame = 0;
}

void ImageAnimationView::setAnimationPath(const std::string& path, FrameNameProvider provider) {
    if (mAnimationPath != path) {
        cancelAnimation();
        mAnimationPath = path;
        mFrameCount = path.empty() ? 0 : countPNGFiles(path);
    }
    mFrameProvider = provider;
    startAnimation();
}

void ImageAnimationView::onDetachedFromWindow() {
    cancelAnimation();
    ImageView::onDetachedFromWindow();
}

void ImageAnimationView::initAnimator() {
    mAnimator = new ValueAnimator();
    mAnimator->setRepeatCount(-1);
    mAnimator->setInterpolator(LinearInterpolator::Instance);

    mAnimator->addUpdateListener(ValueAnimator::AnimatorUpdateListener([this](ValueAnimator& anim) {
        const int value = GET_VARIANT(anim.getAnimatedValue(), int);
        if (value != mCurrentFrame && mFrameProvider) {
            mCurrentFrame = value;
            std::string fileName = mFrameProvider(mCurrentFrame);
            setImageResource(mAnimationPath + "/" + fileName);
        }
    }));
}

int ImageAnimationView::countPNGFiles(const std::string& path) {
    static std::map<std::string, int> sCache;
    auto it = sCache.find(path);
    if (it != sCache.end() && it->second > 0) {
        return it->second;
    }

    int count = 0;
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        sCache[path] = 0;
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len > 4 && strcasecmp(name + len - 4, ".png") == 0) {
            count++;
        }
    }
    closedir(dir);
    sCache[path] = count;
    return count;
}
