/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:35
 * @LastEditTime: 2026-06-19 18:23:51
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
#define g_window  g_windMgr->getWindow()

class WindMgr : public Singleton<WindMgr>,
    public MessageHandler {
    friend class Singleton<WindMgr>;
private:
    enum {
        MSG_AUTO_RECYCLE_PAGE,            // 自动回收无用窗口
        MSG_AUTO_RECYCLE_POP,             // 自动回收无用弹窗
    };

    typedef enum {
        P_JUMP_SAME,            // 相同页面/弹窗
        P_JUMP_ENABLE,          // 允许跳转
        P_JUMP_DISABLE,         // 禁止跳转
    } P_JUMP_TYPE;

private:
    using HistoryNode = std::pair<int8_t, std::unique_ptr<SaveBase>>;

    Looper* mLooper{ nullptr };           // 消息循环
    uint64_t mInitTime{ 0 };              // 初始化时间
    Message  mAutoRecyclePageMsg;         // 自动回收窗口消息定义
    Message  mAutoRecyclePopMsg;          // 自动回收弹窗消息定义

    MainWindow* mWindow{ nullptr };       // 主窗口

    std::unordered_map<int8_t, PopBase*>  mPopCache;        // 弹窗缓存
    std::unordered_map<int8_t, PageBase*> mPageCache;       // 页面缓存

    std::vector<HistoryNode> mPopHistory;                   // 弹窗历史
    std::vector<HistoryNode> mPageHistory;                  // 页面历史

private:
    WindMgr();

public:
    ~WindMgr();
    void init();

    MainWindow* getWindow();

    bool showPage(int8_t page, const LoadBase* initData = nullptr);
    bool replacePage(int8_t page, const LoadBase* initData = nullptr);
    void recyclePage(PageBase* page);
    void recyclePage(int8_t page);

    bool showPop(int8_t pop, const LoadBase* initData = nullptr);
    bool replacePop(int8_t pop, const LoadBase* initData = nullptr);
    void recyclePop(PopBase* pop);
    void recyclePop(int8_t pop);
    void clearPop();

    void goToHome(bool withBundle = false);
    void goToPageBack();
    void goToPopBack();

    void removePageHistory(int8_t page);
    void removePopHistory(int8_t pop);

protected:
    void handleMessage(Message& message)override;

private:
    bool createPage(int8_t page);
    bool ensurePageCached(int8_t page);
    bool switchPage(int8_t page, const LoadBase* initData, const SaveBase* restoreData);
    bool makeCurrentPageHistory(HistoryNode* node);
    void pushPageHistory(HistoryNode&& node);
    void autoRecyclePage();
    P_JUMP_TYPE checkCanShowPage(int8_t newPage);

    bool createPop(int8_t pop);
    bool ensurePopCached(int8_t pop);
    bool switchPop(int8_t pop, const LoadBase* initData, const SaveBase* restoreData);
    bool makeCurrentPopHistory(HistoryNode* node);
    void pushPopHistory(HistoryNode&& node);
    void autoRecyclePop();
    P_JUMP_TYPE checkCanShowPop(int8_t newPop);

    void postAutoRecycle(bool isPage);

};

#endif // !__WIND_MGR_H__
