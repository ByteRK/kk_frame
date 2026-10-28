/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_touch.h
 * @Description: 触摸测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_TOUCH_H__
#define __FACTORY_TOUCH_H__

#include "page_factory.h"

/// @brief 触摸测试
class FactoryTouch : public PageFactory::FactoryItem {
public:
    FactoryTouch(ViewGroup* container, PageFactory* factory);

protected:
    void onShow() override;

private:
    void setTouch();
};

#endif // __FACTORY_TOUCH_H__
