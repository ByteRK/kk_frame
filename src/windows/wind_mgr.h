/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-08-12 05:46:11
 * @FilePath: /kk_frame/src/windows/wind_mgr.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _WIND_MGR_H_
#define _WIND_MGR_H_

// 是否启用线程安全消息机制(对性能存在影响，默认关闭)
#define ENABLE_THREAD_SAFE_MSG false
#define CHECK_THREAD_SAFE_MSG_INTERVAL 100 // 检查消息间隔(毫秒)
#define MAX_THREAD_SAFE_MSG_SIZE 50        // 最大线程安全消息数量

#include "wind_base.h"
#include "json_func.h"

#if ENABLE_THREAD_SAFE_MSG
#include <queue>
#include <mutex>
#endif

enum { // 虚拟事件类型(适配串口按键以及非标准按键)
    VIRT_EVENT_DOWN,     // 虚拟按下
    VIRT_EVENT_LONG,     // 虚拟长按
    VIRT_EVENT_UP,       // 虚拟松开
};

#define g_windMgr CWindMgr::ins()
#define g_window  g_windMgr->mWindow

#if ENABLE_THREAD_SAFE_MSG
class CWindMgr :public MessageHandler, public EventHandler {
#else
class CWindMgr :public MessageHandler {
#endif
public:
    BaseWindow* mWindow;
protected:
    enum {
        MSG_AUTO_CLOSE_PAGE,  // 自动回收无用窗口
        MSG_AUTO_CLOSE_POP,   // 自动回收无用弹窗
    };

    Message  mAutoClosePageMsg;
    Message  mAutoClosePopMsg;
    Looper*  mLooper;
    uint64_t mInitTime;      // 初始化时间

#if ENABLE_THREAD_SAFE_MSG
    uint64_t mLastCheckMsgTime;                                        // 上次检查消息时间
    std::mutex mPopMsgCacheMutex;                                      // 弹窗消息栈锁
    std::mutex mPageMsgCacheMutex;                                     // 页面消息栈锁
    std::queue<std::pair<uint8_t, Json::Value>> mPopMsgCache;          // 弹窗消息栈
    std::queue<std::pair<uint8_t, Json::Value>> mPageMsgCache;         // 页面消息栈
#endif
    std::unordered_map<int, PopBase*>  mPopCache;   // 弹窗缓存
    std::unordered_map<int, PageBase*> mPageCache;  // 页面缓存

    std::vector<std::pair<int,PBase::StateBundle>> mPopHistory;   // 弹窗历史
    std::vector<std::pair<int,PBase::StateBundle>> mPageHistory;  // 页面历史
private:
    std::unordered_map<int, std::function<PopBase*()>>  mPopFactory;  // 弹窗工厂
    std::unordered_map<int, std::function<PageBase*()>> mPageFactory; // 页面工厂
public:
    static CWindMgr* ins() {
        static CWindMgr* instance = new CWindMgr();
        return instance;
    }
#if ENABLE_THREAD_SAFE_MSG
    int checkEvents()override;
    int handleEvents()override;
#endif
    void handleMessage(Message& message)override;
private:
    CWindMgr();
public:
    ~CWindMgr();
    
    void init();
    void close(PageBase* page);
    void close(int page);
    void closeAll(bool withPop = false);
    bool goTo(int page, Json::Value* baseData = nullptr);
    void goTo(int page, PBase::StateBundle& bundle);
    void sendMsg(int page, const Json::Value& data, bool fromUiThread = true);

    bool showPop(int8_t pop, Json::Value* baseData = nullptr);
    void hidePop();
    void sendPopMsg(int pop, const Json::Value& data, bool fromUiThread = true);

    void goToHome(bool withBundle = false);
    void goToPageBack();
    void goToPopBack();

    void removePageHistory(int page);
    void removePopHistory(int pop);
private:
    bool createPage(int page);
    bool createPop(int pop);
    void screenSaver(bool lock);

    bool getBundle(bool isPage, int id, PBase::StateBundle& bundle);
    
};

#endif // _WIND_MGR_H_