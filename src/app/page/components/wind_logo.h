/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-25 10:31:08
 * @LastEditTime: 2026-06-25 14:48:08
 * @FilePath: /kk_frame/src/app/page/components/wind_logo.h
 * @Description: Logo组件
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_LOGO_H__
#define __WIND_LOGO_H__

#include <view/viewgroup.h>
#include <widget/imageview.h>
#include "video_view.h"

class WindLogo {
protected:
    typedef enum {
        LOGO_TYPE_IMG = 0,         // 图片LOGO
        LOGO_TYPE_ANI,             // 动画LOGO
        LOGO_TYPE_VIDEO,           // 视频LOGO
    } LOGO_TYPE;

    typedef struct {
        std::string   path;                   // 路径
        LOGO_TYPE     type{ LOGO_TYPE_IMG };  // 类型
        int           duration{ 3000 };       // 持续时间(ms)，仅对图片生效
    } LOGO_INFO;

private:
    bool              mIsInit{ false };          // 是否初始化
    bool              mIsRunning{ false };       // 是否正在显示
    Runnable          mRuner;                    // 计时回调(用于图片LOGO)
    Animatable2::AnimationCallback mCallback;    // 动画回调(用于动画LOGO)

    ImageView*        mImage{ nullptr };         // 图片LOGO
    VideoView*        mVideo{ nullptr };         // 视频LOGO

public:
    WindLogo();
    virtual ~WindLogo();

    virtual void      showLogo();
    virtual void      hideLogo();
    virtual LOGO_INFO getLogo() = 0;
    bool              isLogoShow() const;

protected:
    void              init(ViewGroup* parent);
    bool              onKey(KeyEvent& evt);

private:
    bool              checkInit();
};

#endif // __WIND_LOGO_H__