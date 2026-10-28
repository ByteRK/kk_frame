/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_aging.h
 * @Description: 屏幕老化
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_AGING_H__
#define __FACTORY_AGING_H__

#include "page_factory.h"

/// @brief 屏幕老化
class FactoryAging : public PageFactory::FactoryItem {
private:
    TickMgr::ITickVariable mTicker;

public:
    FactoryAging(ViewGroup* container, PageFactory* factory);
    ~FactoryAging() override;

protected:
    void onShow() override;
    void onHide() override;

private:
    void onTick(int64_t);
};

#endif // __FACTORY_AGING_H__
