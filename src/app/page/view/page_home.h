/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2026-01-04 14:06:50
 * @FilePath: /kk_frame/src/app/page/view/page_home.h
 * @Description: 主页面
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __PAGE_HOME_H__
#define __PAGE_HOME_H__

#include "page.h"

class HomePage :public PageBase {
private:
public:
    HomePage();
    ~HomePage();
    int8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;
};

#endif // !__PAGE_HOME_H__
