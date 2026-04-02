/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-04-02 12:46:33
 * @FilePath: /kk_frame/src/app/page/view/page_factory.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory.h"
#include "wind_mgr.h"

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
    setFactoryTouch();
    setFactoryColor();
    setFactorySwitch();
}

void PageFactory::onTick() { }

void PageFactory::onAttach() {
    mCurPage = FACTORY_MENU;
    mFlipper->setDisplayedChild(mCurPage);
}

void PageFactory::setFactoryMenu() {
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
