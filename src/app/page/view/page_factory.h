/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:19
 * @LastEditTime: 2026-07-04 04:22:06
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
#include <widget/viewflipper.h>

class PageFactory :public PageBase {
public:
    // 子测试项基类
    class FactoryItem {
    protected:
        View*          mRootView{ nullptr };  // 根视图
        PageFactory*   mFactory{ nullptr };   // 所属工厂界面

    public:
        FactoryItem(View* root, PageFactory* factory);
        virtual ~FactoryItem() { };
        virtual void onShow() { };   // 显示回调
        virtual void onHide() { };   // 隐藏回调

    protected:
        void exit();             // 退出
        void setExitBtn(int id); // 设置退出按钮
    };

private:
    typedef enum {
        FACTORY_MENU = 0,  // 菜单页
        FACTORY_INFO,      // 项目信息页
        FACTORY_NETWORK,   // 网络信息页
        FACTORY_AUTH,      // 授权信息页
        FACTORY_SWITCH,    // 功能开关页
        FACTORY_TOUCH,     // 触摸测试页
        FACTORY_COLOR,     // 显示测试页
        FACTORY_WIFI,      // WIFI检测页
        FACTORY_AGING,     // 屏幕老化页

        FACTORY_MAX,       // 最大值
    } FactoryPageType;

private:
    ViewFlipper*    mFlipper{ nullptr };                   // 页面容器
    FactoryPageType mCurPage{ FACTORY_MENU };              // 当前显示的子页面
    std::map<FactoryPageType, FactoryItem*> mFactoryItems; // 测试子项

public:
    PageFactory();
    ~PageFactory();
    int8_t getType() const override;

    void         showMenu();

protected:
    void         getView() override;
    void         setView() override;
    void         onAttach() override;
    void         onDetach() override;

private:
    void         onMenuClick(View& v);                    // 菜单点击事件
    void         checkoutShow(FactoryPageType page);      // 切换显示项
    FactoryItem* getFactoryItem(FactoryPageType page);    // 获取子项类
    FactoryItem* createFactoryItem(FactoryPageType page); // 创建子项类
};

#endif // __PAGE_FACTORY_H__
