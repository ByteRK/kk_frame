/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2025-02-18 20:28:50
 * @FilePath: /kk_frame/src/windows/manage.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _MANAGE_H_
#define _MANAGE_H_

#include "wind_base.h"

enum { // 虚拟事件类型(适配串口按键以及非标准按键)
    VIRT_EVENT_DOWN,     // 虚拟按下
    VIRT_EVENT_LONG,     // 虚拟长按
    VIRT_EVENT_UP,       // 虚拟松开
};

#define g_windMgr CWindMgr::ins()
#define g_window  g_windMgr->mWindow

class CWindMgr :public MessageHandler {
public:
    BaseWindow* mWindow;
protected:
    enum {
        MSG_AUTO_CLOSE, // 自动关闭无用窗口
    };

    Message  mAutoCloseMsg;
    Looper*  mLooper;
    uint64_t mInitTime;      // 初始化时间

    std::unordered_map<int, PageBase*> mPageList;  // 页面列表
private:
    std::unordered_map<int, std::function<PopBase*()>>  mPopFactory;  // 弹窗工厂
    std::unordered_map<int, std::function<PageBase*()>> mPageFactory; // 页面工厂
public:
    static CWindMgr* ins() {
        static CWindMgr* instance = new CWindMgr();
        return instance;
    }
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
    void sendMsg(int page, int type, void* data);

    bool showPop(int8_t type);
    void sendPopMsg(int pop, int8_t type, void* data);
private:
    bool createPage(int page);
    void screenSaver(bool lock);
};

#endif