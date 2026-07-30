/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-23 13:57:05
 * @LastEditTime: 2026-07-29 18:07:35
 * @FilePath: /kk_frame/src/widgets/image_animation_view.cc
 * @Description: 照片动画类 - 基于帧序列的 PNG 动画播放控件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "image_animation_view.h"
#include <dirent.h>
#include <cstring>
#include <map>

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

    // 先移除所有监听，防止 cancel 触发旧的 onAnimationEnd 回调
    mAnimator->removeAllListeners();
    if (mAnimator->isRunning()) {
        mAnimator->cancel();
    }

    mCurrentFrame = 0;

    if (mPhase == Phase::START && !mRepeatPath.empty() && mRepeatFrameCount > 0) {
        // 两阶段：先切换到 start 路径
        mAnimationPath = mStartPath;
        mFrameCount = mStartFrameCount;
    }

    // 单帧：直接显示，无需启动动画
    if (mFrameCount <= 1) {
        setImageResource(mAnimationPath + "/" + mFrameProvider(0));
        if (mPhase == Phase::START && !mRepeatPath.empty() && mRepeatFrameCount > 0) {
            // start 只有一帧，立即切换到 repeat 阶段
            mPhase = Phase::REPEAT;
            mAnimationPath = mRepeatPath;
            mFrameCount = mRepeatFrameCount;
            startAnimation();
        }
        return;
    }

    if (mPhase == Phase::START && !mRepeatPath.empty() && mRepeatFrameCount > 0) {
        // 两阶段：先播放一次开始动画，然后切换到重复播放模式
        mAnimator->setIntValues({ 0, mFrameCount - 1 });
        mAnimator->setDuration(mFrameCount * 1000 / static_cast<float>(mFPS));
        mAnimator->setRepeatCount(0);

        Animator::AnimatorListener listener;
        listener.onAnimationEnd = [this](Animator& animator, bool isReverse) {
            // cancelAnimation 已重置 phase，说明动画被外部打断，跳过
            if (mPhase != Phase::START) return;
            mPhase = Phase::REPEAT;
            mAnimationPath = mRepeatPath;
            mFrameCount = mRepeatFrameCount;
            mCurrentFrame = 0;
            // repeat 只有一帧时直接显示，无需启动动画
            if (mFrameCount <= 1) {
                setImageResource(mAnimationPath + "/" + mFrameProvider(0));
                return;
            }
            mAnimator->removeAllListeners();
            mAnimator->setIntValues({ 0, mFrameCount - 1 });
            mAnimator->setDuration(mFrameCount * 1000 / static_cast<float>(mFPS));
            mAnimator->setRepeatCount(-1);
            mAnimator->start();
        };
        mAnimator->addListener(listener);
    } else {
        mAnimator->setIntValues({ 0, mFrameCount - 1 });
        mAnimator->setDuration(mFrameCount * 1000 / static_cast<float>(mFPS));
        mAnimator->setRepeatCount(-1);
    }

    mAnimator->start();
}

void ImageAnimationView::cancelAnimation() {
    mPhase = Phase::SINGLE;
    if (mAnimator) {
        mAnimator->removeAllListeners();
        mAnimator->cancel();
    }
    mCurrentFrame = 0;
}

void ImageAnimationView::setAnimationPath(const std::string& path, FrameNameProvider provider, int fps) {
    mFPS = fps > 0 ? fps : 24;
    cancelAnimation();
    mAnimationPath = path;
    mFrameCount = path.empty() ? 0 : countPNGFiles(path);
    mFrameProvider = provider;
    mStartPath.clear();
    mRepeatPath.clear();
    mPhase = Phase::SINGLE;
    startAnimation();
}

std::string ImageAnimationView::getRepeatPath() const {
    // 不在播放或已取消时返回空
    if (!mAnimator || !mAnimator->isRunning()) {
        return {};
    }
    // 返回当前循环播放（或即将播放）的路径，便于外部判断是否需要 re-setAnimationPath
    if (mRepeatPath.empty()) {
        return mAnimationPath;
    }
    return mRepeatPath;
}

void ImageAnimationView::setAnimationPath(const std::string& start, const std::string& repeat, FrameNameProvider provider, int fps) {
    mFPS = fps > 0 ? fps : 24;
    cancelAnimation();
    mFrameProvider = provider;
    mStartPath = start;
    mStartFrameCount = start.empty() ? 0 : countPNGFiles(start);
    mRepeatPath = repeat;
    mRepeatFrameCount = repeat.empty() ? 0 : countPNGFiles(repeat);

    // 条件处理
    if (mRepeatFrameCount <= 0) {
        // 无 repeat 路径：回退重复播放 start 路径
        mAnimationPath = mStartPath;
        mFrameCount = mStartFrameCount;
        mPhase = Phase::SINGLE;
    } else if (mStartFrameCount <= 0) {
        // 无 start 路径: 直接重复播放 repeat 路径
        mAnimationPath = mRepeatPath;
        mFrameCount = mRepeatFrameCount;
        mPhase = Phase::SINGLE;
    } else {
        // 完整的两阶段：先 start ，然后 repeat
        mAnimationPath = mStartPath;
        mFrameCount = mStartFrameCount;
        mPhase = Phase::START;
    }
    startAnimation();
}

void ImageAnimationView::onDetachedFromWindow() {
    cancelAnimation();
    ImageView::onDetachedFromWindow();
}

void ImageAnimationView::initAnimator() {
    mAnimator = new ValueAnimator();
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
    // 静态缓存：资源目录在运行期固定不变，无需刷新
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
