/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_info.cc
 * @Description: 项目信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_info.h"

#include "string_utils.h"
#include "arg_utils.h"
#include "brand_utils.h"

#include "app_version.h"
#include "series_info.h"
#include <core/build.h>

#include <unistd.h>

FactoryInfo::FactoryInfo(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_info") {
    setExitBtn(AppRid::exit);
    setInfo();
}

void FactoryInfo::setInfo() {
    TextView* tv = PBase::get<TextView>(mRootView, AppRid::info);
    std::string info;

    // 软件信息
    info += "软件名称: " APP_NAME_STR "\n";
    info += "软件版本: V" APP_VERSION_D "\n";
    info += StringUtils::format("框架版本: Cdroid_V%s_%s\n", cdroid::Build::VERSION::Release, cdroid::Build::VERSION::CommitID);
    info += "代码哈希: " GIT_VERSION "\n";
    info += StringUtils::format("打包人员: %s\n", getlogin());
    info += "打包时间: " BUILD_DATE "\n";
    info += "打包详情: " APP_VER_INFO "\n";
    info += "运行命令: " + ArgUtils::getRawString() + "\n";
    info += "版权署名: " + BrandUtils::copyrightLine() + "\n";

    info += "\n";

    // 硬件信息
    info += "核心: " CPU_BRAND " " CPU_NAME "\n";
    info += "内存: " MEMORY_NAME " " MEMORY_SIZE "\n";
    info += "闪存: " FLASH_NAME " " FLASH_SIZE "\n";
    info += "屏幕: " SCREEN_SIZE "\n";

    tv->setText(info.c_str());
}

FACTORY_ITEM_REGISTER(AppRid::to_info, FactoryInfo);
