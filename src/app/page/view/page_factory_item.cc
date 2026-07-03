/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-03 17:05:27
 * @LastEditTime: 2026-07-03 19:20:52
 * @FilePath: /kk_frame/src/app/page/view/page_factory_item.cc
 * @Description: 产测页面子项
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory_item.h"

#include "touch_test_view.h"

#include "string_utils.h"
#include "arg_utils.h"

#include "app_version.h"
#include <core/build.h>

/************************** 项目信息 **************************/

FactoryInfo::FactoryInfo(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setInfo();
}

void FactoryInfo::setInfo() {
    TextView* tv = PBase::get<TextView>(mRootView, AppRid::info);

    std::string info;
    info += "软件名称: " APP_NAME_STR "\n";
    info += "软件版本: " APP_VERSION_D "\n";
    info += StringUtils::format("框架版本: Cdroid_V%s_%s\n", cdroid::Build::VERSION::Release, cdroid::Build::VERSION::CommitID);
    info += "代码哈希: " GIT_VERSION "\n";
    info += StringUtils::format("打包人员: %s\n", getlogin());
    info += "打包时间: " BUILD_DATE "\n";
    info += "打包详情: " APP_VER_INFO "\n";
    info += "运行命令: " + ArgUtils::getRawString() + "\n";

    tv->setText(info.c_str());
}

/************************** 功能开关 **************************/

FactorySwitch::FactorySwitch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setSwitch();
}

void FactorySwitch::setSwitch() { }

FactoryTouch::FactoryTouch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setTouch();
}

/************************** 触摸测试 **************************/

void FactoryTouch::onShow() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->resetTest();
}

void FactoryTouch::setTouch() {
    TouchTestView* touch = __dc(TouchTestView, mRootView);
    touch->setOnAllBlocksActivated([this]() {
        exit();
    });
}

/************************** 颜色测试 **************************/

FactoryColor::FactoryColor(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setColor();
}

void FactoryColor::onShow() {
    cdroid::Drawable* bg = mRootView->getBackground();
    bg->setLevel(1);
}

void FactoryColor::setColor() {
    PBase::click(mRootView, [this](View& v) {
        cdroid::Drawable* bg = v.getBackground();
        int level = bg->getLevel();
        if (level < 7) {
            bg->setLevel(++level);
        } else {
            exit();
        }
    });
}

/************************** 老化测试 **************************/

FactoryAging::FactoryAging(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    PBase::click(mRootView, [this](View&) {exit();});
    mTicker.setTick(3000);
    mTicker.setCallBack(std::bind(&FactoryAging::onTick, this, std::placeholders::_1));
}

void FactoryAging::onShow() {
    mRootView->getBackground()->setLevel(1);
    mTicker.startTick();
}

void FactoryAging::onHide() {
    mTicker.stopTick();
}

void FactoryAging::onTick(int64_t) {
    cdroid::Drawable* bg = mRootView->getBackground();
    int level = bg->getLevel();
    if (level < 5) {
        bg->setLevel(++level);
    } else {
        bg->setLevel(1);
    }
}
