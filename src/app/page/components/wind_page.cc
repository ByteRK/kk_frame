/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-02-08 03:57:51
 * @FilePath: /kk_frame/src/app/page/components/wind_page.cc
 * @Description: 页面组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_page.h"

WindPage::WindPage() {
    mPage = nullptr;
    mPageBox = nullptr;
    mPageNextTick = 0;
}

WindPage::~WindPage() {
    removePage();
}

/// @brief 初始化
/// @param parent 父指针 
void WindPage::init(ViewGroup* parent) {
    if (
        !(mPageBox = PBase::get<ViewGroup>(parent, AppRid::page))
        )throw std::runtime_error("WindPage init failed");
    mPageBox->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mPageBox->setVisibility(View::GONE);
}

/// @brief 页面更新
void WindPage::onTick() {
    mPageNextTick = cdroid::SystemClock::uptimeMillis() + 200;
    if (mPage) mPage->callTick();
}

/// @brief 获取页面下次更新时间
/// @return 下次更新时间
int64_t WindPage::getPageNextTick() {
    return mPageNextTick;
}

/// @brief 获取当前页面指针
/// @return 页面指针
PageBase* WindPage::getPage() {
    return mPage;
}

/// @brief 获取当前页面类型
/// @return 页面类型
int8_t WindPage::getPageType() {
    return mPage ? mPage->getType() : PAGE_NULL;
}

/// @brief 显示页面
/// @param page 页面指针
/// @param initData 初始化数据
/// @return 最新页面类型
int8_t WindPage::showPage(PageBase* page, LoadMsgBase* initData) {
    removePage();
    mPage = page;
    if (mPage) {
        mPageBox->setVisibility(View::VISIBLE);
        mPageBox->addView(mPage->getRootView());
        mPage->callAttach();
        mPage->callLoad(initData);
        mPage->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
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

/// @brief 按键监听
/// @param keyCode 键值
/// @param evt 事件
/// @param result 处理结果
/// @return 是否允许下一层处理
bool WindPage::onKey(int keyCode, KeyEvent& evt, bool& result) {
    if (mPage) return false;
    return (result = mPage->callKey(keyCode, evt));
}
