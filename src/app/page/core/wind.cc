/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-01-29 14:11:24
 * @FilePath: /kk_frame/src/app/page/core/wind.cc
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "wind.h"
#include "wind_mgr.h"
#include "global_data.h"
#include "config_info.h"
#include "system_utils.h"
#include "app_common.h"

#include "config_mgr.h"

#include <core/app.h>

constexpr int POP_TICK_INTERVAL = 1000;  // 弹窗刷新间隔
constexpr int PAGE_TICK_INTERVAL = 200;  // 页面刷新间隔

/// @brief 点击时系统自动调用
/// @param sound 音量大小(一般不用)
static void playSound(int sound) {
    LOGV("bi~~~~~~~~~~~~~~~~~~~~~~~~");
}

/// @brief 构造以及基础内容检查
/// @return 返回MainWindow对象
MainWindow::MainWindow() :Window(0, 0, -1, -1) {
    mLastAction = SystemClock::uptimeMillis();
    mPopNextTick = 0;
    mPageNextTick = 0;

    App::getInstance().addEventHandler(this);
}

/// @brief 析构
MainWindow::~MainWindow() {
    removePage();
    removePop();
}

/// @brief Tick检查
/// @return 返回1表示需要处理
int MainWindow::checkEvents() {
    return SystemClock::uptimeMillis() >= std::min(mPageNextTick, mPopNextTick);
}

/// @brief Tick处理
/// @return 
int MainWindow::handleEvents() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick >= mPageNextTick) {
        mPageNextTick = tick + PAGE_TICK_INTERVAL;
        if (mPage)mPage->callTick();
        
        WindToast::onTick();
    }
    if (tick >= mPopNextTick) {
        mPopNextTick = tick + POP_TICK_INTERVAL;
        if (mPop)mPop->callTick();
    }
    return 1;
}

/// @brief 初始化窗口
void MainWindow::init() {
    mContext = getContext();

    // 检查根容器并获取相关节点
    if (
        !(mRootView = dynamic_cast<ViewGroup*>(LayoutInflater::from(mContext)->inflate("@layout/wind", this))) ||
        !(mPageBox = dynamic_cast<ViewGroup*>(mRootView->findViewById(AppRid::page_box))) ||
        !(mPopBox = dynamic_cast<ViewGroup*>(mRootView->findViewById(AppRid::pop_box))) ||
        !(mBlackView = mRootView->findViewById(AppRid::cover))
        ) {
        throw std::runtime_error("MainWindow View Tree Error");
    }

    // 初始化页面
    mPage = nullptr;
    mPageBox->setOnTouchListener([](View&, MotionEvent&) { return true; });

    // 初始化弹窗
    mPop = nullptr;
    mPopBox->setOnTouchListener([](View&, MotionEvent&) { return true; });

    // 初始化黑屏
    mIsBlackView = false;
    mBlackView->setOnClickListener([this](View& view) { hideBlack(); });

    // 模块初始化
    WindLogo::init(mRootView);
    WindToast::init(mRootView);

    // 初始化亮度
    SystemUtils::setBrightness(g_config->getBrightness());

    // 增加点击反馈
    mAttachInfo->mPlaySoundEffect = playSound;

    // 显示LOGO
    showLogo();
}

/// @brief 获取LOGO信息
/// @return LOGO信息
WindLogo::LOGO_INFO MainWindow::getLogo() {
    WindLogo::LOGO_INFO info;
    info.path = "@mipmap/ricken";
    info.type = WindLogo::LOGO_TYPE_IMG;
    info.duration = 3000;
    return info;
}

/// @brief 键盘抬起事件（Windows层）
/// @param keyCode 键值
/// @param evt 事件
/// @return 操作结果
bool MainWindow::onKeyUp(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, VIRT_EVENT_UP) || Window::onKeyUp(keyCode, evt);
}

/// @brief 键盘按下事件（Windows层）
/// @param keyCode 键值
/// @param evt 事件
/// @return 操作结果
bool MainWindow::onKeyDown(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, VIRT_EVENT_DOWN) || Window::onKeyDown(keyCode, evt);
}

/// @brief 按键处理
/// @param keyCode 键值
/// @param status 状态
/// @return 是否需要响铃
bool MainWindow::onKey(uint16_t keyCode, uint8_t status) {
    mLastAction = SystemClock::uptimeMillis();
    if (keyCode == KEY_WINDOW) return false;         // 刷新mLastAction用
    if (WindLogo::isLogoShow()) return false;        // LOGO显示时，不响应按键
    if (mIsBlackView) {
        if (status != VIRT_EVENT_DOWN)return false;
        hideBlack();
        return true;
    }
    if (selfKey(keyCode, status)) return true;
    if (mPop) return mPop->callKey(keyCode, status);
    if (mPage) return mPage->callKey(keyCode, status);
    return false;
}

/// @brief 重载触摸事件入口，方便计时最后一次触摸时间
/// @param evt 事件
/// @return 操作结果
bool MainWindow::dispatchTouchEvent(MotionEvent& evt) {
    mLastAction = SystemClock::uptimeMillis();
    return Window::dispatchTouchEvent(evt);
}

/// @brief 获取当前页面指针
/// @return 页面指针
PageBase* MainWindow::getPage() {
    return mPage;
}

/// @brief 获取当前弹窗指针
/// @return 弹窗指针
PopBase* MainWindow::getPop() {
    return mPop;
}

/// @brief 获取当前页面类型
/// @return 页面类型
int8_t MainWindow::getPageType() {
    return mPage ? mPage->getType() : PAGE_NULL;
}

/// @brief 获取当前弹窗类型
/// @return 弹窗类型
int8_t MainWindow::getPopType() {
    return mPop ? mPop->getType() : POP_NULL;
}

/// @brief 显示黑屏
/// @param upload 是否上传(云端使用)
/// @return 操作结果
bool MainWindow::showBlack(bool upload) {
    if (mIsBlackView)return true;
    SystemUtils::setBrightness(0);
    mBlackView->setVisibility(VISIBLE);
    mIsBlackView = true;
    return true;
}

/// @brief 隐藏黑屏
void MainWindow::hideBlack() {
    if (!mIsBlackView)return;
    g_windMgr->showPage(PAGE_HOME);
    mPage->callKey(KEY_WINDOW, VIRT_EVENT_UP);
    mBlackView->setVisibility(GONE);
    mIsBlackView = false;
    SystemUtils::setBrightness(g_config->getBrightness());
}

/// @brief 显示页面
/// @param page 页面指针
/// @param initData 初始化数据
/// @return 最新页面类型
int8_t MainWindow::showPage(PageBase* page, LoadMsgBase* initData) {
    removePage();
    mPage = page;
    if (mPage) {
        mPageBox->addView(mPage->getRootView());
        mPage->callAttach();
        mPage->callLoad(initData);
        mPage->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
    }
    return getPageType();
}

/// @brief 显示弹窗
/// @param pop 弹窗指针
/// @param initData 初始化数据
/// @return 最新页面类型
int8_t MainWindow::showPop(PopBase* pop, LoadMsgBase* initData) {
    removePop();
    mPop = pop;
    if (mPop) {
        mPopBox->setVisibility(VISIBLE);
        mPopBox->addView(mPop->getRootView());
        mPop->callAttach();
        mPop->callLoad(initData);
        mPop->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
    }
    return getPopType();
}

/// @brief 移除页面
void MainWindow::removePage() {
    if (mPage) {
        mPage->callDetach();
        mPageBox->removeAllViews();
        mPage = nullptr;
    }
}

/// @brief 移除弹窗
void MainWindow::removePop() {
    if (mPop) {
        mPopBox->setVisibility(GONE);
        mPop->callDetach();
        mPopBox->removeAllViews();
        mPop = nullptr;
    }
}

/// @brief 隐藏全部元素
void MainWindow::hideAll() {
    mPopBox->setVisibility(GONE);
    hideToast();
    mBlackView->setVisibility(GONE);
}

/// @brief 按键自处理
/// @param keyCode 
/// @param status 
/// @return 
bool MainWindow::selfKey(uint16_t keyCode, uint8_t status) {
    return false;
}
