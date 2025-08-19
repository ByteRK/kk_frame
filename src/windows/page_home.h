/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-08-10 18:10:01
 * @FilePath: /kk_frame/src/windows/page_home.h
 * @Description: 主页面
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef _PAGE_HOME_H_
#define _PAGE_HOME_H_

#include "base.h"

class HomePage :public PageBase {
private:
public:
    HomePage();
    ~HomePage();
    uint8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;
};

#endif // !_PAGE_HOME_H_
