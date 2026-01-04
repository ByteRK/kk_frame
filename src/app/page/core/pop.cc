/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:55
 * @LastEditTime: 2026-01-04 14:37:22
 * @FilePath: /kk_frame/src/app/page/core/pop.cc
 * @Description: 弹窗基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "pop.h"
#include "wind_mgr.h"
#include <widget/relativelayout.h>

/// @brief 构造
/// @param resource 资源路径
PopBase::PopBase(std::string resource) :PBase(resource) {
    mPopRootView = new RelativeLayout(LayoutParams::MATCH_PARENT, LayoutParams::MATCH_PARENT);
    mPopRootView->setOnTouchListener([](View& v, MotionEvent& e) { return true; });
    mPopRootView->setBackgroundColor(0x99000000);
    mPopRootView->addView(mRootView);
}

/// @brief 析构
PopBase::~PopBase() {
}

/// @brief 获取根节点
/// @return 根节点
View* PopBase::getRootView() {
    return mPopRootView;
}

/// @brief 设置模糊背景
/// @param radius 模糊半径，默认为10
/// @param color 颜色，默认0x99000000
void PopBase::setGlass(int radius, uint color) {
#if 1
    ColorDrawable* d = dynamic_cast<ColorDrawable*>(mPopRootView->getBackground());
    if (d) d->setColor(color);
#else
    // TODO
#endif
}

/// @brief 设置背景颜色
/// @param color 颜色，默认0x99000000
void PopBase::setColor(int color) {
    ColorDrawable* d = dynamic_cast<ColorDrawable*>(mPopRootView->getBackground());
    if (d) d->setColor(color);
}

/*
 ************************************** 注册接口 **************************************
**/

void registerPopToMgr(int8_t pop, std::function<PopBase* ()> func) {
    g_windMgr->registerPop(pop, func);
}

