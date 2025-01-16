/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-01-17 01:21:03
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
#include <core/inputeventsource.h>

 /******* 页面头文件列表开始 *******/
#include "page_standBy.h"
/******* 页面头文件列表结束 *******/

CWindMgr::CWindMgr() {
}

CWindMgr::~CWindMgr() {
    mLooper->removeMessages(this);
    closeAll(true);
    __delete(mWindow);
}

/// @brief 消息处理
/// @param message 
void CWindMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_AUTO_CLOSE: {
        int8_t nowPage = mWindow->getPageType();
        for (auto it = mPageList.begin(); it != mPageList.end(); ) {
            if (it->second->getType() != nowPage) {
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

    mAutoCloseMsg.what = MSG_AUTO_CLOSE;
    mInitTime = SystemClock::uptimeMillis();

#if OPEN_SCREENSAVER
    InputEventSource::getInstance().setScreenSaver(
        std::bind(&CWindMgr::screenSaver, this, std::placeholders::_1),
        20
    );
#endif
    goTo(PAGE_STANDBY);
}

/// @brief 新增页面 (页面构造自动触发,请勿主动调用)
/// @param page 页面指针
void CWindMgr::add(PageBase* page) {
#if OPEN_SCREENSAVER
    refreshScreenSaver();
#endif
    mPageList.insert(std::make_pair(page->getType(), page));
    LOGW("add new page: %d <- %p | page count=%d ", page->getType(), page, mPageList.size());
}

/// @brief 关闭指定页面
/// @param page 页面指针
void CWindMgr::close(PageBase* page) {
    if (mPageList.size() == 0)return;
    auto it = std::find_if(mPageList.begin(), mPageList.end(), \
        [page](const std::pair<int, PageBase*>& pair) { return pair.second == page; });
    if (it != mPageList.end()) {
        int type = it->first;
        if (type == mWindow->getPageType())mWindow->removePage();
        mPageList.erase(it);
        __delete(page);
        LOGW("close page: %d <- %p | page count=%d", type, page, mPageList.size());
        return;
    }
    LOGE("close page but not found: %p | page count=%d", page, mPageList.size());
}

/// @brief 关闭指定页面
/// @param page 页面ID
void CWindMgr::close(int page) {
    if (mPageList.size() == 0)return;
    auto it = mPageList.find(page);
    if (it != mPageList.end()) {
        void* ptr = it->second;
        if (page == mWindow->getPageType())mWindow->removePage();
        mPageList.erase(it);
        __delete(it->second);
        LOGW("close page: %d <- %p | page count=%d ", page, ptr, mPageList.size());
        return;
    }
    LOGE("close page but not found: %d | page count=%d", page, mPageList.size());
}

/// @brief 关闭所有页面
/// @param withPop 是否包含弹窗
void CWindMgr::closeAll(bool withPop) {
    mWindow->removePage();
    if (withPop)mWindow->removePop();
    std::vector<PageBase*> vec;
    for (const auto& pair : mPageList)vec.push_back(pair.second);
    mPageList.clear();
    for (auto it : vec) {
        LOGW("close page: %d <- %p | page count=%d ", it->getType(), it, mPageList.size());
        __delete(it);
    }
}

/// @brief 前往指定页面 (新建页面必须从此处调用)
/// @param page 页面ID
/// @param showBlack 跳转后是否息屏
void CWindMgr::goTo(int page, bool showBlack) {
    mWindow->removePop();
    if (page == PAGE_NULL) {
        mWindow->removePage();
        if (showBlack)mWindow->showBlack();
        return;
    }
    if (page == mWindow->getPageType()) {
        mWindow->getPage()->callReload();
        if (showBlack)mWindow->showBlack();
        return;
    }
#if AUTO_CLOSE
    mLooper->removeMessages(this);
    mLooper->sendMessageDelayed(1000, this, mAutoCloseMsg);
#endif
    auto it = mPageList.find(page);
    if (it == mPageList.end() && !createPage(page)) {
        if (showBlack)mWindow->showBlack();
        return;
    }
    mWindow->showPage(mPageList[page]);
    if (showBlack)mWindow->showBlack();
    LOGI("show page: %d <- %p", page, mPageList[page]);
}

/// @brief 向指定页面发送消息
/// @param page 页面ID
/// @param type 消息类型
/// @param data 消息数据
void CWindMgr::sendMsg(int page, int type, void* data) {
    auto it = mPageList.find(page);
    if (it != mPageList.end()) {
        it->second->callMsg(type, data);
    }
}

/// @brief 显示弹窗
/// @param type 弹窗ID
/// @return 是否显示成功
bool CWindMgr::showPop(int8_t type) {
    if (type == mWindow->getPopType())return true;
    if (type < mWindow->getPopType())return false;
    PopBase* pop = nullptr;
    switch (type) {
    case POP_TIP: {
        // pop = new PopTip(mWindow->getContext());
    }   break;
    case POP_LOCK: {
        // pop = new PopLock(mWindow->getContext());
    }   break;
    default:
        LOGE("can not show pop: %d", type);
        break;
    }
    if (pop)mWindow->showPop(pop);
    return pop != nullptr;
}

/// @brief 向指定弹窗发送消息
/// @param pop 弹窗ID
/// @param type 消息类型
/// @param data 消息数据
void CWindMgr::sendPopMsg(int pop, int8_t type, void* data) {
    if (pop && pop == mWindow->getPopType()) {
        mWindow->getPop()->callMsg(type, data);
    }
}

/// @brief 新建页面
/// @param page 页面ID
/// @return 是否创建成功
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
/// @param lock 解锁/上锁
void CWindMgr::screenSaver(bool lock) {
    LOGE("CWindMgr::screenSaver = %d", lock);
    if (lock) {
    } else {
    }
    InputEventSource::getInstance().closeScreenSaver();
}
