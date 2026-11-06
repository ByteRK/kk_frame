/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-18 09:33:55
 * @LastEditTime: 2026-11-05 09:56:19
 * @FilePath: /kk_frame/src/class/custom_app.h
 * @Description: 自定义APP
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __CUSTOM_APP_H__
#define __CUSTOM_APP_H__

#include <core/app.h>
#include <set>

/// @brief 自定义APP
class CustomApp : public cdroid::App {
private:
    std::set<std::string> mAddPackagesTag;

public:
    CustomApp(int argc = 0, const char* argv[] = NULL);

public:
    /// @brief 指定名字的资源包是否已加载
    bool checkPackage(const std::string& name);

    /// @brief 加载资源包
    void addPackage(const std::string& filePath, const std::string& name);
};

#endif // __CUSTOM_APP_H__
