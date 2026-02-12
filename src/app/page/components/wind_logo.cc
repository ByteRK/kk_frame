/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:31:16
 * @LastEditTime: 2026-02-13 00:54:35
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

WindLogo::WindLogo() {
    mIsInit = false;
    mIsRunning = false;
}

WindLogo::~WindLogo() {
    mImage->removeCallbacks(mRuner);
    mVideo->over();
}

/// @brief 初始化Logo
/// @param parent 父节点
void WindLogo::init(ViewGroup* parent) {
    if (mIsInit) return;
    
    if (
        !(mImage = PBase::get<ImageView>(parent, AppRid::logo)) ||
        !(mVideo = PBase::get<VideoView>(parent, AppRid::logo_video))
        ) {
        LOGE("WindLogo init failed");
        return;
    }

    // 锁定点击事件
    mImage->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mVideo->setOnTouchListener([](View&, MotionEvent&) { return true; });

    // 静态图LOGO回调
    mRuner = [this] {
        mImage->setVisibility(View::GONE);
        AnimatedImageDrawable* drawable = dynamic_cast<AnimatedImageDrawable*>(mImage->getDrawable());
        if (drawable) drawable->stop();
        mIsRunning = false;
    };
    // 动图LOGO回调
    mCallback.onAnimationStart = nullptr;
    mCallback.onAnimationEnd = [this](Drawable&) {
        mImage->setVisibility(View::GONE);
        mIsRunning = false;
    };
    // 视频LOGO回调
    mVideo->setOnTouchListener([this](View& v, MotionEvent& evt) { return true; });
    mVideo->setOnPlayStatusChange([this](View& v, int dutation, int progress, int status) {
        LOGE("video play status = %d", status);
        if (status == VideoView::VS_OVER) {
            mVideo->setVisibility(View::GONE);
            mVideo->over();
            mIsRunning = false;
        }
    });

    mIsInit = true;
}

/// @brief 显示Logo
void WindLogo::showLogo() {
    if (!checkInit()) return;

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
    case LOGO_TYPE_VIDEO:
        mVideo->setVisibility(View::VISIBLE);
        mVideo->setURL(info.path);
        mVideo->play();
        break;
    default:
        LOGE("unknow logo type");
        mIsRunning = false;
        break;
    }
}

/// @brief 隐藏Logo
void WindLogo::hideLogo() {
    if (!checkInit()) return;

    // 清除状态
    mImage->removeCallbacks(mRuner);
    mRuner();
    mVideo->over();

    // 隐藏原有页面
    mImage->setVisibility(View::GONE);
    mVideo->setVisibility(View::GONE);
}

/// @brief 检查当前是否显示
/// @return 是否显示
bool WindLogo::isLogoShow() const {
    return mIsRunning;
}

/// @brief 按键监听
/// @param keyCode 键值
/// @param evt 事件
/// @param result 处理结果
/// @return 是否已消费 为true则下层不再处理
bool WindLogo::onKey(int keyCode, KeyEvent& evt, bool& result) {
    return isLogoShow(); // Logo显示时拦截按键
}

/// @brief 检查当前是否已初始化
/// @return 是否已初始化
inline bool WindLogo::checkInit() {
    if (mIsInit) return true;
    LOGE("Logo uninit");
    return false;
}
