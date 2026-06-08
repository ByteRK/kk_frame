/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-03 15:23:48
 * @LastEditTime: 2026-06-03 20:53:10
 * @FilePath: /kk_frame/src/class/ntp_sync.h
 * @Description: NTP时间同步类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __NTP_SYNC_H__
#define __NTP_SYNC_H__

#include "template/singleton.h"
#include "thread_mgr.h"
#include "tick_mgr.h"

#include <stdint.h>

class NtpSync : public TickMgr::ITickClass, public ThreadTask,
    public Singleton<NtpSync> {
    friend Singleton<NtpSync>;
protected:
    NtpSync();
    ~NtpSync();

public:
    NtpSync* utc(int offsetHour = 8);
    
    void     sync();
    void     start(int64_t interval = 1800 * 1000, int64_t delay = -1);
    void     stop();

protected:
    void onTick(int64_t nowMs) override;
    int  onTask(int id, void *data) override;
    void onMain(int id, void *data) override;

private:
    void doSync();

private:
    bool    mIsStarted{ false };  // 是否启动
    int64_t mSyncInterval{ 0 };   // 同步间隔（毫秒）

    int     mUTC{ 0 };            // 时区偏移小时数
    int     mNtpTaskId{ 0 };      // NTP异步任务ID，0表示未注册
    int64_t mLastSyncMs{ 0 };     // 上次成功同步的时间戳（毫秒）
    
};

#endif // __NTP_SYNC_H__
