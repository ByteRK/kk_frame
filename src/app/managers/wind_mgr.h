/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-12-31 15:43:59
 * @FilePath: /kk_frame/src/app/managers/wind_mgr.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_MGR_H__
#define __WIND_MGR_H__

// 是否启用线程安全消息机制(对性能存在影响，默认关闭)
#define ENABLE_THREAD_SAFE_MSG false
#define MAX_THREAD_SAFE_MSG_SIZE 50         // 最大线程安全消息缓存数量
#define CHECK_THREAD_SAFE_MSG_INTERVAL 100  // 检查消息间隔(毫秒)

#include "template/singleton.h"
#include "wind.h"

#if ENABLE_THREAD_SAFE_MSG
#include <queue>
#include <mutex>
#endif

enum { // 虚拟事件类型(适配串口按键以及非标准按键)
    VIRT_EVENT_DOWN,     // 虚拟按下
    VIRT_EVENT_LONG,     // 虚拟长按
    VIRT_EVENT_UP,       // 虚拟松开
};

#define g_windMgr WindMgr::instance()
#define g_window  g_windMgr->mWindow

class WindMgr : public Singleton<WindMgr>,
#if ENABLE_THREAD_SAFE_MSG
    public MessageHandler, public EventHandler
#else
    public MessageHandler
#endif
{
    friend class Singleton<WindMgr>;
public:
    MainWindow* mWindow;

private:
    enum {
        MSG_AUTO_RECYCLE_PAGE,            // 自动回收无用窗口
        MSG_AUTO_RECYCLE_POP,             // 自动回收无用弹窗
    };

    Looper* mLooper;                      // 消息循环
    uint64_t mInitTime;                   // 初始化时间
    Message  mAutoRecyclePageMsg;         // 自动回收窗口消息定义
    Message  mAutoRecyclePopMsg;          // 自动回收弹窗消息定义

#if ENABLE_THREAD_SAFE_MSG
    uint64_t mLastCheckMsgTime;                                                  // 上次检查消息时间
    std::mutex mPopMsgCacheMutex;                                                // 弹窗消息栈锁
    std::mutex mPageMsgCacheMutex;                                               // 页面消息栈锁
    std::queue<std::pair<int8_t, std::unique_ptr<RunMsgBase>>> mPopMsgCache;     // 弹窗消息栈
    std::queue<std::pair<int8_t, std::unique_ptr<RunMsgBase>>> mPageMsgCache;    // 页面消息栈
#endif
    std::unordered_map<int8_t, PopBase*>  mPopCache;                             // 弹窗缓存
    std::unordered_map<int8_t, PageBase*> mPageCache;                            // 页面缓存

    std::vector<std::pair<int8_t, std::unique_ptr<SaveMsgBase>>> mPopHistory;    // 弹窗历史
    std::vector<std::pair<int8_t, std::unique_ptr<SaveMsgBase>>> mPageHistory;   // 页面历史

    std::unordered_map<int8_t, std::function<PopBase* ()>>  mPopFactory;         // 弹窗工厂
    std::unordered_map<int8_t, std::function<PageBase* ()>> mPageFactory;        // 页面工厂

private:
    WindMgr();

public:
    ~WindMgr();
    void init();
    void registerPage(int8_t page, std::function<PageBase* ()> func);
    void registerPop(int8_t pop, std::function<PopBase* ()> func);

#if ENABLE_THREAD_SAFE_MSG
    int checkEvents()override;
    int handleEvents()override;
#endif
    void handleMessage(Message& message)override;

    bool showPage(int8_t page, LoadMsgBase* initData = nullptr, bool updateHistory = true);
    void sendPageMsg(int8_t page, MSG_TYPE msg, bool fromUiThread = true);
    void sendPageMsg(int8_t page, const RunMsgBase* msg, bool fromUiThread = true);
    void recyclePage(PageBase* page);
    void recyclePage(int8_t page);

    bool showPop(int8_t pop, LoadMsgBase* initData = nullptr, bool updateHistory = true);
    void sendPopMsg(int8_t page, MSG_TYPE msg, bool fromUiThread = true);
    void sendPopMsg(int8_t pop, const RunMsgBase* msg, bool fromUiThread = true);
    void hidePop();

    void goToHome(bool withBundle = false);
    void goToPageBack();
    void goToPopBack();

    void removePageHistory(int8_t page);
    void removePopHistory(int8_t pop);

private:
    bool createPage(int8_t page);
    void autoRecyclePage();
    int8_t checkCanShowPage(int8_t newPage);

    bool createPop(int8_t pop);
    void autoRecyclePop();
    int8_t checkCanShowPop(int8_t newPop);

    bool checkPCache(int8_t pType, bool isPage);
    void addToHistory(bool isPage);
    void adjustHistory(int8_t pType, bool isPage);
    void postAutoRecycle(bool isPage);
    void dealOtherThreadMsg();

private:
    void screenSaver(bool lock);

};

#endif // !__WIND_MGR_H__