/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-08-23 18:25:20
 * @FilePath: /kk_frame/src/windows/wind_mgr.cc
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
**/

#define AUTO_CLOSE true
#define OPEN_SCREENSAVER false

#include "wind_mgr.h"
#if ENABLE_THREAD_SAFE_MSG
#include <core/app.h>
#endif
#include <core/inputeventsource.h>

/******* 页面头文件列表开始 *******/
#include "page_home.h"
/******* 页面头文件列表结束 *******/

/******* 弹窗头文件列表开始 *******/
#include "pop_lock.h"
#include "pop_tip.h"
/******* 弹窗头文件列表结束 *******/

CWindMgr::CWindMgr() {
    // 初始化页面创建映射
    mPageFactory[PAGE_HOME] = []() { return new HomePage(); };
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
        size_t originalSize = mPageCache.size(); // 存储原始大小
        for (auto it = mPageCache.begin(); it != mPageCache.end(); ) {
            if (it->second->getType() != nowPage) {
                LOGW("close page: %d <- %p | page count=%d", it->first, it->second, --originalSize);
                __delete(it->second); // 手动删除
                it = mPageCache.erase(it); // 删除并移动迭代器
            } else {
                ++it; // 仅在未删除时移动迭代器
            }
        }
    }   break;
    case MSG_AUTO_CLOSE_POP: {
        int8_t nowPop = mWindow->getPopType();
        size_t originalSize = mPopCache.size(); // 存储原始大小
        for (auto it = mPopCache.begin(); it != mPopCache.end(); ) {
            if (it->second->getType() != nowPop) {
                LOGW("close pop: %d <- %p | pop count=%d", it->first, it->second, --originalSize);
                __delete(it->second); // 手动删除
                it = mPopCache.erase(it); // 删除并移动迭代器
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
    goTo(PAGE_HOME);
}

/// @brief 关闭指定页面
/// @param page 页面指针
void CWindMgr::close(PageBase* page) {
    if (mPageCache.size() == 0)return;
    auto it = std::find_if(mPageCache.begin(), mPageCache.end(), \
        [page](const std::pair<int, PageBase*>& pair) { return pair.second == page; });
    if (it != mPageCache.end()) {
        int type = it->first;
        if (type == mWindow->getPageType())mWindow->removePage();
        mPageCache.erase(it);
        __delete(page);
        LOGW("close page: %d <- %p | page count=%d", type, page, mPageCache.size());
        return;
    }
    LOGE("close page but not found: %p | page count=%d", page, mPageCache.size());
}

/// @brief 关闭指定页面
/// @param page 页面ID
void CWindMgr::close(int page) {
    if (mPageCache.size() == 0)return;
    auto it = mPageCache.find(page);
    if (it != mPageCache.end()) {
        PageBase* ptr = it->second;
        if (page == mWindow->getPageType())mWindow->removePage();
        mPageCache.erase(it);
        __delete(ptr);
        LOGW("close page: %d <- %p | page count=%d ", page, ptr, mPageCache.size());
        return;
    }
    LOGE("close page but not found: %d | page count=%d", page, mPageCache.size());
}

/// @brief 关闭所有页面
/// @param withPop 是否包含弹窗
void CWindMgr::closeAll(bool withPop) {
    mWindow->removePage();
    if (withPop)mWindow->removePop();
    std::vector<PageBase*> vec;
    for (const auto& pair : mPageCache)vec.push_back(pair.second);
    mPageCache.clear();
    for (auto it : vec) {
        LOGW("close page: %d <- %p | page count=%d ", it->getType(), it, mPageCache.size());
        __delete(it);
    }
}

/// @brief 前往指定页面 (新建页面必须从此处调用)
/// @param page 页面ID
/// @return 是否创建成功
bool CWindMgr::goTo(int page, Json::Value* baseData) {
    hidePop();
    if (page == PAGE_NULL) {
        mWindow->removePage();
        mPageHistory.clear();
        return true;
    }
    if (page == mWindow->getPageType()) {
        mWindow->getPage()->callReload();
        if (baseData)sendMsg(page, *baseData);
        return true;
    }
#if AUTO_CLOSE
    mLooper->removeMessages(this, MSG_AUTO_CLOSE_PAGE);
    mLooper->sendMessageDelayed(1000, this, mAutoClosePageMsg);
#endif
    auto it = mPageCache.find(page);
    if (it == mPageCache.end() && !createPage(page)) {
        return false;
    }

    // 如果当前页面不是空页面，则保存当前页面状态并记录历史记录
    if (mWindow->getPageType() != PAGE_NULL) {
        PBase::StateBundle bundle;
        mWindow->getPage()->callSaveState(bundle);
        mPageHistory.push_back(std::make_pair(mWindow->getPageType(), bundle));
        LOGI("push page[%d] to history, new history size: %d ", mWindow->getPageType(), mPageHistory.size());
    }

    // 重新调整页面历史记录
    auto pageIt = std::find_if(mPageHistory.begin(), mPageHistory.end(),
        [page](const std::pair<int, PBase::StateBundle>& pair) { return pair.first == page; });
    if (pageIt != mPageHistory.end()) {
        mPageHistory.erase(pageIt, mPageHistory.end()); // 删除该元素及后面的所有元素
        LOGI("Adjust page history, new history size: %d", mPageHistory.size());
    }

    // 显示新页面
    mWindow->showPage(mPageCache[page]);
    if (mWindow->getPageType() == page) {
        LOGI("show page: %d <- %p", page, mPageCache[page]);
        if (baseData)sendMsg(page, *baseData);
        return true;
    } else {
        LOGW("show page: %d x %p", page, mPageCache[page]);
        return false;
    }
}

/// @brief 向指定页面发送消息
/// @param page 页面ID
/// @param type 消息类型
/// @param data 消息数据
void CWindMgr::sendMsg(int page, const Json::Value& data, bool fromUiThread) {
#if ENABLE_THREAD_SAFE_MSG
    if (!fromUiThread) {
        std::lock_guard<std::mutex> lock(mPageMsgCacheMutex);
        mPageMsgCache.push(std::make_pair(page, std::move(Json::Value(data))));
        if (mPageMsgCache.size() >= MAX_THREAD_SAFE_MSG_SIZE) {
            mPageMsgCache.pop();
            LOGE("Message cache full, discarded oldest message");
        }
    } else {
        auto it = mPageCache.find(page);
        if (it != mPageCache.end()) {
            it->second->callMsg(data);
        }
    }
#else
    if (!fromUiThread) {
        LOGE("sendMsg from other thread, but not support");
        return;
    }
    auto it = mPageCache.find(page);
    if (it != mPageCache.end()) {
        it->second->callMsg(data);
    }
#endif
}

/// @brief 显示弹窗
/// @param type 弹窗ID
/// @return 是否显示成功
bool CWindMgr::showPop(int8_t pop, Json::Value* baseData) {
    uint8_t nowPop = mWindow->getPopType();
    if (pop < nowPop)return false;
    if (pop == nowPop) {
        mWindow->getPop()->callReload();
        if (baseData) sendMsg(pop, *baseData);
        return true;
    }
#if AUTO_CLOSE
    mLooper->removeMessages(this, MSG_AUTO_CLOSE_POP);
    mLooper->sendMessageDelayed(1000, this, mAutoClosePopMsg);
#endif
    auto it = mPopCache.find(pop);
    if (it == mPopCache.end() && !createPop(pop)) {
        return false;
    }

    // 如果当前弹窗不是空弹窗，则保存当前弹窗状态并记录历史记录
    if (mWindow->getPopType() != POP_NULL) {
        PBase::StateBundle bundle;
        mWindow->getPop()->callSaveState(bundle);
        mPopHistory.push_back(std::make_pair(mWindow->getPopType(), bundle));
        LOGI("push pop[%d] to history, new history size: %d ", mWindow->getPopType(), mPopHistory.size());
    }

    // 重新调整弹窗历史记录
    auto popIt = std::find_if(mPopHistory.begin(), mPopHistory.end(),
        [pop](const std::pair<int, PBase::StateBundle>& pair) { return pair.first == pop; });
    if (popIt != mPopHistory.end()) {
        mPopHistory.erase(popIt, mPopHistory.end()); // 删除该元素及后面的所有元素
        LOGI("Adjust pop history, new history size: %d", mPopHistory.size());
    }

    // 显示新弹窗
    mWindow->showPop(mPopCache[pop]);
    if (mWindow->getPopType() == pop) {
        LOGI("show pop: %d <- %p", pop, mPopCache[pop]);
        if (baseData) sendMsg(pop, *baseData);
        return true;
    } else {
        LOGW("show page: %d x %p", pop, mPopCache[pop]);
        return false;
    }
}

/// @brief 隐藏弹窗
void CWindMgr::hidePop() {
    if (mWindow->getPopType() != POP_NULL) {
        PBase::StateBundle bundle;
        mWindow->getPop()->callSaveState(bundle);
        mPopHistory.push_back(std::make_pair(mWindow->getPopType(), bundle));
    }
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
void CWindMgr::sendPopMsg(int pop, const Json::Value& data, bool fromUiThread) {
#if ENABLE_THREAD_SAFE_MSG
    if (!fromUiThread) {
        std::lock_guard<std::mutex> lock(mPopMsgCacheMutex);
        mPopMsgCache.push(std::make_pair(pop, std::move(Json::Value(data))));
        if (mPopMsgCache.size() >= MAX_THREAD_SAFE_MSG_SIZE) {
            mPopMsgCache.pop();
            LOGE("Message cache full, discarded oldest message");
        }
    } else {
        if (pop && pop == mWindow->getPopType()) {
            mWindow->getPop()->callMsg(data);
        }
    }
#else
    if (!fromUiThread) {
        LOGE("sendPopMsg from other thread, but not support");
        return;
    }
    if (pop && pop == mWindow->getPopType()) {
        mWindow->getPop()->callMsg(data);
    }
#endif
}

/// @brief 返回到首页
/// @param withBundle 是否携带状态包 
void CWindMgr::goToHome(bool withBundle) {
    if (mWindow->getPageType() == PAGE_HOME)return; // 防呆
    LOGI("Go to home, clear page history");

    PBase::StateBundle bundle;
    if (withBundle && getBundle(true, PAGE_HOME, bundle)) {
        if (goTo(PAGE_HOME) && mWindow->getPageType() == PAGE_HOME) {
            mWindow->getPage()->callRestoreState(bundle);
            LOGI("restore state for home page");
        } else {
            LOGE("goTo home page failed");
        }
        mPageHistory.clear();
        return;
    }
    goTo(PAGE_HOME);
    mPageHistory.clear();
}

/// @brief 返回到上一个页面
void CWindMgr::goToPageBack() {
    if (mPageHistory.empty()) {
        LOGW("no page history, go to home");
        goToHome(true);
        return;
    }
    auto last = mPageHistory.back();
    LOGI("go to page back: %d", last.first);
    if (goTo(last.first) && mWindow->getPageType() == last.first) {
        mWindow->getPage()->callRestoreState(last.second);
        LOGI("restore state for page: %d", last.first);
    } else {
        LOGE("goTo page failed: %d", last.first);
    }
}

/// @brief 返回到上一个弹窗
void CWindMgr::goToPopBack() {
    if (mPopHistory.empty()) {
        LOGW("no pop history, hide pop");
        hidePop();
        return;
    }
    auto last = mPopHistory.back();
    LOGI("go to pop back: %d", last.first);
    if (showPop(last.first) && mWindow->getPopType() == last.first) {
        mWindow->getPop()->callRestoreState(last.second);
        LOGI("restore state for pop: %d", last.first);
    } else {
        LOGE("goTo pop failed: %d", last.first);
    }
}

/// @brief 从历史记录抹去指定页面
/// @param page 
void CWindMgr::removePageHistory(int page) {
    auto it = std::find_if(mPageHistory.begin(), mPageHistory.end(),
        [page](const std::pair<int, PBase::StateBundle>& pair) { return pair.first == page; });
    if (it != mPageHistory.end()) {
        mPageHistory.erase(it);
        LOGI("remove page history: %d", page);
    }
}

/// @brief 从历史记录抹去指定弹窗
/// @param pop 
void CWindMgr::removePopHistory(int pop) {
    auto it = std::find_if(mPopHistory.begin(), mPopHistory.end(),
        [pop](const std::pair<int, PBase::StateBundle>& pair) { return pair.first == pop; });
    if (it != mPopHistory.end()) {
        mPopHistory.erase(it);
        LOGI("remove pop history: %d", pop);
    }
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
        mPageCache[page] = pb;
        LOGW("add new page: %d <- %p | page count=%d ", page, pb, mPageCache.size());
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
        mPopCache[pop] = pb;
        LOGW("add new pop: %d <- %p | pop count=%d ", pop, pb, mPopCache.size());
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

/// @brief 获取状态包
/// @param isPage 
/// @param id 
/// @param bundle 
/// @return 
bool CWindMgr::getBundle(bool isPage, int id, PBase::StateBundle& bundle) {
    std::vector<std::pair<int, PBase::StateBundle>>* history = isPage ? &mPageHistory : &mPopHistory;
    auto it = std::find_if(history->begin(), history->end(),
        [id](const std::pair<int, PBase::StateBundle>& pair) { return pair.first == id; });
    if (it != history->end()) {
        bundle = it->second;
        return true;
    }
    return false;
}