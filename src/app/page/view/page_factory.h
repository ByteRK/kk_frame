/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:19
 * @LastEditTime: 2026-10-29 09:08:54
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
#include "item_base.h"
#include "item_switcher.h"

#include <functional>
#include <string>

class PageFactory :public PageBase {
public:
    /// @brief 子测试项基类
    /// @note 布局在构造时载入，挂载/摘除由 ItemSwitcher 统一驱动
    class FactoryItem : public ItemBase {
    protected:
        PageFactory*   mFactory{ nullptr };    // 所属工厂界面

    public:
        FactoryItem(ViewGroup* container, PageFactory* factory, const std::string& layout);
        virtual bool onKey(KeyEvent& evt);

    protected:
        void  exit();
        void  setExitBtn(int id);
        /// @brief 载入子布局并挂载到 parent（语义不同于 ItemBase::inflateItem）
        View* inflateSub(const std::string& layout, ViewGroup* parent);
    };

    using FactorySwitcher = ItemSwitcher<FactoryItem, PageFactory*>;  // 测试项切换器
    using FactoryItemCreator = FactorySwitcher::Creator;              // 子测试项构建函数

public:
    static const int FACTORY_MENU = 0;  // 菜单页标识（非资源id）

private:
    ViewGroup*      mMenuPage{ nullptr };       // 菜单页容器
    ViewGroup*      mContent{ nullptr };        // 测试项内容容器
    FactorySwitcher mSwitcher;                  // 测试项切换器

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
};

/// @brief 测试项注册宏，于子项的.cc文件末尾调用
/// @param MENU_ID 测试项标识（取菜单按钮id，需唯一且非0）
/// @param CLASS 子项类名
#define FACTORY_ITEM_REGISTER(MENU_ID, CLASS) \
    static const bool __FACTORY_ITEM_REG_##CLASS = PageFactory::registerItem( \
        MENU_ID, \
        [](ViewGroup* c, PageFactory* f) -> PageFactory::FactoryItem* { return new CLASS(c, f); })

#endif // __PAGE_FACTORY_H__
