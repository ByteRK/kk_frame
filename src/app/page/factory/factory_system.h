/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_system.h
 * @Description: 系统信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_SYSTEM_H__
#define __FACTORY_SYSTEM_H__

#include "page_factory.h"

/// @brief 系统信息
class FactorySystem : public PageFactory::FactoryItem {
private:
    TextView*              mSystemInfo{ nullptr };  // 信息显示区
    TickMgr::ITickVariable mTicker;                 // 刷新定时器
public:
    FactorySystem(View* view, PageFactory* factory);
    ~FactorySystem() override;
protected:
    void onShow() override;
    void onHide() override;
private:
    void setInfo();
    void onTick(int64_t);
};

#endif // __FACTORY_SYSTEM_H__
