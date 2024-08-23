/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2024-08-23 13:46:53
 * @FilePath: /kk_frame/src/windows/wind_base.cc
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#include "wind_base.h"
#include <core/app.h>
#include "comm_func.h"
#include "R.h"
#include "rvNumberPicker.h"

#include "config_mgr.h"
#include "manage.h"
#include "global_data.h"
#include "btn_mgr.h"

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
    removeCallbacks(mPopTextRun);
    mPopText->animate().cancel();
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

    if (mPage) {
        mPage->onTick();
    }

    if (tick >= m1sTick) {
        m1sTick = tick + 1000;
        if (mPop)mPop->onTick();
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
    mInflater = LayoutInflater::from(mContext);
    mBaseView = (ViewGroup*)(mInflater->inflate("@layout/wind_base", this));
    mMainBox = __getgv(mBaseView, ViewGroup, kk_frame::R::id::mainBox);
    mTimeText = __getgv(mBaseView, TextView, kk_frame::R::id::time);
    mWifiView = __getgv(mBaseView, ImageView, kk_frame::R::id::wifi);
    mPopBox = __getgv(mBaseView, ViewGroup, kk_frame::R::id::popBox);
    mPopText = __getgv(mBaseView, TextView, kk_frame::R::id::popText);
    mBlackView = __getgv(mBaseView, View, kk_frame::R::id::cover);
    if (!(mMainBox && mPopBox && mTimeText && mWifiView && mPopText)) {
        printf("BaseWindow Error\n");
        throw "BaseWindow Error";
    }

    mIsShowLogo = false;
    mLogo = __getg(mBaseView, kk_frame::R::id::logo);
    mCloseLogo = [this] { mLogo->setVisibility(GONE);mIsShowLogo = false; };
    mPopBox->setOnTouchListener([ ](View& view, MotionEvent& evt) { return true; });

    mPage = nullptr;
    mPop = nullptr;
    mIsBlackView = false;

    // 增加点击反馈
    mAttachInfo->mPlaySoundEffect = playSound;

    mPopTextRun = [this] {
        mPopTextLevel = -1;
        mPopTextRunning = false;
        mPopText->animate().translationY(POPTEXT_TRANSLATIONY).setDuration(POPTEXT_ANIMATETIME).start();
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
    if (onKey(keyCode, HW_EVENT_UP))LOGD("onKey true");
    return false;
}

/// @brief 键盘按下事件
/// @param keyCode 
/// @param evt 
/// @return 
bool BaseWindow::onKeyDown(int keyCode, KeyEvent& evt) {
    if (onKey(keyCode, HW_EVENT_DOWN))LOGD("onKey true");
    return false;
}

/// @brief 按键处理
/// @param keyCode 
/// @param status 
/// @return 是否需要响铃
bool BaseWindow::onKey(uint16_t keyCode, uint8_t status) {
    if (mIsShowLogo)return false;
    if (mPage)mPage->baseOnKey(KEY_MUTE, status);
    if (mIsBlackView) {
        if (status != HW_EVENT_DOWN)return false;
        hideBlack();
        return true;
    }
    if (mPop)return mPop->onKey(keyCode, status);
    if (mPage) return mPage->baseOnKey(keyCode, status);
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
    return mPage ? mPage->getPageType() : PAGE_NULL;
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
    mPage->baseOnKey(KEY_MUTE, HW_EVENT_UP);
    setBrightness(BL_MAX);
    mBlackView->setVisibility(GONE);
    mIsBlackView = false;
}

/// @brief 显示页面
/// @param page 
void BaseWindow::showPage(PageBase* page) {
    mMainBox->removeAllViews();
    mMainBox->addView(page);
    page->reload();
    mPage = page;
}

/// @brief 显示弹窗
/// @param pop 
/// @return 
bool BaseWindow::showPop(PopBase* pop) {
    if (mPop) {
        if (mPop->getType() > pop->getType())return false;
        removePop();
    }
    mPop = pop;
    mPopBox->addView(pop->mPopView);
    mPopBox->setVisibility(VISIBLE);
    return true;
}

/// @brief 显示弹幕
/// @param text 
/// @param time 
void BaseWindow::showPopText(std::string text, int8_t level, bool animate, bool lock) {
    if (mPopTextRunning && level <= mPopTextLevel)return;

    removeCallbacks(mPopTextRun);
    mPopTextRunning = true;
    mPopTextLevel = level;
    mPopText->setText(text);
    mPopText->animate().cancel();
    if (animate) {
        mPopText->setTranslationY(POPTEXT_TRANSLATIONY);
        mPopText->animate().translationY(0).setDuration(POPTEXT_ANIMATETIME / 3 * 2).start();
    } else {
        mPopText->setTranslationY(0);
    }
    if (!lock)
        postDelayed(mPopTextRun, 3000);
}

/// @brief 移除页面
void BaseWindow::removePage() {
    mMainBox->removeAllViews();
    mPage = nullptr;
}

/// @brief 移除弹窗
void BaseWindow::removePop() {
    if (mPop) {
        mPopBox->setVisibility(GONE);
        mPopBox->removeAllViews();
        delete mPop;
        mPop = nullptr;
    }
}

/// @brief 移除弹幕
void BaseWindow::removePopText() {
    mPopTextLevel = -1;
    mPopTextRunning = false;
    mPopText->animate().cancel();
    mPopText->setTranslationY(POPTEXT_TRANSLATIONY);
}

/// @brief 隐藏全部元素
void BaseWindow::hideAll() {
    mTimeText->setVisibility(GONE);
    mWifiView->setVisibility(GONE);
    mPopBox->setVisibility(GONE);
    mPopText->setVisibility(GONE);
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
