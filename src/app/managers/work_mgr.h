/*
 * Author: Ricken
 * Email: me@ricken.cn
 * Date: 2026-01-06 14:53:17
 * LastEditTime: 2026-01-06 14:54:29
 * FilePath: /kk_frame/src/app/managers/work_mgr.h
 * Description: 工作状态管理
 * BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __WORK_MGR_H__
#define __WORK_MGR_H__

#include "struct.h"
#include "timer_mgr.h"
#include "template/singleton.h"

#define g_work WorkMgr::instance()

class WorkMgr :public Singleton<WorkMgr> {
    friend class Singleton<WorkMgr>;

private:
    int64_t               mXXXOverTimer;  // XXX工作剩余时间
    TimerMgr::TimerHandle mXXXTimer;

protected:
    WorkMgr();

public:
    ~WorkMgr();
    void init();
    void startXXX();
    void stopXXX();

};

#endif
