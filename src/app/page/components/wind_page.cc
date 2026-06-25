/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-06-25 14:47:46
 * @FilePath: /kk_frame/src/app/page/components/wind_page.cc
 * @Description: 页面组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_page.h"

WindPage::WindPage() { }

WindPage::~WindPage() {
    removePage();
}

/// @brief 获取当前页面指针
/// @return 页面指针
PageBase* WindPage::getPage() {
    return mPage;
}

/// @brief 获取当前页面类型
/// @return 页面类型
int8_t WindPage::getPageType() const {
    return mPage ? mPage->getType() : PAGE_NULL;
}

/// @brief 显示页面
/// @param page 页面指针
/// @param initData 初始化数据
/// @return 最新页面类型
int8_t WindPage::showPage(PageBase* page, const LoadBase* initData) {
    removePage();
    mPage = page;
    if (mPage) {
        mPageBox->setVisibility(View::VISIBLE);
        mPageBox->addView(mPage->getRootView());
        mPage->callAttach();
        mPage->callLoad(initData);
    }
    return getPageType();
}

/// @brief 移除页面
void WindPage::removePage() {
    if (mPage) {
        mPageBox->setVisibility(View::GONE);
        mPage->callDetach();
        mPageBox->removeAllViews();
        mPage = nullptr;
    }
}

/// @brief 隐藏页面盒子
void WindPage::hidePageBox() {
    mPageBox->setVisibility(View::GONE);
}

/// @brief 显示页面盒子
void WindPage::showPageBox() {
    mPageBox->setVisibility(View::VISIBLE);
}

/// @brief 初始化
/// @param parent 父指针 
void WindPage::init(ViewGroup* parent) {
    if (
        !(mPageBox = PBase::get<ViewGroup>(parent, AppRid::page))
        )throw std::runtime_error("WindPage init failed");
    mPageBox->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mPageBox->setSoundEffectsEnabled(false);
    mPageBox->setVisibility(View::GONE);
}

/// @brief 按键监听
/// @param evt 事件
/// @return 是否已消费
bool WindPage::onKey(KeyEvent& evt) {
    return mPage && mPage->callKey(evt);
}
