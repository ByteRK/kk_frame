/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:19
 * @LastEditTime: 2026-10-28 16:56:16
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

#include <functional>
#include <map>
#include <string>

class PageFactory :public PageBase {
public:
    /// @brief 子测试项基类
    /// @note 布局由子类在构造时通过基类传入并自行持有所有权
    /// @note 切换时由基类负责挂载/摘除，子类只关心业务回调
    class FactoryItem {
    protected:
        ViewGroup*     mContainer{ nullptr };  // 内容容器（由 PageFactory 提供）
        View*          mRootView{ nullptr };   // 页面根视图（自身持有所有权）
        PageFactory*   mFactory{ nullptr };    // 所属工厂界面

    public:
        FactoryItem(ViewGroup* container, PageFactory* factory, const std::string& layout);
        virtual ~FactoryItem();
        void callShow();
        void callHide();
        virtual bool onKey(KeyEvent& evt);

    protected:
        virtual void onShow() { };   // 显示回调
        virtual void onHide() { };   // 隐藏回调
        void exit();                 // 退出
        void setExitBtn(int id);     // 设置退出按钮

    private:
        void detachView();           // 从容器摘除视图（不触发虚函数，供析构使用）
    };

    // 子测试项构建函数
    using FactoryItemCreator = std::function<FactoryItem*(ViewGroup*, PageFactory*)>;

public:
    static const int FACTORY_MENU = 0;  // 菜单页标识（非资源id）

private:
    ViewGroup*      mMenuPage{ nullptr };       // 菜单页容器
    ViewGroup*      mContent{ nullptr };        // 测试项内容容器
    int             mCurPage{ FACTORY_MENU };   // 当前显示的子页面标识
    FactoryItem*    mCurItem{ nullptr };        // 当前显示的子项
    std::map<int, FactoryItem*> mFactoryItems;  // 测试子项[menuId]，构建后常驻

public:
    PageFactory();
    ~PageFactory();
    int8_t getType() const override;

protected:
    void         getView() override;
    void         setView() override;
    void         onAttach() override;
    void         onDetach() override;
    bool         onKey(KeyEvent& evt) override;

public:
    void         showMenu();

    static bool  registerItem(int menuId, FactoryItemCreator creator);
    static bool  hasItem(int menuId);

private:
    void         onMenuClick(View& v);
    void         checkoutShow(int menuId);
    FactoryItem* getFactoryItem(int menuId);
};

/// @brief 测试项注册宏，于子项的.cc文件末尾调用
/// @param MENU_ID 测试项标识（取菜单按钮id，需唯一且非0）
/// @param CLASS 子项类名
#define FACTORY_ITEM_REGISTER(MENU_ID, CLASS) \
    static const bool __FACTORY_ITEM_REG_##CLASS = PageFactory::registerItem( \
        MENU_ID, \
        [](ViewGroup* c, PageFactory* f) -> PageFactory::FactoryItem* { return new CLASS(c, f); })

#endif // __PAGE_FACTORY_H__
