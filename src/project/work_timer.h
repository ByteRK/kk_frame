/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-09-01 18:00:24
 * @LastEditTime: 2025-09-01 18:13:54
 * @FilePath: /kk_frame/src/project/work_timer.h
 * @Description: 内置定时器列表
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef _WORK_TIMER_H_
#define _WORK_TIMER_H_

#include "timer_mgr.h"

/// @brief 烹饪定时器
class CookingTimer : public TimerMgr::WorkTimer {
public:
    CookingTimer();
    ~CookingTimer();
    void onTimer(uint32_t id, size_t param, uint32_t count) override;
private:
    int mOverTimeMinute = 0;
};

#endif /* _WORK_TIMER_H_ */