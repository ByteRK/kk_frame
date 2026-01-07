/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:23:15
 * @LastEditTime: 2026-01-07 18:01:55
 * @FilePath: /kk_frame/src/app/page/components/wind_toast.cc
 * @Description: Toast组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "wind_toast.h"
#include "app_version.h"
#include "R.h"
#include <cdlog.h>

WindToast::WindToast() {
    mIsInit = false;
    mIsRunning = false;
    mShowListener = nullptr;
}

WindToast::~WindToast() {
    mToastBox->removeCallbacks(mRuner);
    mToastBox->animate().cancel();
}

/// @brief 初始化Toast
/// @param parent 父节点
void WindToast::init(ViewGroup* parent) {
    if (mIsInit) return;

    mToastBox = dynamic_cast<ViewGroup*>(parent->findViewById(APP_NAME::R::id::toast));
    if (!mToastBox) {
        LOGE("Fail to get R::id::toast");
        return;
    }

    // 获取节点
    mToast = dynamic_cast<TextView*>(mToastBox->findViewById(APP_NAME::R::id::toast_text));
    if (!mToast) {
        LOGE("Fail to get R::id::toast_text");
        return;
    }

    // 初始化Toast
    mRuner = [this] {
        mLevel = -1;
        mIsRunning = false;
        mToastBox->animate().alpha(0.f).setDuration(mAnimatime).start();
    };

    // Toast动画结束回调
    Animator::AnimatorListener toastAnimtorListener;
    toastAnimtorListener.onAnimationEnd = [this](Animator& animator, bool isReverse) {
        if (mToastBox->getAlpha() == 0.f) mToastBox->setVisibility(View::GONE);
    };
    mToastBox->setVisibility(View::GONE);
    mToastBox->animate().setListener(toastAnimtorListener);

    mIsInit = true;
}

/// @brief Tick事件
void WindToast::onTick() {
    if (!checkInit() || mIsRunning) return;

    if (mList.size()) {
        TOAST_TYPE item = mList.front();
        mList.pop();

        LOGV("showToast[over:%d]: %s", mList.size(), item.text.c_str());
        mToastBox->removeCallbacks(mRuner);

        mIsRunning = true;
        mLevel = item.level;
        mToast->setText(item.text);
        mToastBox->setVisibility(View::VISIBLE);

        if (item.animate) {
            mToastBox->animate().alpha(1.f).setDuration(mAnimatime).start();
        } else {
            mToastBox->setAlpha(1.f);
        }
        if (!item.lock)mToastBox->postDelayed(mRuner, mDuration);
    }
}

/// @brief 显示Toast
/// @param text 文本内容
void WindToast::showToast(std::string text) {
    showToast(text, 0, false, true, false);
}

/// @brief 显示Toast
/// @param text 文本内容
/// @param level 级别
/// @param keepNow 是否保留当前Toast
/// @param animate 是否动画显示
/// @param lock 是否锁定
void WindToast::showToast(std::string text, int8_t level, bool keepNow, bool animate, bool lock) {
    if (!checkInit()) return;
    if (mShowListener) mShowListener();

    if (!keepNow) {
        if (mIsRunning && level < mLevel) {
            LOGW("new toast level %d < old %d", level, mLevel);
            return;
        }

        if (mList.size()) std::queue<TOAST_TYPE>().swap(mList);
        if (mIsRunning) mIsRunning = false;
    }

    TOAST_TYPE toast;
    toast.text = text;
    toast.level = level;
    toast.animate = animate;
    toast.lock = lock;
    mList.push(toast);
    LOGI("toast list add %s", text.c_str());
}

/// @brief 隐藏Toast
void WindToast::hideToast() {
    if (!checkInit() || !mIsRunning) return;
    mLevel = -1;
    mIsRunning = false;
    mToastBox->animate().cancel();
    mToastBox->setAlpha(0.f);
    mToastBox->setVisibility(View::GONE);
    mToastBox->removeCallbacks(mRuner);
    std::queue<TOAST_TYPE>().swap(mList);
}

/// @brief 当前Toast是否正在显示
/// @return bool
bool WindToast::isToastShow() const {
    return mIsRunning;
}

/// @brief 设置Toast显示时间
/// @param duration 时间 ms
void WindToast::setToastDuration(int duration) {
    mDuration = duration;
}

/// @brief 设置Toast动画时间
/// @param animatime 时间 ms
void WindToast::setToastAnimatime(int animatime) {
    mAnimatime = animatime;
}

/// @brief 设置Toast显示监听
/// @param listener 
void WindToast::setOnShowToastListener(OnShowToastListener listener) {
    mShowListener = listener;
}

/// @brief 判断当前Toast是否未初始化
/// @return 
inline bool WindToast::checkInit() {
    if (mIsInit) return true;
    LOGE("Toast uninit");
    return false;
}
