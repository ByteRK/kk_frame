/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2026-01-05 11:21:37
 * @FilePath: /kk_frame/src/app/managers/wind_mgr.cc
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#define AUTO_CLOSE true
#define OPEN_SCREENSAVER false

#include "wind_mgr.h"
#include "global_data.h"
#include <core/app.h>
#include <core/inputeventsource.h>

WindMgr::WindMgr() {
}

WindMgr::~WindMgr() {
    mLooper->removeMessages(this);
    recyclePage(PAGE_NULL);
    __delete(mWindow);
}

#if ENABLE_THREAD_SAFE_MSG
int WindMgr::checkEvents() {
    uint64_t now = SystemClock::uptimeMillis();
    if (now - mLastCheckMsgTime > CHECK_THREAD_SAFE_MSG_INTERVAL) {
        mLastCheckMsgTime = now;
        return 1;
    }
    return 0;
}

int WindMgr::handleEvents() {
    dealOtherThreadMsg();
    return 0;
}
#endif

/// @brief 消息处理
/// @param message 
void WindMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_AUTO_RECYCLE_PAGE: {
        autoRecyclePage();
    }   break;
    case MSG_AUTO_RECYCLE_POP: {
        autoRecyclePop();
    }   break;
    }
}

/// @brief 初始化
void WindMgr::init() {
    mLooper = Looper::getMainLooper();
    mWindow = MainWindow::instance();
    mWindow->init();

    mAutoRecyclePageMsg.what = MSG_AUTO_RECYCLE_PAGE;
    mAutoRecyclePopMsg.what = MSG_AUTO_RECYCLE_POP;
    mInitTime = SystemClock::uptimeMillis();

#if ENABLE_THREAD_SAFE_MSG
    mLastCheckMsgTime = 0;
    App::getInstance().addEventHandler(this);
#endif

#if OPEN_SCREENSAVER
    InputEventSource::getInstance().setScreenSaver(
        std::bind(&WindMgr::screenSaver, this, std::placeholders::_1),
        20
    );
#endif

    // 根据模式跳转页面
    switch (g_data->mDeviceMode) {
    case DEVICE_MODE_DEMO: {
        LOGI("[TIP] now in demo mode");
        showPage(PAGE_DEMO);
    }   break;
    case DEVICE_MODE_TEST: {
        LOGI("[TIP] now in test mode");
        showPage(g_data->mTestPage);
    }   break;
    default: {
        showPage(PAGE_HOME);
    }   break;
    }
}

/// @brief 注册页面
/// @param page 
/// @param func 
void WindMgr::registerPage(int8_t page, std::function<PageBase* ()> func) {
    mPageFactory[page] = func;
}

/// @brief 注册弹窗
/// @param pop 
/// @param func 
void WindMgr::registerPop(int8_t pop, std::function<PopBase* ()> func) {
    mPopFactory[pop] = func;
}

/// @brief 显示指定页面 (新建页面必须从此处调用)
/// @param page 页面ID
/// @param initData 初始化数据
/// @return 是否创建成功
bool WindMgr::showPage(int8_t page, LoadMsgBase* initData, bool updateHistory) {
    if (page == PAGE_NULL) return false;

    // 隐藏弹窗
    hidePop();

    // 判断是否允许跳转
    switch (checkCanShowPage(page)) {
    case -1: { // 不允许跳转
        return false;
    }   break;
    case 1: { // 相同页面，直接Load
        mWindow->getPage()->callLoad(initData); return true;
    }   break;
    default: break;
    }

    // 开启延迟自动回收
    postAutoRecycle(true);

    // 检查页面是否成功构建
    if (!checkPCache(page, true))return false;

    // 保存当前页面状态并记录历史记录
    if (updateHistory) addToHistory(true);

    // 显示新页面
    if (mWindow->showPage(mPageCache[page], initData) == page) {
        LOGI("show page: %d <- %p", page, mPageCache[page]);
        return true;
    } else {
        LOGW("show page: %d x %p", page, mPageCache[page]);
        return false;
    }
}

/// @brief 向指定页面发送基础消息
/// @param page 页面ID
/// @param msg 消息类型
/// @param fromUiThread 是否来自UI线程
void WindMgr::sendPageMsg(int8_t page, MSG_TYPE msg, bool fromUiThread) {
    RunMsgBase msgBase;
    msgBase.msgType = msg;
    sendPageMsg(page, &msgBase, fromUiThread);
}

/// @brief 向指定页面发送消息
/// @param page 页面ID
/// @param msg 消息数据
/// @param fromUiThread 是否来自UI线程
void WindMgr::sendPageMsg(int8_t page, const RunMsgBase* msg, bool fromUiThread) {
    if (fromUiThread) {
        if (page == PAGE_NULL) {
            for (auto& it : mPageCache)
                it.second->callMsg(msg);
        } else {
            auto it = mPageCache.find(page);
            if (it != mPageCache.end()) {
                it->second->callMsg(msg);
            }
        }
    } else {
#if ENABLE_THREAD_SAFE_MSG
        std::lock_guard<std::mutex> lock(mPageMsgCacheMutex);
        std::unique_ptr<RunMsgBase> copy(msg->clone());
        mPageMsgCache.push(std::make_pair((uint8_t)page, std::move(copy)));
        if (mPageMsgCache.size() >= MAX_THREAD_SAFE_MSG_SIZE) {
            mPageMsgCache.pop();
            LOGE("Message cache full, discarded oldest message");
        }
#else
        LOGE("sendPageMsg from other thread, but not support");
#endif
    }
}

/// @brief 回收指定页面
/// @param page 页面指针
void WindMgr::recyclePage(PageBase* page) {
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

/// @brief 回收指定页面
/// @param page 页面ID
/// @note page传入PAGE_NULL时，会回收所有页面
void WindMgr::recyclePage(int8_t page) {
    if (mPageCache.size() == 0)return;

    // 回收所有页面
    if (page == PAGE_NULL) {
        mWindow->removePage();
        std::unordered_map<int8_t, PageBase*> swapMap;
        mPageCache.swap(swapMap);
        for (auto& it : swapMap) {
            LOGW("close page: %d <- %p | page count=%d ", it.second->getType(), it.second, swapMap.size());
            if (it.second) delete it.second;
        }
        return;
    }

    // 回收指定页面
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

/// @brief 显示弹窗
/// @param type 弹窗ID
/// @return 是否显示成功
bool WindMgr::showPop(int8_t pop, LoadMsgBase* initData, bool updateHistory) {
    // 判断是否允许跳转
    switch (checkCanShowPop(pop)) {
    case -1: { // 不允许跳转
        return false;
    }   break;
    case 1: { // 相同页面，直接Load
        mWindow->getPop()->callLoad(initData); return true;
    }   break;
    default: break;
    }

    // 开启延迟自动回收
    postAutoRecycle(false);

    // 检查弹窗是否成功构建
    if (!checkPCache(pop, false))return false;

    // 保存当前弹窗状态并记录历史记录
    if (updateHistory) addToHistory(false);

    // 显示新弹窗
    if (mWindow->showPop(mPopCache[pop], initData) == pop) {
        LOGI("show pop: %d <- %p", pop, mPopCache[pop]);
        return true;
    } else {
        LOGW("show pop: %d x %p", pop, mPopCache[pop]);
        return false;
    }
}

/// @brief 向指定弹窗发送基础消息
/// @param page 弹窗ID
/// @param msg 消息类型
/// @param fromUiThread 是否来自UI线程 
void WindMgr::sendPopMsg(int8_t page, MSG_TYPE msg, bool fromUiThread) {
    RunMsgBase msgBase;
    msgBase.msgType = msg;
    sendPopMsg(page, &msgBase, fromUiThread);
}

/// @brief 向指定弹窗发送消息
/// @param pop 弹窗ID
/// @param msg 消息数据
/// @param fromUiThread 是否来自UI线程
void WindMgr::sendPopMsg(int8_t pop, const RunMsgBase* msg, bool fromUiThread) {
    if (fromUiThread) {
        if (pop == POP_NULL) {
            mWindow->getPop()->callMsg(msg);
        } else if (pop == mWindow->getPopType()) {
            mWindow->getPop()->callMsg(msg);
        }
    } else {
#if ENABLE_THREAD_SAFE_MSG
        std::lock_guard<std::mutex> lock(mPopMsgCacheMutex);
        std::unique_ptr<RunMsgBase> copy(msg->clone());
        mPopMsgCache.push(std::make_pair((uint8_t)pop, std::move(copy)));
        if (mPopMsgCache.size() >= MAX_THREAD_SAFE_MSG_SIZE) {
            mPopMsgCache.pop();
            LOGE("Message cache full, discarded oldest message");
        }
#else
        LOGE("sendPopMsg from other thread, but not support");
#endif
    }
}

/// @brief 隐藏弹窗
void WindMgr::hidePop() {
    if (mWindow->getPopType() != POP_NULL) {
        auto saved = mWindow->getPop()->callSaveState();
        std::unique_ptr<SaveMsgBase> copy(nullptr);
        if (saved) copy.reset(saved->clone());

        mPopHistory.push_back(std::make_pair(mWindow->getPopType(), std::move(copy)));
    }
    mWindow->removePop();
    postAutoRecycle(false);
}

/// @brief 返回到首页
/// @param withBundle 是否携带状态包 
void WindMgr::goToHome(bool withBundle) {
    if (mWindow->getPageType() == PAGE_HOME)return; // 防呆
    LOGI("Go to home, clear page history");

    // 判断能否跳转
    if (showPage(PAGE_HOME, nullptr, false)) {
        if (withBundle) {
            // 寻找首页历史记录
            auto it = std::find_if(mPageHistory.begin(), mPageHistory.end(),
                [](const std::pair<int8_t, std::unique_ptr<SaveMsgBase>>& pair) {
                return pair.first == PAGE_HOME;
            });
            if (it != mPageHistory.end()) {
                mWindow->getPage()->callRestoreState(it->second.get());
                LOGI("restore state for home page");
            } else {
                LOGE("home page not has history");
            }
        }
    } else {
        LOGE("goTo home page failed");
        return;
    }
    mPageHistory.clear();
}

/// @brief 返回到上一个页面
void WindMgr::goToPageBack() {
    if (mPageHistory.empty()) { // 没有历史记录
        LOGW("no page history, go to home");
        goToHome(false);
        return;
    }

#if 1
    // 获取最后一条记录的所有权
    auto node = std::move(mPageHistory.back());
    mPageHistory.pop_back();
    LOGI("go to page back: %d", node.first);

    // 判断能否跳转
    if (showPage(node.first, nullptr, false)) {
        // 可以跳转，恢复状态
        LOGI("restore state for page: %d", node.first);
        mWindow->getPage()->callRestoreState(node.second.get());
    } else {
        // 跳转失败，重新加入历史记录
        LOGE("goTo page failed: %d", node.first);
        mPageHistory.push_back(std::move(node));
    }
#else
    // 安全地 move 出最后一条记录
    auto node = std::move(mPageHistory.back());
    mPageHistory.pop_back();
    int8_t pageId = node.first;
    auto& saved = node.second; // unique_ptr<SaveMsgBase>

    LOGI("go to page back: %d", pageId);

    // 确保目标页面存在（先创建）
    if (!checkPCache(pageId, true)) {
        LOGE("failed to create page on back: %d", pageId);
        // 恢复历史（把 node 放回）
        mPageHistory.push_back(std::move(node));
        goToHome(false);
        return;
    }

    PageBase* page = mPageCache[pageId];

    // 先恢复状态（在 show 之前恢复，避免闪烁）
    if (saved) page->callRestoreState(saved.get());

    // 然后显示页面
    if (mWindow->showPage(page, nullptr) == pageId) {
        LOGI("restore state for page: %d", pageId);
    } else {
        LOGE("goTo page failed: %d", pageId);
        // 若失败，把 node 放回历史
        mPageHistory.push_back(std::move(node));
    }
#endif
}

/// @brief 返回到上一个弹窗
void WindMgr::goToPopBack() {
    if (mPopHistory.empty()) { // 没有历史记录
        LOGW("no pop history, hide pop");
        hidePop();
        return;
    }

    // 获取最后一条记录的所有权
    auto node = std::move(mPopHistory.back());
    mPopHistory.pop_back();

    // 判断能否跳转
    if (showPop(node.first, nullptr, false)) {
        // 可以跳转，恢复状态
        LOGI("restore state for pop: %d", node.first);
        mWindow->getPop()->callRestoreState(node.second.get());
    } else {
        // 跳转失败，重新加入历史记录
        LOGE("goTo pop failed: %d", node.first);
        mPopHistory.push_back(std::move(node));
    }
}

/// @brief 从历史记录抹去指定页面
/// @param page 
void WindMgr::removePageHistory(int8_t page) {
    auto it = std::find_if(mPageHistory.begin(), mPageHistory.end(),
        [page](const std::pair<int8_t, std::unique_ptr<SaveMsgBase>>& pair) { return pair.first == page; });
    if (it != mPageHistory.end()) {
        mPageHistory.erase(it);
        LOGI("remove page history: %d", page);
    }
}

/// @brief 从历史记录抹去指定弹窗
/// @param pop 
void WindMgr::removePopHistory(int8_t pop) {
    auto it = std::find_if(mPopHistory.begin(), mPopHistory.end(),
        [pop](const std::pair<int8_t, std::unique_ptr<SaveMsgBase>>& pair) { return pair.first == pop; });
    if (it != mPopHistory.end()) {
        mPopHistory.erase(it);
        LOGI("remove pop history: %d", pop);
    }
}

/// @brief 新建页面
/// @param page 页面ID
/// @return 是否创建成功
bool WindMgr::createPage(int8_t page) {
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

/// @brief 检查是否允许页面跳转
/// @param newPage 新页面类型
/// @return -1:不允许 0:允许 1:前后相同
int8_t WindMgr::checkCanShowPage(int8_t newPage) {
    int8_t nowPage = mWindow->getPageType();
    if (nowPage == newPage) return 1; // 前后相同
    return 0;
}

/// @brief 自动回收页面
void WindMgr::autoRecyclePage() {
    int8_t nowPage = mWindow->getPageType();
    size_t originalSize = mPageCache.size(); // 存储原始大小
    for (auto it = mPageCache.begin(); it != mPageCache.end(); ) {
        if (it->second->getType() != nowPage && it->second->canAutoRecycle()) {
            LOGW("close page: %d <- %p | page count=%d", it->first, it->second, --originalSize);
            __delete(it->second); // 手动删除
            it = mPageCache.erase(it); // 删除并移动迭代器
        } else {
            ++it; // 仅在未删除时移动迭代器
        }
    }
}

/// @brief 新建弹窗
/// @param pop 弹窗ID
/// @return 是否创建成功
bool WindMgr::createPop(int8_t pop) {
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

/// @brief 检查是否允许弹窗跳转
/// @param newPop 新弹窗类型
/// @return -1:不允许 0:允许 1:前后相同
int8_t WindMgr::checkCanShowPop(int8_t newPop) {
    int8_t nowPop = mWindow->getPopType();
    if (nowPop > newPop) return -1;
    if (nowPop == newPop) return 1; // 前后相同
    return 0;
}

/// @brief 检查页面/弹窗缓存
/// @param pType 类型
/// @param isPage 是否为PAGE
/// @return 是否存在
bool WindMgr::checkPCache(int8_t pType, bool isPage) {
    if (isPage) {
        auto it = mPageCache.find(pType);
        if (it == mPageCache.end() && !createPage(pType))
            return false;
    } else {
        auto it = mPopCache.find(pType);
        if (it == mPopCache.end() && !createPop(pType))
            return false;
    }
    return true;
}

/// @brief 保存当前状态
/// @param isPage 是否为PAGE
void WindMgr::addToHistory(bool isPage) {
    // 检查当前类型
    int8_t pType = isPage ? mWindow->getPageType() : mWindow->getPopType();
    if (isPage ? // 空页面不保存
        pType == PAGE_NULL :
        pType == POP_NULL)
        return;

    // 调整历史记录
    adjustHistory(pType, isPage);

    // 获取历史记录
    auto history = isPage ? &mPageHistory : &mPopHistory;

    // 获取当前显示内容指针
    PBase* p = nullptr;
    if (isPage) p = mWindow->getPage();
    else p = mWindow->getPop();

    // 生成包含当前状态信息的智能指针
    auto saved = p->callSaveState();
    std::unique_ptr<SaveMsgBase> copy(nullptr);
    if (saved) copy.reset(saved->clone());

    // 加入历史记录
    history->push_back(std::make_pair(pType, std::move(copy)));

    constexpr size_t MAX_HISTORY = 20;
    if (history->size() >= MAX_HISTORY) history->erase(history->begin());

    LOGI("push %s[%d] to history, new history size: %d ",
        isPage ? "page" : "pop",
        pType, history->size());
}

/// @brief 调整历史记录
/// @param pType 类型
/// @param isPage 是否为PAGE
void WindMgr::adjustHistory(int8_t pType, bool isPage) {
    auto history = isPage ? &mPageHistory : &mPopHistory;
    auto pageIt = std::find_if(history->begin(), history->end(),
        [pType](const std::pair<int8_t, std::unique_ptr<SaveMsgBase>>& pair) { return pair.first == pType; });
    if (pageIt != history->end()) {
        history->erase(pageIt, history->end());
        LOGI("Adjust %s history, new history size: %d", isPage ? "page" : "pop", history->size());
    }
}

/// @brief 检查自动回收
/// @param isPage 是否为PAGE
void WindMgr::postAutoRecycle(bool isPage) {
#if AUTO_CLOSE
    mLooper->removeMessages(this,
        isPage ? MSG_AUTO_RECYCLE_PAGE : MSG_AUTO_RECYCLE_POP
    );
    mLooper->sendMessageDelayed(1000, this,
        isPage ? mAutoRecyclePageMsg : mAutoRecyclePopMsg
    );
#endif
}

/// @brief 自动回收弹窗
void WindMgr::autoRecyclePop() {
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
}

/// @brief 处理其它线程发送的消息
void WindMgr::dealOtherThreadMsg() {
#if ENABLE_THREAD_SAFE_MSG
    std::queue<std::pair<int8_t, std::unique_ptr<RunMsgBase>>> msgCache;
    // 检查页面消息
    {
        std::lock_guard<std::mutex> lock(mPageMsgCacheMutex);
        if (!mPageMsgCache.empty()) msgCache.swap(mPageMsgCache);
    }
    // 处理页面消息
    while (!msgCache.empty()) {
        auto& msg = msgCache.front();
        sendPageMsg(msg.first, msg.second.get(), false);
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
        sendPopMsg(msg.first, msg.second.get(), false);
        msgCache.pop();
    }
#endif
}

/// @brief 屏幕保护（休眠）
/// @param lock 解锁/上锁
void WindMgr::screenSaver(bool lock) {
    LOGV("WindMgr::screenSaver = %d", lock);
    if (lock) {
    } else {
    }
    InputEventSource::getInstance().closeScreenSaver();
}