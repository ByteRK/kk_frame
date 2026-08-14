/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-23 13:57:05
 * @LastEditTime: 2026-08-14 15:00:39
 * @FilePath: /kk_frame/src/widgets/image_animation_view.cc
 * @Description: 照片动画类 - 基于帧序列的 PNG 动画播放控件，支持循环/单次播放及完成回调
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "image_animation_view.h"
#include <dirent.h>
#include <algorithm>
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
}

void ImageAnimationView::startAnimation() {
    ++mGeneration;

    if (mAnimationPath.empty() || mFrameCount <= 0 || !mFrameProvider || !mAnimator) {
        LOGW("Animation state invalid: path=%s frameCount=%d provider=%s",
             mAnimationPath.c_str(), mFrameCount, mFrameProvider ? "valid" : "null");
        mPlaying = false;
        return;
    }
    mPlaying = true;

    // 先移除所有监听并停止当前动画，防止旧的 onAnimationEnd 回调被触发
    mAnimator->removeAllListeners();
    mAnimator->cancel();
    mCurrentFrame = 0;

    const int gen = mGeneration;
    if (hasStartPhase()) {
        playStartPhase();
    } else {
        playPhase(mFrameCount, mOnce ? 0 : -1,
                  mOnce ? [this, gen]() { if (gen == mGeneration) finishAnimation(); } : EndAction());
    }
}

bool ImageAnimationView::hasStartPhase() const {
    return !mStartPath.empty() && mStartFrameCount > 0
        && !mRepeatPath.empty() && mRepeatFrameCount > 0;
}

void ImageAnimationView::playStartPhase() {
    const int gen = mGeneration;
    mAnimationPath = mStartPath;
    mFrameCount = mStartFrameCount;

    // start 阶段仅一帧：直接显示后进入 repeat 阶段
    if (mFrameCount <= 1) {
        if (mFrameCount == 1) {
            showFrame(0);
        }
        playRepeatPhase(gen);
        return;
    }
    playPhase(mFrameCount, 0, [this, gen]() {
        if (gen == mGeneration) {
            playRepeatPhase(gen);
        }
    });
}

void ImageAnimationView::playRepeatPhase(int gen) {
    if (gen != mGeneration) return;

    mAnimationPath = mRepeatPath;
    mFrameCount = mRepeatFrameCount;

    // repeat 阶段仅一帧：直接显示，无需启动动画
    if (mFrameCount <= 1) {
        if (mFrameCount == 1) {
            showFrame(0);
        }
        if (mOnce) {
            finishAnimation();
        }
        return;
    }
    playPhase(mFrameCount, mOnce ? 0 : -1,
              mOnce ? [this, gen]() { if (gen == mGeneration) finishAnimation(); } : EndAction());
}

void ImageAnimationView::playPhase(int frameCount, int repeatCount, EndAction onEnd) {
    // 至少 1ms，避免 0 时长导致 ValueAnimator 直接跳到结束
    const int64_t duration = std::max<int64_t>(1, static_cast<int64_t>(frameCount) * 1000 / mFPS);
    mAnimator->removeAllListeners();
    mAnimator->setIntValues({ 0, frameCount - 1 });
    mAnimator->setDuration(duration);
    mAnimator->setRepeatCount(repeatCount);
    if (onEnd) {
        Animator::AnimatorListener listener;
        listener.onAnimationEnd = [onEnd](Animator&, bool) { onEnd(); };
        mAnimator->addListener(listener);
    }
    // 立即显示第 0 帧，保证阶段切换时画面同步（start 最后一帧可能与 repeat 第 0 帧不同）
    showFrame(0);
    mAnimator->start();
}

void ImageAnimationView::showFrame(int index) {
    if (!mFrameProvider) return;
    mCurrentFrame = index;
    const std::string fileName = mFrameProvider(index);
    if (!fileName.empty()) {
        setImageResource(mAnimationPath + "/" + fileName);
    }
}

void ImageAnimationView::cancelAnimation() {
    ++mGeneration;
    if (mAnimator) {
        mAnimator->removeAllListeners();
        mAnimator->cancel();
    }
    mCurrentFrame = 0;
    mPlaying = false;
}

void ImageAnimationView::setAnimationPath(const std::string& path, FrameNameProvider provider, int fps) {
    setAnimationPath(path, provider, fps, true);
}

void ImageAnimationView::setAnimationPath(const std::string& path, FrameNameProvider provider, int fps, bool loop) {
    mFPS = fps > 0 ? fps : 24;
    mOnce = !loop;
    cancelAnimation();
    mAnimationPath = path;
    mFrameCount = path.empty() ? 0 : countPNGFiles(path);
    mFrameProvider = provider;
    mStartPath.clear();
    mRepeatPath.clear();
    startAnimation();
}

std::string ImageAnimationView::getRepeatPath() const {
    // 返回循环阶段实际使用的路径：两步动画返回 repeat 路径，单路径动画返回动画路径
    if (!mRepeatPath.empty()) {
        return mRepeatPath;
    }
    return mAnimationPath;
}

void ImageAnimationView::setAnimationPath(const std::string& start, const std::string& repeat, FrameNameProvider provider, int fps) {
    setAnimationPath(start, repeat, provider, fps, true);
}

void ImageAnimationView::setAnimationPath(const std::string& start, const std::string& repeat, FrameNameProvider provider, int fps, bool loop) {
    mFPS = fps > 0 ? fps : 24;
    mOnce = !loop;
    cancelAnimation();
    mFrameProvider = provider;
    mStartPath = start;
    mStartFrameCount = start.empty() ? 0 : countPNGFiles(start);
    mRepeatPath = repeat;
    mRepeatFrameCount = repeat.empty() ? 0 : countPNGFiles(repeat);

    // 条件处理：无效阶段回退为单路径循环播放，并清空无效路径使 hasStartPhase 判断一致
    if (mRepeatFrameCount <= 0) {
        // 无 repeat 动画：清空 repeat 配置，循环播放 start 路径
        mRepeatPath.clear();
        mAnimationPath = mStartPath;
        mFrameCount = mStartFrameCount;
    } else if (mStartFrameCount <= 0) {
        // 无 start 动画：清空 start 配置，循环播放 repeat 路径
        mStartPath.clear();
        mAnimationPath = mRepeatPath;
        mFrameCount = mRepeatFrameCount;
    } else {
        // 完整的两阶段：先 start，然后 repeat
        mAnimationPath = mStartPath;
        mFrameCount = mStartFrameCount;
    }
    startAnimation();
}

void ImageAnimationView::setFinishCallback(FinishCallback callback) {
    mFinishCallback = callback;
}

void ImageAnimationView::finishAnimation() {
    // 单次播放自然结束：先标记停止播放，再按需触发完成回调
    mPlaying = false;
    if (!mOnce || !mFinishCallback) return;
    mFinishCallback();
}

bool ImageAnimationView::isPlaying() const {
    return mPlaying;
}

void ImageAnimationView::onDetachedFromWindow() {
    cancelAnimation();
    ImageView::onDetachedFromWindow();
}

void ImageAnimationView::initAnimator() {
    mAnimator.reset(new ValueAnimator());
    mAnimator->setInterpolator(LinearInterpolator::Instance);

    mAnimator->addUpdateListener(ValueAnimator::AnimatorUpdateListener([this](ValueAnimator& anim) {
        const int value = GET_VARIANT(anim.getAnimatedValue(), int);
        if (value != mCurrentFrame && value >= 0 && value < mFrameCount && mFrameProvider) {
            mCurrentFrame = value;
            const std::string fileName = mFrameProvider(mCurrentFrame);
            if (!fileName.empty()) {
                setImageResource(mAnimationPath + "/" + fileName);
            }
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
