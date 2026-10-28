/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 15:51:57
 * @FilePath: /kk_frame/src/app/page/factory/factory_info.h
 * @Description: 项目信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_INFO_H__
#define __FACTORY_INFO_H__

#include "page_factory.h"

/// @brief 项目信息
class FactoryInfo : public PageFactory::FactoryItem {
public:
    FactoryInfo(ViewGroup* container, PageFactory* factory);

private:
    void setInfo();
};

#endif // __FACTORY_INFO_H__
