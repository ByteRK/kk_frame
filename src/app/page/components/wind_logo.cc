/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:31:16
 * @LastEditTime: 2026-08-11 14:01:28
 * @FilePath: /kk_frame/src/app/page/components/wind_logo.cc
 * @Description: Logo组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "wind_logo.h"
#include "base.h"
#include <cdlog.h>

WindLogo::WindLogo() { }

WindLogo::~WindLogo() {
    mImage->removeCallbacks(mRuner);
    mVideo->over();
}

/// @brief 显示Logo
void WindLogo::showLogo() {
    if (!checkInit() || isLogoShow()) return;

    // 获取LOGO类型以及地址
    LOGO_INFO info = getLogo();

    // 根据类型显示LOGO
    mIsRunning = true;
    AnimatedImageDrawable* drawable = nullptr;
    switch (info.type) {
    case LOGO_TYPE_IMG: {
        mImage->setVisibility(View::VISIBLE);
        mImage->setImageResource(info.path);
        mImage->postDelayed(mRuner, info.duration);
    }   break;
    case LOGO_TYPE_ANI: {
        mImage->setVisibility(View::VISIBLE);
        mImage->setImageResource(info.path);
        AnimatedImageDrawable* drawable = dynamic_cast<AnimatedImageDrawable*>(mImage->getDrawable());
        if (drawable) { // 若为动画则调用动画结束回调
            drawable->registerAnimationCallback(mCallback);
            drawable->setRepeatCount(1);
            drawable->start();
        } else { // 若为静态图则延迟关闭
            mImage->postDelayed(mRuner, info.duration);
        }
    }   break;
    case LOGO_TYPE_VIDEO: {
        mVideo->setVisibility(View::VISIBLE);
        mVideo->setURL(info.path);
        mVideo->play();
    }   break;
    default: {
        LOGE("unknow logo type");
        mIsRunning = false;
    }   break;
    }

    if (mIsRunning) mLogo->setVisibility(View::VISIBLE);
}

/// @brief 隐藏Logo
void WindLogo::hideLogo() {
    if (!checkInit() || !isLogoShow()) return;

    // 清除状态，先置位避免回调重入
    mImage->removeCallbacks(mRuner);
    mIsRunning = false;

    // 停止视频播放
    mVideo->over();

    // 停止动画
    AnimatedImageDrawable* drawable = dynamic_cast<AnimatedImageDrawable*>(mImage->getDrawable());
    if (drawable) drawable->stop();

    // 隐藏原有页面
    mLogo->setVisibility(View::GONE);
    mImage->setVisibility(View::GONE);
    mVideo->setVisibility(View::GONE);
}

/// @brief 检查当前是否显示
/// @return 是否显示
bool WindLogo::isLogoShow() const {
    return mIsRunning;
}

/// @brief 初始化Logo
/// @param parent 父节点
void WindLogo::init(ViewGroup* parent) {
    if (mIsInit) return;

    mLogo = PBase::get(parent, AppRid::logo);
    FailFast(mLogo == nullptr, "WindLogo init failed");

    if (
        !(mImage = PBase::get<ImageView>(mLogo, AppRid::logo_image)) ||
        !(mVideo = PBase::get<VideoView>(mLogo, AppRid::logo_video))
        ) {
        LOGE("WindLogo init failed");
        return;
    }

    // 隐藏LOGO
    mLogo->setVisibility(View::GONE);
    mImage->setVisibility(View::GONE);
    mVideo->setVisibility(View::GONE);

    // 锁定点击事件
    mLogo->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mLogo->setSoundEffectsEnabled(false);

    // 静态图LOGO回调
    mRuner = [this] {
        hideLogo();
    };
    // 动图LOGO回调
    mCallback.onAnimationStart = nullptr;
    mCallback.onAnimationEnd = [this](Drawable&) {
        hideLogo();
    };
    // 视频LOGO回调
    mVideo->setOnTouchListener([this](View& v, MotionEvent& evt) { return true; });
    mVideo->setOnPlayStatusChange([this](View& v, int dutation, int progress, int status) {
        LOGE("video play status = %d", status);
        if (status == VideoView::VS_OVER) {
            hideLogo();
        }
    });

    mIsInit = true;
}

/// @brief 按键监听
/// @param evt 事件
/// @return 是否已消费
bool WindLogo::onKey(KeyEvent& evt) {
    return isLogoShow(); // Logo显示时拦截按键
}

/// @brief 检查当前是否已初始化
/// @return 是否已初始化
bool WindLogo::checkInit() {
    if (mIsInit) return true;
    LOGE("Logo uninit");
    return false;
}
