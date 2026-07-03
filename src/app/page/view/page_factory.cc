/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-22 12:32:26
 * @LastEditTime: 2026-07-03 22:10:14
 * @FilePath: /kk_frame/src/app/page/view/page_factory.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "page_factory.h"
#include "page_factory_item.h"

#include "wind_mgr.h"

#include "arg_utils.h"

PAGE_REGISTER(PAGE_FACTORY, PageFactory);

/************************** 子项类 **************************/

PageFactory::FactoryItem::FactoryItem(View* root, PageFactory* factory) {
    mFactory = factory;
    mRootView = root;
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
    checkoutShow(FACTORY_MENU);
    for (auto it = mFactoryItems.begin(); it != mFactoryItems.end(); ++it) {
        delete it->second;
    }
    mFactoryItems.clear();
}

int8_t PageFactory::getType() const {
    return PAGE_FACTORY;
}

void PageFactory::showMenu() {
    checkoutShow(FACTORY_MENU);
}

void PageFactory::getView() {
    mFlipper = __dc(ViewFlipper, mRootView);
}

void PageFactory::setView() {
    ViewGroup* menuG = get<ViewGroup>(AppRid::factory_menu);
    auto clickL = std::bind(&PageFactory::onMenuClick, this, std::placeholders::_1);
    for (int i = 0, sum = menuG->getChildCount(); i < sum; ++i) {
        click(menuG->getChildAt(i), clickL);
    }

    TextView* copyright = get<TextView>(AppRid::copyright);
    std::string copyrightText("Copyright (c) 2026 by Ric" "ken, All Rights Reserved.");
    copyrightText += ("\n" + copyright->getText());
    copyrightText += ("\n" "Project Based On k" "k_frame [https://github.com/Byte" "RK/k" "k_frame]");
    copyright->setText(copyrightText);
}

void PageFactory::onAttach() {
    // 直接进入产测则默认显示触摸校准页面
    if (ArgUtils::get().selectPage == PAGE_FACTORY) {
        checkoutShow(FACTORY_TOUCH);
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
    // 恢复侧边栏状态
    g_window->restoreSidebarState();
    // 恢复屏保计时
    g_window->setScreenSave(true);
}

void PageFactory::onMenuClick(View& v) {
    switch (v.getId()) {
    case AppRid::to_info:     checkoutShow(FACTORY_INFO);     break;
    case AppRid::to_network:  checkoutShow(FACTORY_NETWORK);  break;
    case AppRid::to_switch:   checkoutShow(FACTORY_SWITCH);   break;
    case AppRid::to_touch:    checkoutShow(FACTORY_TOUCH);    break;
    case AppRid::to_color:    checkoutShow(FACTORY_COLOR);    break;
    case AppRid::to_wifi:     checkoutShow(FACTORY_WIFI);     break;
    case AppRid::to_aging:    checkoutShow(FACTORY_AGING);    break;
    case AppRid::exit: g_windMgr->goToPageBack(); break;
    default: {
        LOGE("unknow menu click %d", v.getId());
    }   break;
    }
}

void PageFactory::checkoutShow(FactoryPageType page) {
    if (page == mCurPage) return;

    // 获取前后项的指针
    FactoryItem *itemNow = nullptr, *itemNew = nullptr;
    if (mCurPage != FACTORY_MENU) itemNow = getFactoryItem(mCurPage);
    if (page != FACTORY_MENU) itemNew = getFactoryItem(page);

    // 判断目标是否能够正常构建
    if (page != FACTORY_MENU && itemNew == nullptr) {
        LOGE("unknow page %d", page);
        return;
    }

    // 切换
    if (itemNow) itemNow->onHide();
    if (itemNew) itemNew->onShow();
    mFlipper->setDisplayedChild(page);
    mCurPage = page;
}

PageFactory::FactoryItem* PageFactory::getFactoryItem(FactoryPageType page) {
    // 检查缓存
    auto it = mFactoryItems.find(page);
    if (it != mFactoryItems.end()) return it->second;

    // 尝试构建
    PageFactory::FactoryItem* item = createFactoryItem(page);
    FailFast(item == nullptr, "create factory item[%d] failed", page);
    return item;
}

PageFactory::FactoryItem* PageFactory::createFactoryItem(FactoryPageType page) {
    PageFactory::FactoryItem* item = nullptr;
    switch (page) {
    case FACTORY_INFO: {
        item = new FactoryInfo(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_NETWORK: {
        item = new FactoryNetwork(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_SWITCH: {
        item = new FactorySwitch(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_TOUCH: {
        item = new FactoryTouch(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_COLOR: {
        item = new FactoryColor(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_WIFI: {
        item = new FactoryWifi(mFlipper->getChildAt(page), this);
    }   break;
    case FACTORY_AGING: {
        item = new FactoryAging(mFlipper->getChildAt(page), this);
    }   break;
    default: break;
    }
    if (item) mFactoryItems[page] = item;
    return item;
}
