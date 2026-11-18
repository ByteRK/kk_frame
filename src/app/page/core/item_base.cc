/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-18 16:10:00
 * @LastEditTime: 2026-11-18 16:10:00
 * @FilePath: /kk_frame/src/app/page/core/item_base.cc
 * @Description: 容器子项基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "item_base.h"

#include <core/app.h>

#include "macro_define.h"

/// @brief 载入布局，不挂载
ItemBase::ItemBase(const std::string& layout) {
    mRootView = inflateItem(layout, false);
}

/// @brief 析构时虚函数已退化，不回调 onHide，只摘除并释放视图
ItemBase::~ItemBase() {
    detachView();
    __delete(mRootView);
}

/// @brief 设置挂载容器（换容器时先摘除已挂载视图）
void ItemBase::setContainer(ViewGroup* container) {
    if (mContainer == container) return;
    detachView();
    mContainer = container;
}

void ItemBase::callShow() {
    if (mRootView == nullptr) return;
    attachView();
    onShow();
}

void ItemBase::callHide() {
    onHide();
    detachView();
}

/// @brief 载入布局
/// @param attachToContainer true 时直接挂载到 mContainer
/// @note 无论是否挂载，都以 mContainer 作为 root 生成布局参数，避免参数类型与容器不匹配
View* ItemBase::inflateItem(const std::string& layout, bool attachToContainer) {
    LayoutInflater* inflater = mContainer
        ? LayoutInflater::from(mContainer->getContext())
        : LayoutInflater::from(&App::getInstance());
    FailFast(inflater == nullptr, "ItemBase::inflateItem inflater is null");

    if (mContainer == nullptr) attachToContainer = false;
    return inflater->inflate(layout, mContainer, attachToContainer);
}

/// @brief 将根视图挂载到容器
/// @note addView 会触发 onAttachedToWindow 清除 laid-out 标志，依赖 isLaidOut() 的组件
///       （如 Switch 的 thumb 动画）需在 onShow 中自行 requestLayout 重建该状态
void ItemBase::attachView() {
    if (mRootView == nullptr || mContainer == nullptr) return;
    if (mRootView->getParent() == mContainer) return;

    // 残留动画未清理会导致 mParent 未置空，addView 抛异常
    mRootView->clearAnimation();
    auto lp = mRootView->getLayoutParams();
    if (lp) mContainer->addView(mRootView, lp);
    else mContainer->addView(mRootView);
}

void ItemBase::detachView() {
    if (mRootView == nullptr) return;
    ViewGroup* parent = mRootView->getParent();
    if (parent == nullptr) return;

    mRootView->clearAnimation();
    parent->removeView(mRootView);
}
