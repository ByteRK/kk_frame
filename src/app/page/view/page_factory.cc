/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-10-28 18:39:41
 * @FilePath: /kk_frame/src/app/page/view/page_factory.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory.h"

#include "wind_mgr.h"

#include "arg_utils.h"
#include "brand_utils.h"

PAGE_REGISTER(PAGE_FACTORY, PageFactory);

/// @brief 注册测试项
/// @note 新增测试项：准备布局 -> 菜单加按钮（可选） -> 子项 .cc 中调用注册宏
bool PageFactory::registerItem(int menuId, FactoryItemCreator creator) {
    return FactorySwitcher::registerItem(menuId, creator);
}

bool PageFactory::hasItem(int menuId) {
    return FactorySwitcher::hasItem(menuId);
}

/************************** 子项类 **************************/

/// @brief 构造（绑定容器并载入布局，不挂载）
/// @param container 内容容器
/// @param factory 工厂界面
/// @param layout 布局资源
PageFactory::FactoryItem::FactoryItem(ViewGroup* container, PageFactory* factory,
    const std::string& layout) {
    mFactory = factory;
    // 先绑定容器，inflater 才会与容器上下文一致
    setContainer(container);
    mRootView = ItemBase::inflateItem(layout, false);
    FailFast(mRootView == nullptr, "inflate factory layout[%s] failed", layout.c_str());
}

bool PageFactory::FactoryItem::onKey(KeyEvent& evt) {
    return false;
}

View* PageFactory::FactoryItem::inflateSub(const std::string& layout, ViewGroup* parent) {
    if (parent == nullptr || mFactory == nullptr) return nullptr;
    return mFactory->mInflater->inflate(layout, parent, true);
}

void PageFactory::FactoryItem::exit() {
    mFactory->showMenu();
}

void PageFactory::FactoryItem::setExitBtn(int id) {
    PBase::click(mRootView, id, [this](View&) {exit();});
}

/************************** 页面类 **************************/

PageFactory::PageFactory() :PageBase("@layout/page_factory") {
    initUI();
}

PageFactory::~PageFactory() {
    mSwitcher.clear();
}

int8_t PageFactory::getType() const {
    return PAGE_FACTORY;
}

void PageFactory::getView() {
    mMenuPage = __dc(ViewGroup, get(AppRid::factory_menu_page));
    mContent = __dc(ViewGroup, get(AppRid::factory_content));
    FailFast(mMenuPage == nullptr || mContent == nullptr, "factory layout view missing");
    mSwitcher.init(mContent);
}

void PageFactory::setView() {
    ViewGroup* menuG = get<ViewGroup>(AppRid::factory_menu);
    auto clickL = std::bind(&PageFactory::onMenuClick, this, std::placeholders::_1);
    for (int i = 0, sum = menuG->getChildCount(); i < sum; ++i) {
        click(menuG->getChildAt(i), clickL);
    }

    // 署名信息统一由 BrandUtils 提供，禁止在此处硬编码
    TextView* copyright = get<TextView>(AppRid::copyright);
    std::string copyrightText = BrandUtils::copyrightLine();
    copyrightText += ("\n" + copyright->getText());
    copyrightText += ("\n" + BrandUtils::basedOnLine());
    copyrightText += ("\n" + BrandUtils::licenseLine());
    copyright->setText(copyrightText);
}

void PageFactory::onAttach() {
    // 直接进入产测则默认显示触摸校准页面
    if (ArgUtils::get().selectPage == PAGE_FACTORY && hasItem(AppRid::to_touch)) {
        checkoutShow(AppRid::to_touch);
    } else {
        checkoutShow(FACTORY_MENU);
    }
    g_window->storeSidebarState();
    g_window->hideSidebar();
    g_window->setScreenSave(false);   // 常亮，取消屏保计时
}

void PageFactory::onDetach() {
    // 摘除当前测试项，避免其定时器/监听在后台继续运行
    checkoutShow(FACTORY_MENU);
    g_window->restoreSidebarState();
    g_window->setScreenSave(true);
}

bool PageFactory::onKey(KeyEvent & evt) {
    FactoryItem* item = mSwitcher.getCurrent();
    return item && item->onKey(evt);
}

void PageFactory::showMenu() {
    checkoutShow(FACTORY_MENU);
}

void PageFactory::onMenuClick(View& v) {
    const int id = v.getId();
    if (id == AppRid::exit) {
        g_windMgr->goToPageBack();
        return;
    }

    if (!hasItem(id)) {
        LOGE("unknow menu click %d", id);
        return;
    }
    checkoutShow(id);
}

/// @brief 切换显示测试项
/// @param menuId 测试项标识，FACTORY_MENU 表示仅隐藏当前项
void PageFactory::checkoutShow(int menuId) {
    mSwitcher.show(menuId, this);

    // 菜单页与测试项互斥显示
    if (mMenuPage) {
        mMenuPage->setVisibility(mSwitcher.isShown() ? View::GONE : View::VISIBLE);
    }
}
