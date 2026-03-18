/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-18 09:34:51
 * @LastEditTime: 2026-03-18 10:28:28
 * @FilePath: /kk_frame/src/class/custom_app.cc
 * @Description: 自定义APP
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "custom_app.h"
#include <unistd.h>
#include <cdlog.h>

CustomApp::CustomApp(int argc, const char * argv[]) :cdroid::App(argc, argv) {
}

bool CustomApp::checkPackage(const std::string& name) {
    return mAddPackagesTag.find(name) != mAddPackagesTag.end();
}

void CustomApp::addPackage(const std::string& path, const std::string& name) {
    if (access(path.c_str(), F_OK) != 0) {
        LOGE("package %s not found", path.c_str());
        return;
    }
    mAddPackagesTag.insert(name);
    addResource(path, name);
}