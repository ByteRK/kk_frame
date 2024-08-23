/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2024-08-23 10:24:02
 * @FilePath: /kk_frame/src/windows/manage.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _MANAGE_H_
#define _MANAGE_H_

#include "base.h"
#include "wind_base.h"

enum {
    HW_EVENT_DOWN,     // 按下
    HW_EVENT_LONG,     // 长按
    HW_EVENT_UP,       // 松开
};

#define g_windMgr CWindMgr::ins()

class CWindMgr :public MessageHandler {
public:
    BaseWindow* mWindow;
protected:
    enum {
        MSG_LOCK,       // 休眠与关机
        MSG_AUTO_CLOSE, // 自动关闭无用窗口
    };

    Message  mLockMsg;
    Message  mCloseMsg;
    Looper*  mLooper;
    uint64_t mLastTouchTime;
    uint64_t mInitTime;      // 初始化时间

    std::map<int, PageBase*> mPageList;  // 页面列表
public:
    static CWindMgr* ins() {
        static CWindMgr* instance = new CWindMgr();
        return instance;
    }
    void handleMessage(Message& message)override;
private:
    CWindMgr();
public:
    void init();
    ~CWindMgr();

    void add(PageBase* page);
    void close(PageBase* page);
    void close(int page);
    void closeAll();
    void goTo(int page, bool showBlack = false);

    void sendMsg(int page, int type, void* data);
    int8_t getNowPageType();
    PageBase* getNowPage();

    void sendKey(uint16_t keyCode, uint8_t status);
    void specialKey(uint8_t key);
    uint64_t getLastTouchTime();
private:
    bool createPage(int page);
    void screenSaver(bool lock);
};

#endif