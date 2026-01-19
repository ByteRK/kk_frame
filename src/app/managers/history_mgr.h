/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-19 15:50:12
 * @LastEditTime: 2026-01-19 17:11:30
 * @FilePath: /kk_frame/src/app/managers/history_mgr.h
 * @Description: 历史记录管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __HISTORY_MGR_H__
#define __HISTORY_MGR_H__


#include "struct.h"
#include "template/singleton.h"
#include "class/auto_save.h"
#include <list>

#define g_history HistoryMgr::instance()

class HistoryMgr :
    public AutoSaveItem,
    public Singleton<HistoryMgr> {
    friend Singleton<HistoryMgr>;
private:
    static constexpr uint8_t             MAX_COUNT = 80;        // 最大存储80条数据
    std::list<HistoryStruct>             mBuffer;               // 数据缓冲区
    bool                                 mHaveChange;           // 数据是否发生变化

private:
    time_t                               mLastEventTime;        // 上次事件时间
    time_t                               mCurrentDayStart;      // 当天开始时间
    
private:
    HistoryMgr();

public:
    ~HistoryMgr();
    void            init();
    void            reset();
    bool            load();
    bool            save(bool isBackup = false) override;
    bool            haveChange() override;

public:
    uint8_t         size();
    std::vector<const HistoryStruct*> 
                    getHistory();
    const HistoryStruct*
                    getHistory(int position);
    std::vector<const HistoryStruct*>
                    getHistory(int start, int end);
    void            addHistory(HistoryStruct& data);
    void            delHistory(int position);
};

#endif
