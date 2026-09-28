/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-28 10:00:00
 * @LastEditTime: 2026-09-28 10:00:00
 * @FilePath: /kk_frame/src/utils/brand_utils.h
 * @Description: 署名及版权信息（数据源见 config/brand_info.h）
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BRAND_UTILS_H__
#define __BRAND_UTILS_H__

#include <string>

/**
 * 署名信息的唯一出口
 * 数据来源为 config/brand_info.h 中的编码负载，禁止在其它位置硬编码署名文案；
 * 工厂界面、启动 Banner、项目信息页等统一由此取用，保证署名在多处同时存在。
**/
namespace BrandUtils {

    /// @brief 署名所有者
    const std::string& owner();

    /// @brief 版权年份
    const std::string& year();

    /// @brief 上游项目名
    const std::string& project();

    /// @brief 上游仓库地址
    const std::string& repo();

    /// @brief 开源协议标识
    const std::string& license();

    /// @brief 版权行（Copyright (c) {year} by {owner}, All Rights Reserved.）
    std::string copyrightLine();

    /// @brief 项目来源行（Project Based On {project} [{repo}]）
    std::string basedOnLine();

    /// @brief 开源协议行（Licensed under {license}, see LICENSE for details.）
    std::string licenseLine();

    /// @brief 完整署名段落，多行以 \n 分隔
    std::string notice();

} // namespace BrandUtils

#endif // !__BRAND_UTILS_H__
