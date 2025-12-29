/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-12-16 09:51:10
 * @LastEditTime: 2025-12-26 11:51:36
 * @FilePath: /kk_frame/src/utils/tuya_utils.h
 * @Description: 涂鸦相关的一些函数
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TUYA_UTILS_H__
#define __TUYA_UTILS_H__

#include <string>

namespace TuyaUtils {

    /// @brief 获取天气图标资源路径
    /// @param code 天气代码
    /// @return 图标资源路径
    std::string getWeatherIcon(const std::string& code);

} // TuyaUtils

#endif // !__TUYA_UTILS_H__