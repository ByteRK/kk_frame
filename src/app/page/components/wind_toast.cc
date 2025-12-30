/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:23:15
 * @LastEditTime: 2025-12-30 16:47:21
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
}

WindToast::~WindToast() {
    mToast->removeCallbacks(mRuner);
    mToast->animate().cancel();
}

/// @brief 初始化Toast
/// @param parent 父节点
void WindToast::init(ViewGroup* parent) {
    if (mIsInit) return;

    // 获取节点
    mToast = dynamic_cast<TextView*>(parent->findViewById(APP_NAME::R::id::toast));
    if (!mToast) {
        LOGE("Toast节点获取失败");
        return;
    }

    // 初始化Toast
    mRuner = [this] {
        mLevel = -1;
        mIsRunning = false;
        mToast->animate().alpha(0.f).setDuration(ANIMATETIME).start();
    };

    // Toast动画结束回调
    Animator::AnimatorListener toastAnimtorListener;
    toastAnimtorListener.onAnimationEnd = [this](Animator& animator, bool isReverse) {
        if (mToast->getAlpha() == 0.f) mToast->setVisibility(View::GONE);
    };
    mToast->setVisibility(View::GONE);
    mToast->animate().setListener(toastAnimtorListener);

    mIsInit = true;
}

/// @brief Tick事件
void WindToast::onTick() {
    if (!checkInit() || mIsRunning) return;

    if (mList.size()) {
        TOAST_TYPE item = mList.front();
        mList.pop();

        LOGI("showToast[over:%d]: %s", mList.size(), item.text.c_str());
        mToast->removeCallbacks(mRuner);

        mIsRunning = true;
        mLevel = item.level;
        mToast->setText(item.text);
        mToast->setVisibility(View::VISIBLE);

        if (item.animate) {
            mToast->animate().alpha(1.f).setDuration(ANIMATETIME).start();
        } else {
            mToast->setAlpha(1.f);
        }
        if (!item.lock)mToast->postDelayed(mRuner, DURATION);
    }
}

/// @brief 显示Toast
/// @param text 文本内容
/// @param level 级别
/// @param keepNow 是否保留当前Toast
/// @param animate 是否动画显示
/// @param lock 是否锁定
void WindToast::showToast(std::string text, int8_t level, bool keepNow, bool animate, bool lock) {
    if (!checkInit()) return;

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
    if (!checkInit()) return;
    mLevel = -1;
    mIsRunning = false;
    mToast->animate().cancel();
    mToast->setAlpha(0.f);
    mToast->setVisibility(View::GONE);
    mToast->removeCallbacks(mRuner);
    std::queue<TOAST_TYPE>().swap(mList);
}

/// @brief 当前Toast是否正在显示
/// @return bool
bool WindToast::isToastShow() const {
    return mIsRunning;
}

/// @brief 判断当前Toast是否未初始化
/// @return 
inline bool WindToast::checkInit() {
    if (mIsInit) return true;
    LOGE("Toast未初始化");
    return false;
}
