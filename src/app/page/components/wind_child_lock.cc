/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-08-11 14:12:08
 * @LastEditTime: 2026-08-11 16:54:34
 * @FilePath: /kk_frame/src/app/page/components/wind_child_lock.cc
 * @Description: 童锁组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_child_lock.h"
#include "gauss_drawable.h"
#include "wind_mgr.h"
#include "base.h"

WindChildLock::WindChildLock() { }

WindChildLock::~WindChildLock() {
    mTickVariable.stopTick();
    __delete(mValueAnimator);
}

void WindChildLock::openChildLock() {
    if (!checkInit()) return;

    // 已处于锁定状态，只需确保解锁盒可见
    if (isChildLockOpen()) {
        showChildLockBox();
        return;
    }

    // 进入锁定状态：显示全屏触摸层 + 解锁盒
    mRootView->setVisibility(View::VISIBLE);
    showChildLockBox();
    mSeekBar->setProgress(0);
    mTickVariable.startTick();
    mHideTime = SystemClock::uptimeMillis() + kBoxAutoHideDelayMs;
}

void WindChildLock::closeChildLock() {
    if (!checkInit() || !isChildLockOpen()) return;

    mRootView->setVisibility(View::GONE);
    hideChildLockBox();
    mValueAnimator->cancel();
    mTickVariable.stopTick();
    mHideTime = 0;
}

bool WindChildLock::isChildLockOpen() const {
    if (!mIsInit) return false;
    return mRootView->getVisibility() == View::VISIBLE;
}

void WindChildLock::setChildLockGauss(bool enable, int radius, uint64_t color) {
    mGaussEnable = enable;
    mGaussRadius = radius;
    mGaussColor = color;
}

void WindChildLock::setChildLockStartWithFirstTouch(bool ubf) {
    mUBF = ubf;
}

void WindChildLock::init(ViewGroup* parent) {
    if (mIsInit) return;

    mRootView = PBase::get(parent, AppRid::child_lock);
    FailFast(mRootView == nullptr, "WindChildLock init failed");

    if (
        !(mChildLockBox = PBase::get(mRootView, AppRid::child_lock_box)) ||
        !(mChildLockCover = PBase::get(mRootView, AppRid::child_lock_cover)) ||
        !(mSeekBar = PBase::get<ArcSeekBar>(mRootView, AppRid::child_lock_seekbar))
        ) {
        LOGE("WindChildLock init failed");
        return;
    }

    // 初始化全隐藏
    mRootView->setVisibility(View::GONE);
    mChildLockBox->setVisibility(View::GONE);
    mChildLockCover->setVisibility(View::GONE);

    mRootView->setOnTouchListener([this](View&, MotionEvent& e) {
        switch (e.getAction()) {
        case MotionEvent::ACTION_DOWN: {
            bool boxWasHidden = showChildLockBox();
            if (mUBF || !boxWasHidden) {
                startUnlockAnimation();
            }
        }   break;
        case MotionEvent::ACTION_CANCEL:
        case MotionEvent::ACTION_UP: {
            cancelUnlockAnimation();
        }   break;
        default: break;
        }
        return true;
    });

    mSeekBar->setMin(kSeekBarMin);
    mSeekBar->setMax(kSeekBarMax);
    mSeekBar->setOnChangeListener([this](View&, int progress) {
        mChildLockBox->setActivated(progress >= kBoxActivateValue);
    });

    mValueAnimator = ValueAnimator::ofInt({ kSeekBarMin, kSeekBarMax + kSeekBarPlus });
    mValueAnimator->setInterpolator(nullptr);
    mValueAnimator->setDuration(kUnlockAnimDurationMs);
    mValueAnimator->addUpdateListener([this](ValueAnimator& anim) {
        const int value = GET_VARIANT(anim.getAnimatedValue(), int);
        mSeekBar->setProgress(value >= kSeekBarMax ? kSeekBarMax : value);
        if (value == kSeekBarMax + kSeekBarPlus) closeChildLock();
    });

    mTickVariable.setTick(kTickIntervalMs);
    mTickVariable.setCallBack([this](int64_t now) { onTick(now); });

    mIsInit = true;
}

bool WindChildLock::onKey(KeyEvent& evt) {
    return false;
}

bool WindChildLock::checkInit() {
    if (mIsInit) return true;
    LOGE("ChildLock uninit");
    return false;
}

bool WindChildLock::showChildLockBox() {
    if (mChildLockBox->getVisibility() == View::VISIBLE) return false;
    mChildLockBox->setVisibility(View::VISIBLE);
    mChildLockCover->setVisibility(View::VISIBLE);
#if ENABLED(GAUSS_DRAWABLE) || defined(__VSCODE__)
    if (mGaussEnable)
        mChildLockCover->setBackground(new GaussDrawable(g_window->getRegularLayer(), mGaussRadius, 0.5f, mGaussColor, true));
#endif
    return true;
}

void WindChildLock::hideChildLockBox() {
    mChildLockBox->setVisibility(View::GONE);
    mChildLockCover->setVisibility(View::GONE);
}

void WindChildLock::startUnlockAnimation() {
    mHideTime = 0;
    mValueAnimator->cancel();
    mValueAnimator->start();
}

void WindChildLock::cancelUnlockAnimation() {
    mSeekBar->setProgress(0);
    mValueAnimator->cancel();
    mHideTime = SystemClock::uptimeMillis() + kBoxAutoHideDelayMs;
}

void WindChildLock::onTick(int64_t now) {
    if (mHideTime && now >= mHideTime) {
        hideChildLockBox();
        mHideTime = 0;
    }
}
