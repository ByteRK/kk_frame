/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2025-01-17 01:16:59
 * @FilePath: /kk_frame/src/windows/wind_base.cc
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include <core/app.h>

#include "manage.h"
#include "wind_base.h"
#include "comm_func.h"
#include "R.h"
#include "global_data.h"

#define BL_MAX 96 // 最大亮度
#define BL_MIN 1  // 最小亮度

#define POPTEXT_TRANSLATIONY 76  // 弹幕的Y轴偏移量
#define POPTEXT_ANIMATETIME 600  // 弹幕动画时间

 /// @brief 点击时系统自动调用
 /// @param sound 音量大小(一般不用)
static void playSound(int sound) {
    LOGV("bi~~~~~~~~~~~~~~~~~~~~~~~~");
}

/// @brief 构造以及基础内容检查
/// @return 返回BaseWindow对象
BaseWindow::BaseWindow() :Window(0, 0, -1, -1) {
    m200msTick = 0;
    m1sTick = 0;
    m2sTick = 0;
    App::getInstance().addEventHandler(this);
}

BaseWindow::~BaseWindow() {
    removeCallbacks(mToastRun);
    mToast->animate().cancel();
    __delete(mPop);
    // __delete(g_windMgr);
}

int BaseWindow::checkEvents() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick >= m200msTick) {
        m200msTick = tick + 200;
        return 1;
    }
    return 0;
}

int BaseWindow::handleEvents() {
    int64_t tick = SystemClock::uptimeMillis();

    if (mPage) mPage->callTick();

    if (tick >= m1sTick) {
        m1sTick = tick + 1000;
        if (mPop)mPop->callTick();
    }

    if (tick >= m2sTick) {
        m2sTick = tick + 2000;
        timeTextTick();
    }

    btnLightTick();
    return 1;
}

void BaseWindow::init() {
    mContext = getContext();
    mRootView = (ViewGroup*)(
        LayoutInflater::from(mContext)->inflate("@layout/wind_base", this)
        );
    mPageBox = __getgv(mRootView, ViewGroup, kk_frame::R::id::mainBox);
    mPopBox = __getgv(mRootView, ViewGroup, kk_frame::R::id::popBox);
    mLogo = __getg(mRootView, kk_frame::R::id::logo);
    mToast = __getgv(mRootView, TextView, kk_frame::R::id::toast);
    mTimeText = __getgv(mRootView, TextView, kk_frame::R::id::time);
    mWifiView = __getgv(mRootView, ImageView, kk_frame::R::id::wifi);
    mBlackView = __getgv(mRootView, View, kk_frame::R::id::cover);


    if (!(mPageBox && mPopBox && mTimeText && mWifiView && mToast)) {
        printf("BaseWindow Error\n");
        throw "BaseWindow Error";
    }

    mPage = nullptr;
    mPop = nullptr;
    mIsShowLogo = false;
    mCloseLogo = [this] { mLogo->setVisibility(GONE);mIsShowLogo = false; };
    mPopBox->setOnTouchListener([ ](View& view, MotionEvent& evt) { return true; });

    mIsBlackView = false;

    // 增加点击反馈
    mAttachInfo->mPlaySoundEffect = playSound;

    mToastRun = [this] {
        mToastLevel = -1;
        mToastRunning = false;
        mToast->animate().translationY(POPTEXT_TRANSLATIONY).setDuration(POPTEXT_ANIMATETIME).start();
        };
}

/// @brief 显示 LOGO
/// @param time 
void BaseWindow::showLogo(uint32_t time) {
    removeCallbacks(mCloseLogo);
    mIsShowLogo = true;
    mLogo->setVisibility(VISIBLE);
    postDelayed(mCloseLogo, time);
}

ImageView* BaseWindow::getWifiView() {
    return mWifiView;
}

/// @brief 键盘抬起事件
/// @param keyCode 
/// @param evt 
/// @return 
bool BaseWindow::onKeyUp(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, HW_EVENT_UP) || Window::onKeyUp(keyCode, evt);
}

/// @brief 键盘按下事件
/// @param keyCode 
/// @param evt 
/// @return 
bool BaseWindow::onKeyDown(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, HW_EVENT_DOWN) || Window::onKeyDown(keyCode, evt);
}

/// @brief 按键处理
/// @param keyCode 
/// @param status 
/// @return 是否需要响铃
bool BaseWindow::onKey(uint16_t keyCode, uint8_t status) {
    if (mIsShowLogo)return false;
    if (mPage)mPage->callKey(KEY_MUTE, status);
    if (mIsBlackView) {
        if (status != HW_EVENT_DOWN)return false;
        hideBlack();
        return true;
    }
    if (mPop)return mPop->callKey(keyCode, status);
    if (mPage) return mPage->callKey(keyCode, status);
    return false;
}

/// @brief 获取当前页面指针
/// @return 
PageBase* BaseWindow::getPage() {
    return mPage;
}

PopBase* BaseWindow::getPop() {
    return mPop;
}

/// @brief 获取当前页面类型
/// @return 
int8_t BaseWindow::getPageType() {
    return mPage ? mPage->getType() : PAGE_NULL;
}

/// @brief 获取当前弹窗类型
/// @return 
int8_t BaseWindow::getPopType() {
    return mPop ? mPop->getType() : POP_NULL;
}

/// @brief 显示黑屏
bool BaseWindow::showBlack(bool upload) {
    setBrightness(BL_MIN);
    mBlackView->setVisibility(VISIBLE);
    mIsBlackView = true;
    return true;
}

/// @brief 
void BaseWindow::hideBlack() {
    g_windMgr->goTo(PAGE_STANDBY);
    mPage->callKey(KEY_MUTE, HW_EVENT_UP);
    setBrightness(BL_MAX);
    mBlackView->setVisibility(GONE);
    mIsBlackView = false;
}

/// @brief 显示页面
/// @param page 
void BaseWindow::showPage(PageBase* page) {
    if (mPage)mPage->callDetach();
    mPageBox->removeAllViews();
    mPage = page;
    if (mPage) {
        mPageBox->addView(mPage->getRootView());
        mPage->callAttach();
    }
}

/// @brief 显示弹窗
/// @param pop 
/// @return 
bool BaseWindow::showPop(PopBase* pop) {
    if (mPop) {
        if (mPop->getType() > pop->getType())return false;
        mPop->callDetach();
        removePop();
    }
    mPop = pop;
    if (mPop) {
        mPopBox->setVisibility(VISIBLE);
        mPopBox->addView(mPop->getRootView());
        mPop->callAttach();
    }
    return true;
}

/// @brief 显示弹幕
/// @param text 
/// @param time 
void BaseWindow::showToast(std::string text, int8_t level, bool animate, bool lock) {
    if (mToastRunning && level <= mToastLevel)return;

    removeCallbacks(mToastRun);
    mToastRunning = true;
    mToastLevel = level;
    mToast->setText(text);
    mToast->animate().cancel();
    if (animate) {
        mToast->setTranslationY(POPTEXT_TRANSLATIONY);
        mToast->animate().translationY(0).setDuration(POPTEXT_ANIMATETIME / 3 * 2).start();
    } else {
        mToast->setTranslationY(0);
    }
    if (!lock)
        postDelayed(mToastRun, 3000);
}

/// @brief 移除页面
void BaseWindow::removePage() {
    if (mPage) {
        mPage->callDetach();
        mPageBox->removeAllViews();
        mPage = nullptr;
    }
}

/// @brief 移除弹窗
void BaseWindow::removePop() {
    if (mPop) {
        mPopBox->setVisibility(GONE);
        mPop->callDetach();
        mPopBox->removeAllViews();
        __delete(mPop);
        mPop = nullptr;
    }
}

/// @brief 移除弹幕
void BaseWindow::hideToast() {
    mToastLevel = -1;
    mToastRunning = false;
    mToast->animate().cancel();
    mToast->setTranslationY(POPTEXT_TRANSLATIONY);
}

/// @brief 隐藏全部元素
void BaseWindow::hideAll() {
    mTimeText->setVisibility(GONE);
    mWifiView->setVisibility(GONE);
    mPopBox->setVisibility(GONE);
    mToast->setVisibility(GONE);
    mBlackView->setVisibility(GONE);
}

/// @brief 时间文本更新
void BaseWindow::timeTextTick() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);
    mTimeText->setText(fillLength(now_tm.tm_hour, 2) + ":" + fillLength(now_tm.tm_min, 2));
    mWifiView->setImageLevel(g_data->mNetWork);
}

/// @brief 按键灯更新
void BaseWindow::btnLightTick() {
}
