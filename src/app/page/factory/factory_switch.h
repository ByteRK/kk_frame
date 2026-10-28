/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_switch.h
 * @Description: 功能开关
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_SWITCH_H__
#define __FACTORY_SWITCH_H__

#include "page_factory.h"

/// @brief 功能开关
class FactorySwitch : public PageFactory::FactoryItem {
public:
    FactorySwitch(View* view, PageFactory* factory);
private:
    void setSwitch();
};

#endif // __FACTORY_SWITCH_H__
