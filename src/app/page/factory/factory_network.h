/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_network.h
 * @Description: 网络信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_NETWORK_H__
#define __FACTORY_NETWORK_H__

#include "page_factory.h"

/// @brief 网络信息
class FactoryNetwork : public PageFactory::FactoryItem {
public:
    FactoryNetwork(View* view, PageFactory* factory);
private:
    void setInterface();
};

#endif // __FACTORY_NETWORK_H__
