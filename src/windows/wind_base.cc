/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 14:51:04
 * @LastEditTime: 2025-05-03 20:52:13
 * @FilePath: /kk_frame/src/windows/wind_base.cc
 * @Description: 窗口类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
 */

#include <core/app.h>
#include "manage.h"
#include "wind_base.h"
#include "comm_func.h"
#include "global_data.h"

#include "btn_mgr.h"
#include "config_mgr.h"
#include "config_info.h"

constexpr int POP_TICK_INTERVAL = 1000;  // 弹窗刷新间隔
constexpr int PAGE_TICK_INTERVAL = 200;  // 页面刷新间隔

constexpr int POPTEXT_TRANSLATIONY = 76;    // 弹幕的Y轴偏移量
constexpr int POPTEXT_ANIMATETIME = 600;    // 弹幕动画时间

/// @brief 点击时系统自动调用
/// @param sound 音量大小(一般不用)
static void playSound(int sound) {
    LOGV("bi~~~~~~~~~~~~~~~~~~~~~~~~");
}

/// @brief 构造以及基础内容检查
/// @return 返回BaseWindow对象
BaseWindow::BaseWindow() :Window(0, 0, -1, -1) {
    mLastAction = SystemClock::uptimeMillis();
    mLastTick = 0;
    mPopTickTime = 0;
    mPageTickTime = 0;

    mToastRunning = false;
    mToastLevel = -1;

    App::getInstance().addEventHandler(this);
}

BaseWindow::~BaseWindow() {
    removeCallbacks(mToastRun);
    mToast->animate().cancel();
    __delete(mPop);
}

int BaseWindow::checkEvents() {
    int64_t tick = SystemClock::uptimeMillis();
    if (tick >= mLastTick) {
        mLastTick = tick + 100;
        return 1;
    }
    return 0;
}

int BaseWindow::handleEvents() {
    int64_t tick = SystemClock::uptimeMillis();

    if (tick >= mPageTickTime) {
        mPageTickTime = tick + PAGE_TICK_INTERVAL;
        if (mPage)mPage->callTick();

        toastTick();
    }

    if (tick >= mPopTickTime) {
        mPopTickTime = tick + POP_TICK_INTERVAL;
        if (mPop)mPop->callTick();
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
    mLogoForImage = __getgv(mRootView, ImageView, kk_frame::R::id::logo);
    mLogoForVideo = __getgv(mRootView, VideoView, kk_frame::R::id::logo_video);
    mToast = __getgv(mRootView, TextView, kk_frame::R::id::toast);
    mBlackView = __getgv(mRootView, View, kk_frame::R::id::cover);

    if (!(mPageBox && mPopBox && mLogoForImage && mLogoForVideo && mToast && mBlackView))
        throw std::runtime_error("BaseWindow Error");

    // 初始化页面指针
    mPage = nullptr;

    // 初始化弹窗指针
    mPop = nullptr;
    mPopBox->setOnTouchListener([ ](View& view, MotionEvent& evt) { return true; });

    // 初始化黑屏
    mIsBlackView = false;
    __clickv(mBlackView, [this](View& view) { hideBlack(); });
    mBlackView->setEnabled(false);
    setBrightness(g_config->getBrightness(), true);

    // 增加点击反馈
    mAttachInfo->mPlaySoundEffect = playSound;

    // 初始化Toast
    mToastRun = [this] {
        mToastLevel = -1;
        mToastRunning = false;
        mToast->animate().alpha(0.f).setDuration(POPTEXT_ANIMATETIME).start();
        };

    // Toast动画结束回调
    Animator::AnimatorListener toastAnimtorListener;
    toastAnimtorListener.onAnimationEnd = [this](Animator& animator, bool isReverse) {
        if (mToast->getAlpha() == 0.f) mToast->setVisibility(View::GONE);
        };
    mToast->setVisibility(View::GONE);
    mToast->animate().setListener(toastAnimtorListener);

    // 初始化LOGO
    mIsShowLogo = false;
    // 静态图LOGO回调
    mCloseLogo = [this] {
        mLogoForImage->setVisibility(GONE);
        AnimatedImageDrawable* drawable = __dc(AnimatedImageDrawable, mLogoForImage->getDrawable());
        if (drawable) drawable->stop();
        mIsShowLogo = false;
        };
    // 动图LOGO回调
    mLogoForImageCallback.onAnimationStart = nullptr;
    mLogoForImageCallback.onAnimationEnd = [this](Drawable&) {
        mLogoForImage->setVisibility(GONE);
        mIsShowLogo = false;
        };
    // 视频LOGO回调
    mLogoForVideo->setOnTouchListener([this](View& v, MotionEvent& evt) { return true; });
    mLogoForVideo->setOnPlayStatusChange([this](View& v, int dutation, int progress, int status) {
        LOGE("video play status = %d", status);
        if (status == VideoView::VS_OVER) {
            mLogoForVideo->setVisibility(View::GONE);
            mLogoForVideo->over();
            mIsShowLogo = false;
        }
        });

    showLogo();
}

/// @brief 显示 LOGO
/// @param time 
void BaseWindow::showLogo(uint32_t time) {
    // 清除状态
    removeCallbacks(mCloseLogo);
    mCloseLogo();
    mLogoForVideo->over();
    // 隐藏原有页面
    mLogoForImage->setVisibility(View::GONE);
    mLogoForVideo->setVisibility(View::GONE);
    // 获取LOGO类型以及地址
    std::string path;
    LOGO_TYPE type = getLogo(path);

    // 根据类型显示LOGO
    mIsShowLogo = true;
    AnimatedImageDrawable* drawable = nullptr;
    switch (type) {
    case LOGO_TYPE_IMG:
        mLogoForImage->setVisibility(VISIBLE);
        mLogoForImage->setImageResource(path);
        postDelayed(mCloseLogo, time);
        break;
    case LOGO_TYPE_ANI:
        mLogoForImage->setVisibility(VISIBLE);
        mLogoForImage->setImageResource(path);
        drawable = __dc(AnimatedImageDrawable, mLogoForImage->getDrawable());
        if (drawable) { // 若为动画则调用动画结束回调
            drawable->registerAnimationCallback(mLogoForImageCallback);
            drawable->setRepeatCount(1);
            drawable->start();
        } else { // 若为静态图则延迟关闭
            postDelayed(mCloseLogo, time);
        }
        break;
    case LOGO_TYPE_VIDEO:
        mLogoForVideo->setVisibility(VISIBLE);
        mLogoForVideo->setURL(path);
        mLogoForVideo->play();
        break;
    default:
        LOGE("unknow logo type");
        mIsShowLogo = false;
        break;
    }
}

/// @brief 获取logo
/// @param path 
/// @return 
BaseWindow::LOGO_TYPE BaseWindow::getLogo(std::string& path) {
    BaseWindow::LOGO_TYPE type = LOGO_TYPE_IMG;

    path = "@mipmap/logo";
    type = LOGO_TYPE_IMG;

    return type;
}

/// @brief 键盘抬起事件
/// @param keyCode 
/// @param evt 
/// @return 
bool BaseWindow::onKeyUp(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, VIRT_EVENT_UP) || Window::onKeyUp(keyCode, evt);
}

/// @brief 键盘按下事件
/// @param keyCode 
/// @param evt 
/// @return 
bool BaseWindow::onKeyDown(int keyCode, KeyEvent& evt) {
    return onKey(keyCode, VIRT_EVENT_DOWN) || Window::onKeyDown(keyCode, evt);
}

/// @brief 按键处理
/// @param keyCode 
/// @param status 
/// @return 是否需要响铃
bool BaseWindow::onKey(uint16_t keyCode, uint8_t status) {
    mLastAction = SystemClock::uptimeMillis();
    if (keyCode == KEY_WINDOW)return false;  // 刷新mLastAction用
    if (mIsShowLogo) return false;
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
/// @param evt 
/// @return 
bool BaseWindow::dispatchTouchEvent(MotionEvent& evt) {
    mLastAction = SystemClock::uptimeMillis();
    return Window::dispatchTouchEvent(evt);
}

/// @brief 获取当前页面指针
/// @return 
PageBase* BaseWindow::getPage() {
    return mPage;
}

/// @brief 获取当前弹窗指针
/// @return 
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
    if (mIsBlackView)return true;
    setBrightness(CONFIG_BRIGHTNESS_MIN);
    mBlackView->setVisibility(VISIBLE);
    mIsBlackView = true;
    return true;
}

/// @brief 
void BaseWindow::hideBlack() {
    if (!mIsBlackView)return;
    g_windMgr->goTo(PAGE_STANDBY);
    mPage->callKey(KEY_WINDOW, VIRT_EVENT_UP);
    mBlackView->setVisibility(GONE);
    mIsBlackView = false;
    setBrightness(g_config->getBrightness());
}

/// @brief 显示页面
/// @param page 
void BaseWindow::showPage(PageBase* page) {
    removePage();
    mPage = page;
    if (mPage) {
        mPageBox->addView(mPage->getRootView());
        mPage->callAttach();
        mPage->callReload();
        mPage->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
    }
}

/// @brief 显示弹窗
/// @param pop 
void BaseWindow::showPop(PopBase* pop) {
    removePop();
    mPop = pop;
    if (mPop) {
        mPopBox->setVisibility(VISIBLE);
        mPopBox->addView(mPop->getRootView());
        mPop->callAttach();
        mPop->callReload();
        mPop->callTick(); // 为了避免有些页面变化是通过tick更新的，导致页面刚载入时闪烁
    }
}

/// @brief 显示弹幕
/// @param text 
/// @param time 
void BaseWindow::showToast(std::string text, int8_t level, bool keepNow, bool animate, bool lock) {
    // if (mToastRunning && level <= mToastLevel)return;

    if (!keepNow) {
        if (mToastList.size())
            std::queue<TOAST_TYPE>().swap(mToastList);
        if (mToastRunning)
            mToastRunning = false;
    }

    TOAST_TYPE toast;
    toast.text = text;
    toast.level = level;
    toast.animate = animate;
    toast.lock = lock;
    mToastList.push(toast);
    LOGI("toast list add %s", text.c_str());
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
    mToast->setAlpha(0.f);
    mToast->setVisibility(View::GONE);
    removeCallbacks(mToastRun);
    std::queue<TOAST_TYPE>().swap(mToastList);
}

/// @brief 隐藏全部元素
void BaseWindow::hideAll() {
    mPopBox->setVisibility(GONE);
    hideToast();
    mBlackView->setVisibility(GONE);
}

bool BaseWindow::selfKey(uint16_t keyCode, uint8_t status) {
    return false;
}

/// @brief Toast刷新器
void BaseWindow::toastTick() {
    if (!mToastRunning && mToastList.size()) {
        TOAST_TYPE item = mToastList.front();
        mToastList.pop();

        LOGI("showToast[over:%d]: %s", mToastList.size(), item.text.c_str());
        removeCallbacks(mToastRun);

        mToastRunning = true;
        mToastLevel = item.level;
        mToast->setText(item.text);
        mToast->setVisibility(View::VISIBLE);
        mToast->animate().cancel();

        if (item.animate) {
            mToast->setAlpha(0.f);
            mToast->animate().alpha(0.9f).setDuration(POPTEXT_ANIMATETIME).start();
        } else {
            mToast->setAlpha(0.9f);
        }
        if (!item.lock)postDelayed(mToastRun, 5000);
    }
}

/// @brief 按键灯更新
void BaseWindow::btnLightTick() {
    // uint8_t btnLight[ALL_BTN_COUNT] = { 0 };

    // if (mPop)mPop->callCheckLight(btnLight, btnLight + LEFT_BTN_COUNT);
    // else if (mPage)mPage->callCheckLight(btnLight, btnLight + LEFT_BTN_COUNT);

    // g_btnMgr->setLight(btnLight);
}
