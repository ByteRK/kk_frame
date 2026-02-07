/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-02-08 03:58:48
 * @FilePath: /kk_frame/src/app/page/components/wind_pop.cc
 * @Description: 弹窗组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "wind_pop.h"

WindPop::WindPop() {
    mPop = nullptr;
    mPopBox = nullptr;
    mPopNextTick = 0;
}

WindPop::~WindPop() {
    removePop();
}

/// @brief 初始化
/// @param parent 父指针 
void WindPop::init(ViewGroup* parent) {
    if (
        !(mPopBox = PBase::get<ViewGroup>(parent, AppRid::pop))
        )throw std::runtime_error("WindPop init failed");
    mPopBox->setOnTouchListener([](View&, MotionEvent&) { return true; });
    mPopBox->setVisibility(View::GONE);
}

/// @brief 弹窗更新
void WindPop::onTick() {
    mPopNextTick = cdroid::SystemClock::uptimeMillis() + 1000;
    if (mPop) mPop->callTick();
}

/// @brief 获取弹窗下次更新时间
/// @return 下次更新时间
int64_t WindPop::getPopNextTick() {
    return mPopNextTick;
}

/// @brief 获取当前弹窗指针
/// @return 弹窗指针
PopBase* WindPop::getPop() {
    return mPop;
}

/// @brief 获取当前弹窗类型
/// @return 弹窗类型
int8_t WindPop::getPopType() {
    return mPop ? mPop->getType() : POP_NULL;
}

/// @brief 显示弹窗
/// @param pop 弹窗指针
/// @param initData 初始化数据
/// @return 最新弹窗类型
int8_t WindPop::showPop(PopBase* pop, LoadMsgBase* initData) {
    removePop();
    mPop = pop;
    if (mPop) {
        mPopBox->setVisibility(View::VISIBLE);
        mPopBox->addView(mPop->getRootView());
        mPop->callAttach();
        mPop->callLoad(initData);
        mPop->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
    }
    return getPopType();
}

/// @brief 移除弹窗
void WindPop::removePop() {
    if (mPop) {
        mPopBox->setVisibility(View::GONE);
        mPop->callDetach();
        mPopBox->removeAllViews();
        mPop = nullptr;
    }
}

/// @brief 隐藏弹窗盒子
void WindPop::hidePopBox() {
    mPopBox->setVisibility(View::GONE);
}

/// @brief 显示弹窗盒子
void WindPop::showPopBox() {
    mPopBox->setVisibility(View::VISIBLE);
}

/// @brief 按键监听
/// @param keyCode 键值
/// @param evt 事件
/// @param result 处理结果
/// @return 是否允许下一层处理
bool WindPop::onKey(int keyCode, KeyEvent& evt, bool& result) {
    if (mPop) return false;
    return (result = mPop->callKey(keyCode, evt));
}
