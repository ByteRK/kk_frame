/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:19
 * @LastEditTime: 2026-04-23 14:56:32
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
#include "touch_test_view.h"
#include <widget/viewflipper.h>

class PageFactory :public PageBase {
private:
    typedef enum {
        FACTORY_MENU = 0,  // 菜单页
        FACTORY_TOUCH,     // 触摸测试页
        FACTORY_COLOR,     // 颜色测试页
        FACTORY_SWITCH,    // 开关功能页

        FACTORY_MAX,       // 最大值
    } FactoryPageType;

private:
    cdroid::ViewFlipper* mFlipper{ nullptr };
    FactoryPageType mCurPage{ FACTORY_MENU };

    TouchTestView*  mTouchView{ nullptr };

public:
    PageFactory();
    ~PageFactory();

    int8_t getType() const override;

protected:
    void getView() override;
    void setView() override;
    void onAttach() override;

private:
    void setFactoryMenu();
    void setFactoryTouch();
    void setFactoryColor();
    void setFactorySwitch();
};

#endif // __PAGE_FACTORY_H__
