/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-26 11:23:27
 * @FilePath: /hana_frame/src/windows/manage.cc
 * @Description: 页面管理类
 * Copyright (c) 2025 by hanakami, All Rights Reserved.
 */

#define AUTO_CLOSE true
#define OPEN_SCREENSAVER false

#include "manage.h"
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

/// @brief 消息处理
/// @param message 
void CWindMgr::handleMessage(Message& message) {
    switch (message.what) {
    case MSG_AUTO_CLOSE: {
        int8_t nowPage = mWindow->getPageType();
        size_t originalSize = mPageMap.size(); // 存储原始大小
        for (auto it = mPageMap.begin(); it != mPageMap.end(); ) {
            if (it->second->getType() != nowPage) {
                LOGW("close page: %d <- %p | page count=%d", it->first, it->second, --originalSize);
                __delete(it->second); // 手动删除
                it = mPageMap.erase(it); // 删除并移动迭代器
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

    mAutoCloseMsg.what = MSG_AUTO_CLOSE;
    mInitTime = SystemClock::uptimeMillis();
    isBackPage = false;
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
    if (mPageMap.size() == 0)return;
    auto it = std::find_if(mPageMap.begin(), mPageMap.end(), \
        [page](const std::pair<int, PageBase*>& pair) { return pair.second == page; });
    if (it != mPageMap.end()) {
        int type = it->first;
        if (type == mWindow->getPageType())mWindow->removePage();
        mPageMap.erase(it);
        auto itL = std::find(mPageList.begin(), mPageList.end(), mWindow->getPageType());
        if(itL != mPageList.end()){
            mPageList.erase(itL);
        }
        __delete(page);
        LOGW("close page: %d <- %p | page count=%d", type, page, mPageMap.size());
        return;
    }
    LOGE("close page but not found: %p | page count=%d", page, mPageMap.size());
}

/// @brief 关闭指定页面
/// @param page 页面ID
void CWindMgr::close(int page) {
    if (mPageMap.size() == 0)return;
    auto it = std::find(mPageList.begin(), mPageList.end(), page);
    if(it != mPageList.end()){
        mPageList.erase(it);
    }
    auto itM = mPageMap.find(page);
    if (itM != mPageMap.end()) {
        PageBase* ptr = itM->second;
        if (page == mWindow->getPageType())mWindow->removePage();
        mPageMap.erase(itM);
        __delete(ptr);
        LOGW("close page: %d <- %p | page count=%d ", page, ptr, mPageMap.size());
        return;
    }
    LOGE("close page but not found: %d | page count=%d", page, mPageMap.size());
}

void CWindMgr::backPage() {
    isBackPage = true;
    if (mPageList.size() >= 2) {
        int nowPage = mPageList[mPageList.size() - 1];
        int previousPage = mPageList[mPageList.size() - 2];
        mPageList.pop_back();
        LOGE("penultimate : %d",previousPage);
        if(previousPage == PAGE_HOME){
            mPageList.clear();
        }
        // 如还有其他节点，删除节点后的所有元素
        // else if(penultimate == PAGE_EDIT){
        //     auto it = mPageList.begin() + (mPageList.size() - 1); // 找到 EDIT 的位置
        //     mPageList.erase(it + 1, mPageList.end()); // 删除 EDIT 后面的所有元素
        // }
        goTo(previousPage);
        if(previousPage != PAGE_HOME){
            mPageList.pop_back();     
        }

    }else{
        auto it = mPageList.begin();
        if (it != mPageList.end()) {
            for (; it != mPageList.end(); it++) {
                LOGE("*it = %d" ,(*it));
            }
        }
        LOGI("No previous page");
    }
}

/// @brief 关闭所有页面
/// @param withPop 是否包含弹窗
void CWindMgr::closeAll(bool withPop) {
    mWindow->removePage();
    if (withPop)mWindow->removePop();
    std::vector<PageBase*> vec;
    for (const auto& pair : mPageMap)vec.push_back(pair.second);
    mPageList.clear();
    mPageMap.clear();
    for (auto it : vec) {
        LOGW("close page: %d <- %p | page count=%d ", it->getType(), it, mPageMap.size());
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
    auto it = mPageMap.find(page);
    if (it == mPageMap.end() && !createPage(page)) {
        if (showBlack)mWindow->showBlack();
        return;
    }
    mWindow->showPage(mPageMap[page]);
    if(page == PAGE_HOME && !isBackPage){
        mPageList.clear();      
    }
    mPageList.push_back(mPageMap[page]->getType());
    if (showBlack)mWindow->showBlack();
    isBackPage = false;
    LOGI("show page: %d <- %p", page, mPageMap[page]);
}

/// @brief 向指定页面发送消息
/// @param page 页面ID
/// @param type 消息类型
/// @param data 消息数据
void CWindMgr::sendMsg(int page, int type, void* data) {
    auto it = mPageMap.find(page);
    if (it != mPageMap.end()) {
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
    auto it = mPopFactory.find(type);
    if (it != mPopFactory.end())
        pop = it->second(); // 调用工厂函数创建页面
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
        mPageMap[page] = pb;
        LOGW("add new page: %d <- %p | page count=%d ", page, pb, mPageMap.size());
        return true;
    }
    LOGE("can not create page: %d", page);
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
