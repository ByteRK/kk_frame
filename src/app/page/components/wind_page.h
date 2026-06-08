/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-06-09 00:29:56
 * @FilePath: /kk_frame/src/app/page/components/wind_page.h
 * @Description: 页面组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_PAGE_H__
#define __WIND_PAGE_H__

#include "page.h"

class WindPage {
private:
    PageBase*   mPage{ nullptr };       // 页面指针
    ViewGroup*  mPageBox{ nullptr };    // 页面容器

public:
    WindPage();
    virtual ~WindPage();
    
    void       init(ViewGroup* parent);

    PageBase*  getPage();
    int8_t     getPageType();
    int8_t     showPage(PageBase* page, LoadMsgBase* initData = nullptr);
    void       removePage();
    void       hidePageBox();
    void       showPageBox();
    bool       onKey(KeyEvent& evt);
    
};

#endif // !__WIND_PAGE_H__
