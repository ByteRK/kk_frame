/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_color.cc
 * @Description: 显示测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_color.h"

FactoryColor::FactoryColor(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_color") {
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

FACTORY_ITEM_REGISTER(AppRid::to_color, FactoryColor);
