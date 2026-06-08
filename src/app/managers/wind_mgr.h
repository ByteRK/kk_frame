/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2026-06-08 23:29:58
 * @FilePath: /kk_frame/src/app/managers/wind_mgr.h
 * @Description: 页面管理类
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WIND_MGR_H__
#define __WIND_MGR_H__

#include "template/singleton.h"
#include "wind.h"

#define g_windMgr WindMgr::instance()
#define g_window  g_windMgr->mWindow

class WindMgr : public Singleton<WindMgr>,
    public MessageHandler {
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

    std::unordered_map<int8_t, PopBase*>  mPopCache;                             // 弹窗缓存
    std::unordered_map<int8_t, PageBase*> mPageCache;                            // 页面缓存

    std::vector<std::pair<int8_t, std::unique_ptr<SaveMsgBase>>> mPopHistory;    // 弹窗历史
    std::vector<std::pair<int8_t, std::unique_ptr<SaveMsgBase>>> mPageHistory;   // 页面历史

private:
    WindMgr();

public:
    ~WindMgr();
    void init();
    void handleMessage(Message& message)override;

    bool showPage(int8_t page, LoadMsgBase* initData = nullptr, bool updateHistory = true);
    void recyclePage(PageBase* page);
    void recyclePage(int8_t page);

    bool showPop(int8_t pop, LoadMsgBase* initData = nullptr, bool updateHistory = true);
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

private:
    void screenSaver(bool lock);

};

#endif // !__WIND_MGR_H__