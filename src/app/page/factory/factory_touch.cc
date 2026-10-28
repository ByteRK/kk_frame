/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_touch.cc
 * @Description: 触摸测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_touch.h"

#include "touch_test_view.h"

FactoryTouch::FactoryTouch(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_touch") {
    setTouch();
}

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

FACTORY_ITEM_REGISTER(AppRid::to_touch, FactoryTouch);
