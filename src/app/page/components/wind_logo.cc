/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:31:16
 * @LastEditTime: 2025-12-30 17:20:56
 * @FilePath: /kk_frame/src/app/page/components/wind_logo.cc
 * @Description: Logo组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "wind_logo.h"
#include "app_version.h"
#include "R.h"
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
        !(mImage = dynamic_cast<ImageView*>(parent->findViewById(APP_NAME::R::id::logo))) ||
        !(mVideo = dynamic_cast<VideoView*>(parent->findViewById(APP_NAME::R::id::logo_video)))
        ) {
        LOGE("Logo节点获取失败");
        return;
    }

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

/// @brief 当前Logo是否正在显示
/// @return 
bool WindLogo::isLogoShow() const {
    return mIsRunning;
}

/// @brief 判断当前Logo是否未初始化
/// @return 
inline bool WindLogo::checkInit() {
    if (mIsInit) return true;
    LOGE("Logo未初始化");
    return false;
}
