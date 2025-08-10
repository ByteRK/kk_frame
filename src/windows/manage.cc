/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-08-10 17:34:48
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
#if ENABLE_THREAD_SAFE_MSG
#include <core/app.h>
#endif
#include <core/inputeventsource.h>

 /******* 页面头文件列表开始 *******/
#include "page_standBy.h"
/******* 页面头文件列表结束 *******/

/******* 弹窗头文件列表开始 *******/
#include "pop_lock.h"
#include "pop_tip.h"
/******* 弹窗头文件列表结束 *******/

CWindMgr::CWindMgr() {
    // 初始化页面创建映射
    mPageFactory[PAGE_STANDBY] = []() { return new StandByPage(); };
    // 初始化弹窗创建映射
    mPopFactory[POP_LOCK] = []() { return new LockPop(); };
    mPopFactory[POP_TIP] = []() { return new TipPop(); };
}

CWindMgr::~CWindMgr() {
    mLooper->removeMessages(this);
    closeAll(true);
    __delete(mWindow);
}

#if ENABLE_THREAD_SAFE_MSG
int CWindMgr::checkEvents() {
    uint64_t now = SystemClock::uptimeMillis();
    if (now - mLastCheckMsgTime > CHECK_THREAD_SAFE_MSG_INTERVAL) {
        mLastCheckMsgTime = now;
        return 1;
    }
    return 0;
}

int CWindMgr::handleEvents() {
    std::queue<std::pair<uint8_t, Json::Value>> msgCache;
    // 检查页面消息
    {
        std::lock_guard<std::mutex> lock(mPageMsgCacheMutex);
        if (!mPageMsgCache.empty()) msgCache.swap(mPageMsgCache);
    }
    // 处理页面消息
    while (!msgCache.empty()) {
        auto& msg = msgCache.front();
        sendMsg(msg.first, msg.second, false);
        msgCache.pop();
    }
    // 检查弹窗消息
    {
        std::lock_guard<std::mutex> lock(mPopMsgCacheMutex);
        if (!mPopMsgCache.empty()) msgCache.swap(mPopMsgCache);
    }
    // 处理弹窗消息
    while (!msgCache.empty()) {
        auto& msg = msgCache.front();
        sendPopMsg(msg.first, msg.second, false);
        msgCache.pop();
    }
    return 0;
}
#endif

/// @brief 消息处理
/// @param message 
void CWindMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_AUTO_CLOSE_PAGE: {
        int8_t nowPage = mWindow->getPageType();
        size_t originalSize = mPageList.size(); // 存储原始大小
        for (auto it = mPageList.begin(); it != mPageList.end(); ) {
            if (it->second->getType() != nowPage) {
                LOGW("close page: %d <- %p | page count=%d", it->first, it->second, --originalSize);
                __delete(it->second); // 手动删除
                it = mPageList.erase(it); // 删除并移动迭代器
            } else {
                ++it; // 仅在未删除时移动迭代器
            }
        }
    }   break;
    case MSG_AUTO_CLOSE_POP: {
        int8_t nowPop = mWindow->getPopType();
        size_t originalSize = mPopList.size(); // 存储原始大小
        for (auto it = mPopList.begin(); it != mPopList.end(); ) {
            if (it->second->getType() != nowPop) {
                LOGW("close pop: %d <- %p | pop count=%d", it->first, it->second, --originalSize);
                __delete(it->second); // 手动删除
                it = mPopList.erase(it); // 删除并移动迭代器
            } else {
                ++it; // 仅在未删除时移动迭代器
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

    mAutoClosePageMsg.what = MSG_AUTO_CLOSE_PAGE;
    mAutoClosePopMsg.what = MSG_AUTO_CLOSE_POP;
    mInitTime = SystemClock::uptimeMillis();

#if ENABLE_THREAD_SAFE_MSG
    mLastCheckMsgTime = 0;
    App::getInstance().addEventHandler(this);
#endif

#if OPEN_SCREENSAVER
    InputEventSource::getInstance().setScreenSaver(
        std::bind(&CWindMgr::screenSaver, this, std::placeholders::_1),
        20
    );
#endif
    goTo(PAGE_STANDBY);
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
        PageBase* ptr = it->second;
        if (page == mWindow->getPageType())mWindow->removePage();
        mPageList.erase(it);
        __delete(ptr);
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
    mLooper->removeMessages(this, MSG_AUTO_CLOSE_PAGE);
    mLooper->sendMessageDelayed(1000, this, mAutoClosePageMsg);
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
void CWindMgr::sendMsg(int page, const Json::Value& data, bool fromOtherThread) {
#if ENABLE_THREAD_SAFE_MSG
    if (fromOtherThread) {
        std::lock_guard<std::mutex> lock(mPageMsgCacheMutex);
        mPageMsgCache.push(std::make_pair(page, std::move(Json::Value(data))));
    } else {
        auto it = mPageList.find(page);
        if (it != mPageList.end()) {
            it->second->callMsg(data);
        }
    }
#else
    if (fromOtherThread) {
        LOGE("sendMsg from other thread, but not support");
        return;
    }
    auto it = mPageList.find(page);
    if (it != mPageList.end()) {
        it->second->callMsg(data);
    }
#endif
}

/// @brief 显示弹窗
/// @param type 弹窗ID
/// @return 是否显示成功
bool CWindMgr::showPop(int8_t pop) {
    uint8_t nowPop = mWindow->getPopType();
    if (pop < nowPop)return false;
    if (pop == nowPop) {
        mWindow->getPop()->callReload();
        return true;
    }
#if AUTO_CLOSE
    mLooper->removeMessages(this, MSG_AUTO_CLOSE_POP);
    mLooper->sendMessageDelayed(1000, this, mAutoClosePopMsg);
#endif
    auto it = mPopList.find(pop);
    if (it == mPopList.end() && !createPop(pop)) {
        return false;
    }
    mWindow->showPop(mPopList[pop]);
    LOGI("show pop: %d <- %p", pop, mPopList[pop]);
    return true;
}

/// @brief 隐藏弹窗
void CWindMgr::hidePop() {
    mWindow->removePop();
#if AUTO_CLOSE
    mLooper->removeMessages(this, MSG_AUTO_CLOSE_POP);
    mLooper->sendMessageDelayed(1000, this, mAutoClosePopMsg);
#endif
}

/// @brief 向指定弹窗发送消息
/// @param pop 弹窗ID
/// @param type 消息类型
/// @param data 消息数据
void CWindMgr::sendPopMsg(int pop, const Json::Value& data, bool fromOtherThread) {
#if ENABLE_THREAD_SAFE_MSG
    if (fromOtherThread) {
        std::lock_guard<std::mutex> lock(mPopMsgCacheMutex);
        mPopMsgCache.push(std::make_pair(pop, std::move(Json::Value(data))));
    } else {
        if (pop && pop == mWindow->getPopType()) {
            mWindow->getPop()->callMsg(data);
        }
    }
#else
    if (fromOtherThread) {
        LOGE("sendPopMsg from other thread, but not support");
        return;
    }
    if (pop && pop == mWindow->getPopType()) {
        mWindow->getPop()->callMsg(data);
    }
#endif
}

/// @brief 新建页面
/// @param page 页面ID
/// @return 是否创建成功
bool CWindMgr::createPage(int page) {
    auto it = mPageFactory.find(page);
    if (it != mPageFactory.end()) {
        PageBase* pb = it->second(); // 调用构造函数创建页面
        if (page != pb->getType()) { // 防呆
            std::string msg = "page[" + std::to_string(page) + "] type error";
            throw std::runtime_error(msg.c_str());
        }
#if OPEN_SCREENSAVER
        refreshScreenSaver();
#endif
        mPageList[page] = pb;
        LOGW("add new page: %d <- %p | page count=%d ", page, pb, mPageList.size());
        return true;
    }
    LOGE("can not create page: %d", page);
    return false;
}

/// @brief 新建弹窗
/// @param pop 弹窗ID
/// @return 是否创建成功
bool CWindMgr::createPop(int pop) {
    auto it = mPopFactory.find(pop);
    if (it != mPopFactory.end()) {
        PopBase* pb = it->second(); // 调用构造函数创建页面
        if (pop != pb->getType()) { // 防呆
            std::string msg = "pop[" + std::to_string(pop) + "] type error";
            throw std::runtime_error(msg.c_str());
        }
#if OPEN_SCREENSAVER
        refreshScreenSaver();
#endif
        mPopList[pop] = pb;
        LOGW("add new pop: %d <- %p | pop count=%d ", pop, pb, mPopList.size());
        return true;
    }
    LOGE("can not create pop: %d", pop);
    return false;
}

/// @brief 屏幕保护（休眠）
/// @param lock 解锁/上锁
void CWindMgr::screenSaver(bool lock) {
    LOGV("CWindMgr::screenSaver = %d", lock);
    if (lock) {
    } else {
    }
    InputEventSource::getInstance().closeScreenSaver();
}
