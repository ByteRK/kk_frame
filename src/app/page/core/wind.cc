/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2026-06-09 00:32:27
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
    LOGI("bi~~~~~~~~~~~~~~~~~~~~~~~~");
}

/// @brief 构造以及基础内容检查
/// @return 返回MainWindow对象
MainWindow::MainWindow() :Window(0, 0, -1, -1) {
    mLastAction = SystemClock::uptimeMillis();
}

/// @brief 析构
MainWindow::~MainWindow() { }

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

/// @brief 重载触摸事件入口，方便计时最后一次触摸时间
/// @param evt 事件
/// @return 操作结果
bool MainWindow::dispatchTouchEvent(MotionEvent& evt) {
    mLastAction = SystemClock::uptimeMillis();
    return Window::dispatchTouchEvent(evt);
}

/// @brief 重载按键事件入口，方便计时最后一次触摸时间
/// @param evt 事件
/// @return 操作结果
bool MainWindow::dispatchKeyEvent(KeyEvent & evt) {
    mLastAction = SystemClock::uptimeMillis();
    return (
        evt.getKeyCode() == KeyEvent::KEYCODE_WINDOW   // 刷新mLastAction用
        || WindLogo::onKey(evt)
        || WindBlack::onKey(evt)
        || selfKey(evt)
        || WindPop::onKey(evt)
        || WindPage::onKey(evt)
        || Window::dispatchKeyEvent(evt)
        );
}

/// @brief 按键监听
/// @param evt 事件
/// @return 是否已消费
bool MainWindow::selfKey(KeyEvent& evt) {
    return false;
}
