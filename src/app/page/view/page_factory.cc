/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-07-01 22:41:27
 * @FilePath: /kk_frame/src/app/page/view/page_factory.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory.h"
#include "wind_mgr.h"
#include "arg_utils.h"
#include "string_utils.h"
#include "arg_utils.h"

#include "app_version.h"
#include <core/build.h>

PAGE_REGISTER(PAGE_FACTORY, PageFactory);

PageFactory::PageFactory() :PageBase("@layout/page_factory") {
    initUI();
}

PageFactory::~PageFactory() { }

int8_t PageFactory::getType() const {
    return PAGE_FACTORY;
}

void PageFactory::getView() {
    mFlipper = __dc(ViewFlipper, mRootView);
    mTouchView = get<TouchTestView>(AppRid::touch_test);
}

void PageFactory::setView() {
    setFactoryMenu();
    setFactoryInfo();
    setFactoryTouch();
    setFactoryColor();
    setFactorySwitch();
}

void PageFactory::onAttach() {
    if (ArgUtils::get().selectPage == PAGE_FACTORY) {
        mCurPage = FACTORY_TOUCH;
    } else {
        mCurPage = FACTORY_MENU;
    }
    mFlipper->setDisplayedChild(mCurPage);

    g_window->storeSidebarState();
    g_window->hideSidebar();
    g_window->setScreenSave(false);
}

void PageFactory::onDetach() {
    g_window->restoreSidebarState();
    g_window->setScreenSave(true);
}

void PageFactory::setFactoryMenu() {
    /* 项目信息 */
    click(AppRid::to_info, [this](View&) {
        mCurPage = FACTORY_INFO;
        mFlipper->setDisplayedChild(mCurPage);
    });

    /* 触摸测试 */
    click(AppRid::to_touch, [this](View&) {
        mCurPage = FACTORY_TOUCH;
        mFlipper->setDisplayedChild(mCurPage);
    });

    /* 色彩测试 */
    click(AppRid::to_color, [this](View&) {
        mCurPage = FACTORY_COLOR;
        mFlipper->setDisplayedChild(mCurPage);
        mFlipper->getChildAt(mCurPage)->getBackground()->setLevel(1);
    });

    /* 功能开关 */
    click(AppRid::to_switch, [this](View&) {
        mCurPage = FACTORY_SWITCH;
        mFlipper->setDisplayedChild(mCurPage);
    });

    /* 退出 */
    click(mFlipper->getChildAt(FACTORY_MENU), AppRid::exit, [](View&) {
        g_windMgr->goToPageBack();
    });
}

void PageFactory::setFactoryInfo() {
    TextView* tv = get<TextView>(mFlipper->getChildAt(FACTORY_INFO), AppRid::info);

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
    click(mFlipper->getChildAt(FACTORY_INFO), AppRid::exit, [this](View&) {
        mFlipper->setDisplayedChild(FACTORY_MENU);
    });
}

void PageFactory::setFactoryTouch() {
    mTouchView->setOnAllBlocksActivated([this]() {
        mTouchView->resetTest();
        mFlipper->setDisplayedChild(FACTORY_MENU);
    });
}

void PageFactory::setFactoryColor() {
    click(mFlipper->getChildAt(FACTORY_COLOR), [this](View& v) {
        cdroid::Drawable* bg = v.getBackground();

        int level = bg->getLevel();
        if (level < 7) {
            bg->setLevel(++level);
        } else {
            bg->setLevel(1);
            mFlipper->setDisplayedChild(FACTORY_MENU);
        }
    });
}

void PageFactory::setFactorySwitch() {
    click(mFlipper->getChildAt(FACTORY_SWITCH), AppRid::exit, [this](View&) {
        mFlipper->setDisplayedChild(FACTORY_MENU);
    });
}
