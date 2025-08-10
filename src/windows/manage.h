/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-08-10 16:46:33
 * @FilePath: /kk_frame/src/windows/manage.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _MANAGE_H_
#define _MANAGE_H_

// 是否启用线程安全消息机制(对性能存在影响，默认关闭)
#define ENABLE_THREAD_SAFE_MSG false
#define CHECK_THREAD_SAFE_MSG_INTERVAL 100 // 检查消息间隔(毫秒)

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
        MSG_AUTO_CLOSE, // 自动关闭无用窗口
    };

    Message  mAutoCloseMsg;
    Looper*  mLooper;
    uint64_t mInitTime;      // 初始化时间

#if ENABLE_THREAD_SAFE_MSG
    uint64_t mLastCheckMsgTime;                                        // 上次检查消息时间
    std::mutex mPopMsgCacheMutex;                                      // 弹窗消息栈锁
    std::mutex mPageMsgCacheMutex;                                     // 页面消息栈锁
    std::queue<std::pair<uint8_t, Json::Value>> mPopMsgCache;          // 弹窗消息栈
    std::queue<std::pair<uint8_t, Json::Value>> mPageMsgCache;         // 页面消息栈
#endif

    std::unordered_map<int, PopBase*>  mPopList;   // 弹窗列表
    std::unordered_map<int, PageBase*> mPageList;  // 页面列表
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
    void goTo(int page, bool showBlack = false);
    void sendMsg(int page, const Json::Value& data, bool fromOtherThread = false);

    bool showPop(int8_t type);
    void sendPopMsg(int pop, const Json::Value& data, bool fromOtherThread = false);
private:
    bool createPage(int page);
    void screenSaver(bool lock);
};

#endif