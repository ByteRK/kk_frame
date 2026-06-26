/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 01:53:51
 * @LastEditTime: 2026-06-26 11:42:58
 * @FilePath: /kk_frame/src/utils/cdroid_utils.h
 * @Description: Cdroid相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CDROID_UTILS_H__
#define __CDROID_UTILS_H__

#include <view/keyevent.h>
#include <widget/imageview.h>
#include <gui/drawables/drawable.h>

#define SEND_WIND_KEY(down) CdroidUtils::sendKey(cdroid::KeyEvent::KEYCODE_WINDOW, down);

namespace CdroidUtils {

    /// @brief 模拟按键事件
    /// @param code 按键码
    /// @param down 按下或抬起
    void sendKey(int code, bool down);

    /// @brief 模拟按键事件
    /// @param code 按键码
    /// @param value 按键值
    void analogInput(int code, int value);

    /// @brief 设置View的filter(带递归)
    /// @param view View对象
    /// @param filter 是否开启filter
    void setFilterBitmap(View* view, bool filter);

    /// @brief 设置Drawable的filter(带递归)
    /// @param drawable Drawable对象
    /// @param filter 是否开启filter
    void setFilterBitmap(cdroid::Drawable* drawable, bool filter);

    /// @brief 刷新屏保
    void refreshScreenSaver();

} // CdroidUtils

#endif // !__CDROID_UTILS_H__
