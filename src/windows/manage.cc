/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2024-08-23 10:35:03
 * @FilePath: /kk_frame/src/windows/manage.cc
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#define AUTO_CLOSE true
#define OPEN_SCREENSAVER false


#include "manage.h"
#include "struct.h"
#include "global_data.h"
#include "conn_mgr.h"
#include "btn_mgr.h"
#include "config_mgr.h"
#include "this_func.h"
#include "hv_time_common.h"

#include <core/inputeventsource.h>

#include "page_standBy.h"

CWindMgr::CWindMgr() {
    mLockMsg.what = MSG_LOCK;
    mCloseMsg.what = MSG_AUTO_CLOSE;
    mInitTime = SystemClock::uptimeMillis();
}

CWindMgr::~CWindMgr() {
    mLooper->removeMessages(this);
    closeAll();
}

/// @brief 消息处理
/// @param message 
void CWindMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_LOCK:
        break;
    case MSG_AUTO_CLOSE: {
        int8_t nowPage = getNowPageType();
        for (auto it = mPageList.begin(); it != mPageList.end(); ) {
            if (it->second->getPageType() != nowPage) {
                __delete(it->second);
                LOGW("close page: %d <- %p | page count=%d", it->first, it->second, mPageList.size() - 1);
                it = mPageList.erase(it);
            } else {
                ++it;
            }
        }
    }   break;
    }
}

/// @brief 初始化
void CWindMgr::init() {
    mLooper = Looper::getMainLooper();
    mWindow = BaseWindow::ins();
    mWindow->init();
    mWindow->showLogo();
    mLastTouchTime = SystemClock::uptimeMillis();

#if OPEN_SCREENSAVER
    InputEventSource::getInstance().setScreenSaver(
        std::bind(&CWindMgr::screenSaver, this, std::placeholders::_1),
        20
    );
#endif

    goTo(PAGE_STANDBY);
}

/// @brief 新增PAGE (PAGE构造自动触发,请勿主动调用)
/// @param page 
/// @return 
void CWindMgr::add(PageBase* page) {
#if OPEN_SCREENSAVER
    refreshScreenSaver();
#endif
    mPageList.insert(std::make_pair(page->getPageType(), page));
    LOGW("add new page: %d <- %p | page count=%d ", page->getPageType(), page, mPageList.size());
}

/// @brief 关闭指定PAGE (建议只在PAGE内部调用)
/// @param win 
void CWindMgr::close(PageBase* page) {
    if (mPageList.size() == 0)return;
    auto it = std::find_if(mPageList.begin(), mPageList.end(), \
        [page](const std::pair<int, PageBase*>& pair) { return pair.second == page; });
    if (it != mPageList.end()) {
        int type = it->first;
        mPageList.erase(it);
        if (type == getNowPageType())mWindow->removePage();
        __delete(page);
        LOGW("close page: %d <- %p | page count=%d", type, page, mPageList.size());
        return;
    }
    LOGE("close page but not found: %p | page count=%d", page, mPageList.size());
}

/// @brief 关闭指定PAGE
/// @param page 
void CWindMgr::close(int page) {
    auto it = mPageList.find(page);
    if (it != mPageList.end()) {
        mPageList.erase(it);
        LOGW("close page: %d <- %p | page count=%d ", it->second->getPageType(), it->second, mPageList.size());
        __delete(it->second);
        return;
    }
    LOGE("close page but not found: %d | page count=%d", page, mPageList.size());
}

/// @brief 关闭所有页面
void CWindMgr::closeAll() {
    mWindow->removePage();
    std::vector<PageBase*> vec;
    for (const auto& pair : mPageList)vec.push_back(pair.second);
    mPageList.clear();
    for (auto it : vec) {
        LOGW("close page: %d <- %p | page count=%d ", it->getPageType(), it, mPageList.size());
        __delete(it);
    }
}

/// @brief 前往指定页面 (新建页面必须从此处调用)
/// @param id
void CWindMgr::goTo(int page, bool showBlack) {
    mWindow->removePop();
    if (page == getNowPageType()) {
        mWindow->getPage()->reload();
        if (showBlack)mWindow->showBlack();
        return;
    }
#if AUTO_CLOSE
    mLooper->removeMessages(this);
    mLooper->sendMessageDelayed(1000, this, mCloseMsg);
#endif
    if (page == PAGE_NULL) {
        mWindow->removePage();
        if (showBlack)mWindow->showBlack();
        return;
    }
    auto it = mPageList.find(page);
    if (it == mPageList.end() && !createPage(page)) {
        if (showBlack)mWindow->showBlack();
        return;
    }
    mWindow->showPage(mPageList[page]);
    if (showBlack)mWindow->showBlack();
    LOGI("show page: %d <- %p", page, mPageList[page]);
}

/// @brief 向其它窗口发送消息
/// @param page 
/// @param type 
/// @param data 
void CWindMgr::sendMsg(int page, int type, void* data) {
}

/// @brief 获取当前窗口类型
/// @return 
int8_t CWindMgr::getNowPageType() {
    return mWindow->getPageType();
}

/// @brief 获取当前窗口
/// @return 
PageBase* CWindMgr::getNowPage() {
    if (getNowPageType() == PAGE_NULL)return nullptr;
    return mWindow->getPage();
}

/// @brief 按键转发
/// @param keyCode 
/// @param status 
void CWindMgr::sendKey(uint16_t keyCode, uint8_t status) {
    bool res = mWindow->onKey(keyCode, status);
    // g_data->mBuzzer = res && status != HW_EVENT_UP;
    if (res) {
        static bool sendLong = false;
        switch (status) {
        case HW_EVENT_DOWN:
        case HW_EVENT_UP:
            sendLong = false;
            break;
        case HW_EVENT_LONG:
            sendLong = true;
            break;
        }
        mLastTouchTime = SystemClock::uptimeMillis();
    }
}

/// @brief 特殊按键处理
/// @param key 
void CWindMgr::specialKey(uint8_t key) {
}

/// @brief 获取最后一次触摸时间
/// @return 
uint64_t CWindMgr::getLastTouchTime() {
    return mLastTouchTime;
}

/// @brief 新建PAGE
/// @param page 
/// @return 
bool CWindMgr::createPage(int page) {
    switch (page) {
    case PAGE_STANDBY: {
        new StandByPage();
    }   return true;
    }
    LOGE("can not create page: %d", page);
    return false;
}

/// @brief 屏幕保护（休眠）
/// @param lock 
void CWindMgr::screenSaver(bool lock) {
    LOGE("CWindMgr::screenSaver  =  %d", lock);
    if (lock) {
    } else {
    }
    InputEventSource::getInstance().closeScreenSaver();
}
