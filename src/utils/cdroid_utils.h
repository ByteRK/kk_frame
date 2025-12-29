/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-12-26 01:53:51
 * @LastEditTime: 2025-12-29 11:13:13
 * @FilePath: /kk_frame/src/utils/cdroid_utils.h
 * @Description: Cdroid相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CDROID_UTILS_H__
#define __CDROID_UTILS_H__

#include <view/keycodes.h>

namespace CdroidUtils {

    /// @brief 模拟按键事件
    /// @param code 按键码
    /// @param value 按键值
    void SENDKEY(int code, int value);

    /// @brief 模拟按键事件
    /// @param code 按键码
    /// @param value 按键值
    void analogInput(int code, int value);

    /// @brief 刷新屏保
    void refreshScreenSaver();

} // CdroidUtils

#endif // !__CDROID_UTILS_H__
