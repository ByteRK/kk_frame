/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-12-31 14:37:43
 * @FilePath: /kk_frame/src/app/page/view/page_demo.h
 * @Description: 框架演示主页面（建议保留）
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#ifndef __PAGE_DEMO_H__
#define __PAGE_DEMO_H__

#include "base.h"

class DemoPage :public PageBase {
private:
public:
    DemoPage();
    ~DemoPage();
    int8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;
};

#endif // !__PAGE_DEMO_H__
