/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-02-11 01:00:57
 * @FilePath: /kk_frame/src/app/page/core/wind.cc
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#include "wind.h"
#include <core/app.h>

/// @brief 点击时系统自动调用
/// @param sound 音量大小(一般不用)
static void playSound(int sound) {
    LOGV("bi~~~~~~~~~~~~~~~~~~~~~~~~");
}

/// @brief 构造以及基础内容检查
/// @return 返回MainWindow对象
MainWindow::MainWindow() :Window(0, 0, -1, -1) {
    mLastAction = SystemClock::uptimeMillis();
    App::getInstance().addEventHandler(this);
}

/// @brief 析构
MainWindow::~MainWindow() {
}

/// @brief 初始化窗口
void MainWindow::init() {
    mContext = getContext();

    // 检查根容器并获取节点
    if (
        !(mRootView = dynamic_cast<ViewGroup*>(LayoutInflater::from(mContext)->inflate("@layout/wind", this)))
        ) {
        throw std::runtime_error("MainWindow View Tree Error");
    }

    // 模块初始化
    WindLogo::init(mRootView);
    WindBlack::init(mRootView);
    WindPage::init(mRootView);
    WindPop::init(mRootView);
    WindToast::init(mRootView);
    WindKeyboard::init(mRootView);

    // 增加点击反馈
    mAttachInfo->mPlaySoundEffect = playSound;

    // 显示LOGO
    showLogo();
}

/// @brief 隐藏全部元素
void MainWindow::hideAll() {
    hidePopBox();
    hideToast();
    hideBlack();
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

/// @brief Tick检查
/// @return 返回1表示需要处理
int MainWindow::checkEvents() {
    return SystemClock::uptimeMillis() >=
        std::min(getPageNextTick(), getPopNextTick());
}

/// @brief Tick处理
/// @return 
int MainWindow::handleEvents() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick >= getPageNextTick()) {
        WindPage::onTick();
        WindToast::onTick();
    }
    if (tick >= getPopNextTick()) {
        WindPop::onTick();
    }
    return 1;
}

/// @brief 重载触摸事件入口，方便计时最后一次触摸时间
/// @param evt 事件
/// @return 操作结果
bool MainWindow::dispatchTouchEvent(MotionEvent& evt) {
    mLastAction = SystemClock::uptimeMillis();
    return Window::dispatchTouchEvent(evt);
}

/// @brief 键盘抬起事件（Windows层）
/// @param keyCode 键值
/// @param evt 事件
/// @return 操作结果
bool MainWindow::onKeyUp(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, evt) || Window::onKeyUp(keyCode, evt);
}

/// @brief 键盘按下事件（Windows层）
/// @param keyCode 键值
/// @param evt 事件
/// @return 操作结果
bool MainWindow::onKeyDown(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, evt) || Window::onKeyDown(keyCode, evt);
}

/// @brief 按键处理
/// @param keyCode 键值
/// @param evt 事件
/// @return 消费结果
bool MainWindow::onKey(int keyCode, KeyEvent& evt) {
    mLastAction = SystemClock::uptimeMillis();
    if (keyCode == KeyEvent::KEYCODE_WINDOW)
        return false;    // 刷新mLastAction用

    bool result = false; // 消费结果(决定是否继续传递到Window)

    if (
        WindLogo::onKey(keyCode, evt, result) ||
        WindBlack::onKey(keyCode, evt, result) ||
        selfKey(keyCode, evt, result) ||
        WindPop::onKey(keyCode, evt, result) ||
        WindPage::onKey(keyCode, evt, result)
        )return result;

    return false;
}

/// @brief 按键监听
/// @param keyCode 键值
/// @param evt 事件
/// @param result 处理结果
/// @return 是否允许下一层处理
bool MainWindow::selfKey(int keyCode, KeyEvent& evt, bool& result) {
    return false;
}
