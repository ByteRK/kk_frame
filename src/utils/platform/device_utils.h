/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-04 14:00:00
 * @LastEditTime: 2026-11-05 14:39:48
 * @FilePath: /kk_frame/src/utils/platform/device_utils.h
 * @Description: 屏体硬件控制（背光、音量）产品适配层
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __DEVICE_UTILS_H__
#define __DEVICE_UTILS_H__

/// @brief 屏体/音频硬件控制
/// @note 未适配的平台只打印日志，不做任何硬件动作
/// @note 新平台接入时只需在本模块补充分支，业务层无需改动
namespace DeviceUtils {

    /// @brief 设置屏幕亮度
    /// @param value 亮度值 [0-100]
    /// @param swap 是否反转亮度值
    void setBrightness(int value, bool swap = false);

    /// @brief 设置音频输出音量
    /// @param value 音量值 [0-100]
    void setVolume(int value);

} // DeviceUtils

#endif // !__DEVICE_UTILS_H__
