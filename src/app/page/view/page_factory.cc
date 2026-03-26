/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-03-26 18:07:57
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
    click(AppRid::to_touch, [this](View&) {
        mCurPage = FACTORY_TOUCH;
        mFlipper->setDisplayedChild(mCurPage);
    });

    click(AppRid::to_color, [this](View&) {
        mCurPage = FACTORY_COLOR;
        mFlipper->setDisplayedChild(mCurPage);
        mFlipper->getChildAt(mCurPage)->getBackground()->setLevel(1);
    });

    click(AppRid::exit, [](View&) {
        g_windMgr->goToPageBack();
    });
}

void PageFactory::setFactoryTouch() {


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

void PageFactory::setFactorySwitch() { }
