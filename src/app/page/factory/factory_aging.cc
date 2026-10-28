/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_aging.cc
 * @Description: 屏幕老化
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_aging.h"

namespace {
    const static int sTickTime = 3000;
}

FactoryAging::FactoryAging(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_color") {
    PBase::click(mRootView, [this](View&) {exit();});
    mTicker.setTick(sTickTime);
    mTicker.setCallBack(std::bind(&FactoryAging::onTick, this, std::placeholders::_1));
}

FactoryAging::~FactoryAging() {
    mTicker.stopTick();
}

void FactoryAging::onShow() {
    mRootView->getBackground()->setLevel(1);
    mTicker.startTick(sTickTime);
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

FACTORY_ITEM_REGISTER(AppRid::to_aging, FactoryAging);
