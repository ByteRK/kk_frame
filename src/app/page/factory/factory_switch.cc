/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_switch.cc
 * @Description: 功能开关
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_switch.h"

FactorySwitch::FactorySwitch(View* view, PageFactory* factory) :
    PageFactory::FactoryItem(view, factory) {
    setExitBtn(AppRid::exit);
    setSwitch();
}

void FactorySwitch::setSwitch() { }
