/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_color.h
 * @Description: 显示测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_COLOR_H__
#define __FACTORY_COLOR_H__

#include "page_factory.h"

/// @brief 显示测试
class FactoryColor : public PageFactory::FactoryItem {
public:
    FactoryColor(ViewGroup* container, PageFactory* factory);

protected:
    void onShow() override;

private:
    void setColor();
};

#endif // __FACTORY_COLOR_H__
