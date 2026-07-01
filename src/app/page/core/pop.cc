/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-04 13:52:55
 * @LastEditTime: 2026-07-01 14:27:40
 * @FilePath: /kk_frame/src/app/page/core/pop.cc
 * @Description: 弹窗基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#define POP_DISPLAY_ANIMATE 0

#include "pop.h"
#include "gauss_drawable.h"
#include "wind_mgr.h"
#include <widget/relativelayout.h>

/// @brief 构造
/// @param resource 资源路径
PopBase::PopBase(std::string resource) :PBase(resource) {
    mPopRootView = new RelativeLayout(LayoutParams::MATCH_PARENT, LayoutParams::MATCH_PARENT);
    mPopRootView->setOnTouchListener([](View& v, MotionEvent& e) { return true; });
    mPopRootView->setSoundEffectsEnabled(false);
    mPopRootView->addView(mRootView);
    setColor();                // 默认背景颜色
    setMargin(0, 0, 0, 0);     // 默认全屏
    setPadding(0, 0, 0, 0);    // 默认无内边距
    setPageDisplay(false);     // 默认不显示底层页面

    mPageDspRunner = []() {g_window->hidePageBox();};
}

/// @brief 析构
PopBase::~PopBase() {
    __delete(mPopRootView);
    mRootView = nullptr;
}

/// @brief 获取根节点
/// @return 根节点
View* PopBase::getRootView() {
    return mPopRootView;
}

/// @brief 挂载
void PopBase::callAttach() {
    if (mIsGauss)applyGauss();
#if POP_DISPLAY_ANIMATE
    mPopRootView->setAlpha(0.f);
    mPopRootView->animate().alpha(1.f).setDuration(300).start();
#endif
    PBase::callAttach();
    if (!mPageDisplay) getRootView()->postDelayed(mPageDspRunner, 500);
}

/// @brief 卸载
void PopBase::callDetach() {
#if POP_DISPLAY_ANIMATE
    mPopRootView->animate().cancel();
#endif
    PBase::callDetach();
    getRootView()->removeCallbacks(mPageDspRunner);
    g_window->showPageBox();
}

/// @brief 关闭弹窗
void PopBase::close() {
    if (g_window->getPopType() == getType())
        g_windMgr->goToPopBack();
}

/// @brief 设置边距(适用于不需要完整覆盖屏幕的情况)
/// @param start 左
/// @param top 上
/// @param end 右
/// @param bottom 下 
void PopBase::setMargin(int start, int top, int end, int bottom) {
    cdroid::MarginLayoutParams* marginParams = dynamic_cast<cdroid::MarginLayoutParams*>(mPopRootView->getLayoutParams());
    if (!marginParams)
        marginParams = new cdroid::MarginLayoutParams(LayoutParams::MATCH_PARENT, LayoutParams::MATCH_PARENT);
    marginParams->setMarginsRelative(start, top, end, bottom);
    mPopRootView->setLayoutParams(marginParams);
}

/// @brief 设置内边距
/// @param start 左
/// @param top 上
/// @param end 右
/// @param bottom 下
void PopBase::setPadding(int start, int top, int end, int bottom) {
    mPopRootView->setPaddingRelative(start, top, end, bottom);
}

/// @brief 设置模糊背景
/// @param radius 模糊半径，默认为10
/// @param color 颜色，默认0x99000000
void PopBase::setGauss(int radius, uint64_t color) {
#if ENABLED(GAUSS_DRAWABLE) || defined(__VSCODE__)
    mIsGauss = true;
    mGaussRadius = radius;
    mGaussColor = color;
    if (mIsAttach)applyGauss();
#else
    setColor(color);
#endif
}

/// @brief 设置背景颜色
/// @param color 颜色，默认0x99000000
void PopBase::setColor(uint64_t color) {
    mIsGauss = false;
    mPopRootView->setBackgroundColor(color);
}

/// @brief 设置底层页面是否显示
/// @param show 是否显示
void PopBase::setPageDisplay(bool show) {
    if (mPageDisplay == show) return;
    mPageDisplay = show;
    if (mIsAttach) {
        getRootView()->removeCallbacks(mPageDspRunner);
        if (mPageDisplay) g_window->showPageBox();
        else getRootView()->postDelayed(mPageDspRunner, 500);
    }
}

/// @brief 应用模糊背景
void PopBase::applyGauss() {
#if ENABLED(GAUSS_DRAWABLE) || defined(__VSCODE__)
    mPopRootView->setBackground(new GaussDrawable(getRootView(), mGaussRadius, 0.5f, mGaussColor, true));
#endif
}
