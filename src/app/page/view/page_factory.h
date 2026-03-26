/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:19
 * @LastEditTime: 2026-03-22 12:34:20
 * @FilePath: /kk_frame/src/app/page/view/page_factory.h
 * @Description: 工厂界面
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PAGE_FACTORY_H__
#define __PAGE_FACTORY_H__

#include "page.h"

class PageFactory :public PageBase {
public:
    PageFactory();
    ~PageFactory();

    int8_t getType() const override;
};

#endif // __PAGE_FACTORY_H__
