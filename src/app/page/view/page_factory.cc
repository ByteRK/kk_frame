/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-10-28 16:56:23
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

#include <vector>

PAGE_REGISTER(PAGE_FACTORY, PageFactory);

/************************** 测试项注册 **************************/

namespace {

    /// @brief 测试项注册条目
    struct ItemEntry {
        int                             menuId{ 0 };        // 测试项标识
        PageFactory::FactoryItemCreator creator{ nullptr }; // 构建函数
    };

    /// @brief 获取测试项注册表
    std::vector<ItemEntry>& registry() {
        static std::vector<ItemEntry> items;
        return items;
    }

    /// @brief 按标识查找注册项
    const ItemEntry* findEntry(int menuId) {
        if (menuId == 0) return nullptr;
        for (const auto& item : registry()) {
            if (item.menuId == menuId) return &item;
        }
        return nullptr;
    }

} // namespace


/// @brief 注册测试项
/// @param menuId 测试项标识（取菜单按钮id，需唯一且非0）
/// @param creator 子项构建函数
/// @return true 注册成功
/// @note 新增测试项：布局文件 -> 菜单中加按钮(可选) -> 子项.cc中注册
bool PageFactory::registerItem(int menuId, FactoryItemCreator creator) {
    if (menuId == 0 || !creator) {
        LOGE("factory item register failed, menuId=%d", menuId);
        return false;
    }
    if (findEntry(menuId)) {
        LOGE("factory item duplicated, menuId=%d", menuId);
        return false;
    }

    ItemEntry entry;
    entry.menuId = menuId;
    entry.creator = creator;
    registry().push_back(entry);
    return true;
}

/// @brief 是否已注册指定测试项
/// @param menuId 测试项标识
bool PageFactory::hasItem(int menuId) {
    return findEntry(menuId) != nullptr;
}

/************************** 子项类 **************************/

/// @brief 构造函数（自动载入布局并挂载到容器）
/// @param container 内容容器
/// @param factory 工厂界面
/// @param layout 布局资源路径
PageFactory::FactoryItem::FactoryItem(ViewGroup* container, PageFactory* factory,
    const std::string& layout) {
    mContainer = container;
    mFactory = factory;
    // 直接挂载到容器，由 inflate 生成与容器匹配的布局参数
    mRootView = mFactory->mInflater->inflate(layout, mContainer, true);
    FailFast(mRootView == nullptr, "inflate factory layout[%s] failed", layout.c_str());
}

/// @brief 析构函数
/// @note 析构期间虚函数已退化，不能依赖子类 onHide，因此只做视图摘除与释放
PageFactory::FactoryItem::~FactoryItem() {
    detachView();
    __delete(mRootView);
}

/// @brief 挂载并通知显示
void PageFactory::FactoryItem::callShow() {
    if (mRootView && mContainer && mRootView->getParent() != mContainer) {
        // 清除可能残留的动画，避免 mParent 未被清空导致 addView 抛异常
        mRootView->clearAnimation();
        mContainer->addView(mRootView, mRootView->getLayoutParams());
    }
    onShow();
}

/// @brief 通知隐藏并摘除
void PageFactory::FactoryItem::callHide() {
    onHide();
    detachView();
}

/// @brief 按键回调
/// @param evt 按键事件
/// @return true 消费
bool PageFactory::FactoryItem::onKey(KeyEvent& evt) {
    return false;
}

/// @brief 从容器摘除视图
void PageFactory::FactoryItem::detachView() {
    if (mRootView && mContainer && mRootView->getParent() == mContainer) {
        mRootView->clearAnimation();
        mContainer->removeView(mRootView);
    }
}

/// @brief 退出测试项
void PageFactory::FactoryItem::exit() {
    mFactory->showMenu();
}

/// @brief 设置退出按钮
/// @param id 
void PageFactory::FactoryItem::setExitBtn(int id) {
    PBase::click(mRootView, id, [this](View&) {exit();});
}

/************************** 页面类 **************************/

PageFactory::PageFactory() :PageBase("@layout/page_factory") {
    initUI();
}

PageFactory::~PageFactory() {
    checkoutShow(FACTORY_MENU);
    for (auto it = mFactoryItems.begin(); it != mFactoryItems.end(); ++it) {
        delete it->second;
    }
    mFactoryItems.clear();
}

int8_t PageFactory::getType() const {
    return PAGE_FACTORY;
}

void PageFactory::getView() {
    mMenuPage = __dc(ViewGroup, get(AppRid::factory_menu_page));
    mContent = __dc(ViewGroup, get(AppRid::factory_content));
    FailFast(mMenuPage == nullptr || mContent == nullptr, "factory layout view missing");
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
    // 侧边栏处理
    g_window->storeSidebarState();
    g_window->hideSidebar();
    // 取消屏保计时，保证常亮
    g_window->setScreenSave(false);
}

void PageFactory::onDetach() {
    // 离开产测页时摘除当前测试项，避免其定时器/监听在后台继续运行
    checkoutShow(FACTORY_MENU);
    // 恢复侧边栏状态
    g_window->restoreSidebarState();
    // 恢复屏保计时
    g_window->setScreenSave(true);
}

bool PageFactory::onKey(KeyEvent & evt) {
    return mCurItem && mCurItem->onKey(evt);
}

/// @brief 显示菜单项
void PageFactory::showMenu() {
    checkoutShow(FACTORY_MENU);
}

/// @brief 菜单点击事件
/// @param v 菜单按钮
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
/// @param menuId 
void PageFactory::checkoutShow(int menuId) {
    if (menuId == mCurPage) return;

    // 先摘除当前项，保证容器内始终只有一个测试项
    if (mCurItem) {
        mCurItem->callHide();
        mCurItem = nullptr;
    }

    // 构建目标项
    FactoryItem* itemNew = nullptr;
    if (menuId != FACTORY_MENU) {
        itemNew = getFactoryItem(menuId);
        if (itemNew == nullptr) {
            // 构建失败，回落到菜单页
            mMenuPage->setVisibility(View::VISIBLE);
            mCurPage = FACTORY_MENU;
            return;
        }
        itemNew->callShow();
    }

    // 菜单页与测试项互斥显示
    mMenuPage->setVisibility(itemNew ? View::GONE : View::VISIBLE);
    mCurItem = itemNew;
    mCurPage = menuId;
}

/// @brief 获取测试项
/// @param menuId 菜单项 ID
/// @return 测试项
PageFactory::FactoryItem* PageFactory::getFactoryItem(int menuId) {
    // 检查缓存
    auto it = mFactoryItems.find(menuId);
    if (it != mFactoryItems.end()) return it->second;

    // 查找注册信息
    const ItemEntry* entry = findEntry(menuId);
    if (entry == nullptr) {
        LOGE("factory item[%d] not registered", menuId);
        return nullptr;
    }

    // 创建测试项
    FactoryItem* item = entry->creator(mContent, this);
    FailFast(item == nullptr, "create factory item[%d] failed", menuId);
    if (item) mFactoryItems[menuId] = item;
    return item;
}
