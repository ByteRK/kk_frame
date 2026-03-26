/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-03-22 12:39:12
 * @FilePath: /kk_frame/src/app/page/view/page_factory.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory.h"

PAGE_REGISTER(PAGE_FACTORY, PageFactory);

PageFactory::PageFactory() :PageBase("@layout/page_factory") {
    initUI();
}

PageFactory::~PageFactory() {
}

int8_t PageFactory::getType() const {
    return PAGE_FACTORY;
}
