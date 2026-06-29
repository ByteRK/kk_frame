/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2026-06-29 16:15:56
 * @FilePath: /kk_frame/src/app/managers/wind_mgr.cc
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#define AUTO_CLOSE true
#define CLEAR_POP_ON_PAGE_SWITCH true
#define RELOAD_SAME_ID_ON_SHOW true

#include "wind_mgr.h"
#include "global_data.h"
#include <core/app.h>
#include <core/inputeventsource.h>

WindMgr::WindMgr() { }

WindMgr::~WindMgr() {
    if (mLooper) mLooper->removeMessages(this);
    recyclePop(POP_NULL);
    recyclePage(PAGE_NULL);
    __delete(mWindow);
}

/// @brief 获取主窗口
/// @return 主窗口指针
MainWindow * WindMgr::getWindow() {
    return mWindow;
}

/// @brief 初始化
void WindMgr::init() {
    mLooper = Looper::getMainLooper();
    mWindow = MainWindow::instance();
    mWindow->init();

    mAutoRecyclePageMsg.what = MSG_AUTO_RECYCLE_PAGE;
    mAutoRecyclePopMsg.what = MSG_AUTO_RECYCLE_POP;
    mInitTime = SystemClock::uptimeMillis();

    // 显示LOGO
    mWindow->showLogo();

    // 根据模式跳转页面
    switch (g_data->mDeviceMode) {
    case DEVICE_MODE_DEMO: {
        LOGI("[TIP] now in demo mode");
        replacePage(PAGE_DEMO);
    }   break;
    case DEVICE_MODE_TEST: {
        LOGI("[TIP] now in test mode");
        replacePage(g_data->mTestPage);
    }   break;
    default: {
        replacePage(PAGE_HOME);
    }   break;
    }
}

/// @brief 显示指定页面
/// @param page 页面ID
/// @param initData 初始化数据
/// @return 是否创建成功
bool WindMgr::showPage(int8_t page, const LoadBase* initData) {
    if (page == PAGE_NULL) return false;

#if CLEAR_POP_ON_PAGE_SWITCH
    // 页面切换时清理弹窗层
    clearPop();
#endif

    // 判断是否允许跳转
    switch (checkCanShowPage(page)) {
    case P_JUMP_DISABLE: { // 不允许跳转
        return false;
    }   break;
    case P_JUMP_SAME: { // 相同页面，直接Load
#if RELOAD_SAME_ID_ON_SHOW
        mWindow->getPage()->callLoad(initData);
        return true;
#endif
    }   break;
    default: break;
    }

    // 先确保目标页面可用，再保存当前页面历史，避免跳转失败污染历史。
    if (!ensurePageCached(page)) return false;

    HistoryNode node;
    bool hasHistory = makeCurrentPageHistory(&node);

    if (switchPage(page, initData, nullptr)) {
        if (hasHistory) pushPageHistory(std::move(node));
        return true;
    }

    return false;
}

/// @brief 替换当前页面，不写入页面历史
/// @param page 页面ID
/// @param initData 初始化数据
/// @return 是否替换成功
bool WindMgr::replacePage(int8_t page, const LoadBase* initData) {
    if (page == PAGE_NULL) return false;

#if CLEAR_POP_ON_PAGE_SWITCH
    // 页面切换时清理弹窗层
    clearPop();
#endif

    switch (checkCanShowPage(page)) {
    case P_JUMP_DISABLE: {
        return false;
    }   break;
    case P_JUMP_SAME: {
        mWindow->getPage()->callLoad(initData);
    }   return true;
    default: break;
    }

    return switchPage(page, initData, nullptr);
}

/// @brief 回收指定页面
/// @param page 页面指针
void WindMgr::recyclePage(PageBase* page) {
    if (page == nullptr) return;
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
bool WindMgr::showPop(int8_t pop, const LoadBase* initData) {
    if (pop == POP_NULL) return false;

    // 判断是否允许跳转
    switch (checkCanShowPop(pop)) {
    case P_JUMP_DISABLE: { // 不允许跳转
        return false;
    }   break;
    case P_JUMP_SAME: { // 相同页面，直接Load
#if RELOAD_SAME_ID_ON_SHOW
        mWindow->getPop()->callLoad(initData);
        return true;
#endif
    }   break;
    default: break;
    }

    // 先确保目标弹窗可用，再保存当前弹窗历史。
    if (!ensurePopCached(pop)) return false;

    HistoryNode node;
    bool hasHistory = makeCurrentPopHistory(&node);

    if (switchPop(pop, initData, nullptr)) {
        if (hasHistory) pushPopHistory(std::move(node));
        return true;
    }

    return false;
}

/// @brief 替换当前弹窗，不写入弹窗历史
/// @param pop 弹窗ID
/// @param initData 初始化数据
/// @return 是否替换成功
bool WindMgr::replacePop(int8_t pop, const LoadBase* initData) {
    if (pop == POP_NULL) return false;

    switch (checkCanShowPop(pop)) {
    case P_JUMP_DISABLE: {
        return false;
    }   break;
    case P_JUMP_SAME: {
        mWindow->getPop()->callLoad(initData);
    }   return true;
    default: break;
    }

    return switchPop(pop, initData, nullptr);
}

/// @brief 回收指定弹窗
/// @param pop 弹窗指针
void WindMgr::recyclePop(PopBase* pop) {
    if (pop == nullptr)return;
    if (mPopCache.size() == 0)return;
    auto it = std::find_if(mPopCache.begin(), mPopCache.end(), \
        [pop](const std::pair<int, PopBase*>& pair) { return pair.second == pop; });
    if (it != mPopCache.end()) {
        int type = it->first;
        if (type == mWindow->getPopType())mWindow->removePop();
        mPopCache.erase(it);
        __delete(pop);
        LOGW("close pop: %d <- %p | pop count=%d", type, pop, mPopCache.size());
        return;
    }
    LOGE("close pop but not found: %p | pop count=%d", pop, mPopCache.size());
}

/// @brief 回收指定弹窗
/// @param pop 弹窗ID
/// @note pop传入POP_NULL时，会回收所有弹窗
void WindMgr::recyclePop(int8_t pop) {
    if (mPopCache.size() == 0)return;

    // 回收所有弹窗
    if (pop == POP_NULL) {
        if (mWindow) mWindow->removePop();
        mPopHistory.clear();
        std::unordered_map<int8_t, PopBase*> swapMap;
        mPopCache.swap(swapMap);
        for (auto& it : swapMap) {
            LOGW("close pop: %d <- %p | pop count=%d ", it.second->getType(), it.second, swapMap.size());
            if (it.second) delete it.second;
        }
        return;
    }

    // 回收指定弹窗
    auto it = mPopCache.find(pop);
    if (it != mPopCache.end()) {
        PopBase* ptr = it->second;
        if (pop == mWindow->getPopType())mWindow->removePop();
        mPopCache.erase(it);
        __delete(ptr);
        LOGW("close pop: %d <- %p | pop count=%d ", pop, ptr, mPopCache.size());
        return;
    }
    LOGE("close pop but not found: %d | pop count=%d", pop, mPopCache.size());
}

/// @brief 清空弹窗层
void WindMgr::clearPop() {
    if (mWindow) mWindow->removePop();
    mPopHistory.clear();
    postAutoRecycle(false);
}

/// @brief 返回到首页
/// @param withBundle 是否携带状态包 
void WindMgr::goToHome(bool withBundle) {
    if (mWindow->getPageType() == PAGE_HOME) {
        clearPop();
        mPageHistory.clear();
        return;
    }
    LOGI("Go to home, clear page history");

    // 判断能否跳转
    if (replacePage(PAGE_HOME, nullptr)) {
        if (withBundle) {
            // 寻找首页历史记录
            auto it = std::find_if(mPageHistory.begin(), mPageHistory.end(),
                [](const HistoryNode& pair) {
                return pair.first == PAGE_HOME;
            });
            if (it != mPageHistory.end()) {
                mWindow->getPage()->callRestore(it->second.get());
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

    // 获取最后一条记录的所有权
    auto node = std::move(mPageHistory.back());
    mPageHistory.pop_back();
    LOGI("go to page back: %d", node.first);

    // 判断能否跳转
    if (switchPage(node.first, nullptr, node.second.get())) {
        LOGI("go to page back success: %d", node.first);
    } else {
        // 跳转失败，重新加入历史记录
        LOGE("goTo page failed: %d", node.first);
        mPageHistory.push_back(std::move(node));
    }
}

/// @brief 返回到上一个弹窗
void WindMgr::goToPopBack() {
    if (mPopHistory.empty()) { // 没有历史记录
        LOGW("no pop history, clear pop");
        clearPop();
        return;
    }

    // 获取最后一条记录的所有权
    auto node = std::move(mPopHistory.back());
    mPopHistory.pop_back();

    // 判断能否跳转
    if (switchPop(node.first, nullptr, node.second.get())) {
        LOGI("go to pop back success: %d", node.first);
    } else {
        // 跳转失败，重新加入历史记录
        LOGE("goTo pop failed: %d", node.first);
        mPopHistory.push_back(std::move(node));
    }
}

/// @brief 从历史记录抹去指定页面
/// @param page 
void WindMgr::removePageHistory(int8_t page) {
    if (page == PAGE_NULL) {
        mPageHistory.clear();
        LOGI("clear page history");
        return;
    }

    size_t oldSize = mPageHistory.size();
    mPageHistory.erase(
        std::remove_if(mPageHistory.begin(), mPageHistory.end(),
            [page](const HistoryNode& pair) { return pair.first == page; }),
        mPageHistory.end()
    );
    if (mPageHistory.size() != oldSize) {
        LOGI("remove page history: %d, count=%d", page, oldSize - mPageHistory.size());
    }
}

/// @brief 从历史记录抹去指定弹窗
/// @param pop 
void WindMgr::removePopHistory(int8_t pop) {
    if (pop == POP_NULL) {
        mPopHistory.clear();
        LOGI("clear pop history");
        return;
    }

    size_t oldSize = mPopHistory.size();
    mPopHistory.erase(
        std::remove_if(mPopHistory.begin(), mPopHistory.end(),
            [pop](const HistoryNode& pair) { return pair.first == pop; }),
        mPopHistory.end()
    );
    if (mPopHistory.size() != oldSize) {
        LOGI("remove pop history: %d, count=%d", pop, oldSize - mPopHistory.size());
    }
}

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

/// @brief 新建页面
/// @param page 页面ID
/// @return 是否创建成功
bool WindMgr::createPage(int8_t page) {
    PageBase* pb = PageCreator::get(page); // 调用构建器
    if (!pb) {
        LOGE("can not create page: %d", page);
        return false;
    }
    if (page != pb->getType()) { // 防呆
        std::string msg = "page[" + std::to_string(page) + "] type error";
        throw std::runtime_error(msg.c_str());
    }
    mPageCache[page] = pb;
    LOGW("add new page: %d <- %p | page count=%d ", page, pb, mPageCache.size());
    return true;
}

/// @brief 确保页面已经在缓存中
/// @param page 页面ID
/// @return 是否存在或创建成功
bool WindMgr::ensurePageCached(int8_t page) {
    auto it = mPageCache.find(page);
    return it != mPageCache.end() || createPage(page);
}

/// @brief 切换到指定页面，可选择恢复历史状态
/// @param page 页面ID
/// @param initData 初始化数据
/// @param restoreData 恢复状态数据
/// @return 是否切换成功
bool WindMgr::switchPage(int8_t page, const LoadBase* initData, const SaveBase* restoreData) {
    if (!ensurePageCached(page)) return false;

    postAutoRecycle(true);

    if (mWindow->showPage(mPageCache[page], initData) == page) {
        if (restoreData) {
            mWindow->getPage()->callRestore(restoreData);
            LOGI("restore state for page: %d", page);
        }
        LOGI("show page: %d <- %p", page, mPageCache[page]);
        return true;
    }

    LOGE("show page: %d x %p", page, mPageCache[page]);
    return false;
}

/// @brief 保存当前页面状态为历史节点
/// @param node 输出历史节点
/// @return 是否生成历史节点
bool WindMgr::makeCurrentPageHistory(HistoryNode* node) {
    int8_t page = mWindow->getPageType();
    if (page == PAGE_NULL) return false;

    std::unique_ptr<SaveBase> saved(mWindow->getPage()->callSave());
    *node = std::make_pair(page, std::move(saved));
    return true;
}

/// @brief 推入页面历史
/// @param node 历史节点
void WindMgr::pushPageHistory(HistoryNode&& node) {
    constexpr size_t MAX_HISTORY = 20;

    mPageHistory.push_back(std::move(node));
    while (mPageHistory.size() > MAX_HISTORY) mPageHistory.erase(mPageHistory.begin());

    LOGI("push page[%d] to history, new history size: %d ",
        mPageHistory.back().first, mPageHistory.size());
}

/// @brief 检查是否允许页面跳转
/// @param newPage 新页面类型
/// @return 跳转类型
WindMgr::P_JUMP_TYPE WindMgr::checkCanShowPage(int8_t newPage) {
    int8_t nowPage = mWindow->getPageType();
    if (nowPage == newPage) return P_JUMP_SAME; // 前后相同
    return P_JUMP_ENABLE;
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
    PopBase* pb = PopCreator::get(pop); // 调用构建器
    if (!pb) {
        LOGE("can not create pop: %d", pop);
        return false;
    }
    if (pop != pb->getType()) { // 防呆
        std::string msg = "pop[" + std::to_string(pop) + "] type error";
        throw std::runtime_error(msg.c_str());
    }
    mPopCache[pop] = pb;
    LOGW("add new pop: %d <- %p | pop count=%d ", pop, pb, mPopCache.size());
    return true;
}

/// @brief 确保弹窗已经在缓存中
/// @param pop 弹窗ID
/// @return 是否存在或创建成功
bool WindMgr::ensurePopCached(int8_t pop) {
    auto it = mPopCache.find(pop);
    return it != mPopCache.end() || createPop(pop);
}

/// @brief 切换到指定弹窗，可选择恢复历史状态
/// @param pop 弹窗ID
/// @param initData 初始化数据
/// @param restoreData 恢复状态数据
/// @return 是否切换成功
bool WindMgr::switchPop(int8_t pop, const LoadBase* initData, const SaveBase* restoreData) {
    if (!ensurePopCached(pop)) return false;

    postAutoRecycle(false);

    if (mWindow->showPop(mPopCache[pop], initData) == pop) {
        if (restoreData) {
            mWindow->getPop()->callRestore(restoreData);
            LOGI("restore state for pop: %d", pop);
        }
        LOGI("show pop: %d <- %p", pop, mPopCache[pop]);
        return true;
    }

    LOGE("show pop: %d x %p", pop, mPopCache[pop]);
    return false;
}

/// @brief 保存当前弹窗状态为历史节点
/// @param node 输出历史节点
/// @return 是否生成历史节点
bool WindMgr::makeCurrentPopHistory(HistoryNode* node) {
    int8_t pop = mWindow->getPopType();
    if (pop == POP_NULL) return false;

    std::unique_ptr<SaveBase> saved(mWindow->getPop()->callSave());
    *node = std::make_pair(pop, std::move(saved));
    return true;
}

/// @brief 推入弹窗历史
/// @param node 历史节点
void WindMgr::pushPopHistory(HistoryNode&& node) {
    constexpr size_t MAX_HISTORY = 20;

    mPopHistory.push_back(std::move(node));
    while (mPopHistory.size() > MAX_HISTORY) mPopHistory.erase(mPopHistory.begin());

    LOGI("push pop[%d] to history, new history size: %d ",
        mPopHistory.back().first, mPopHistory.size());
}

/// @brief 检查是否允许弹窗跳转
/// @param newPop 新弹窗类型
/// @return 跳转类型
WindMgr::P_JUMP_TYPE WindMgr::checkCanShowPop(int8_t newPop) {
    int8_t nowPop = mWindow->getPopType();
    if (nowPop > newPop) return P_JUMP_DISABLE;
    if (nowPop == newPop) return P_JUMP_SAME; // 前后相同
    return P_JUMP_ENABLE;
}

/// @brief 检查自动回收
/// @param isPage 是否为PAGE
void WindMgr::postAutoRecycle(bool isPage) {
#if AUTO_CLOSE
    if (!mLooper) return;
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
