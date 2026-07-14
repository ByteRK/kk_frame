/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-03 15:23:56
 * @LastEditTime: 2026-07-14 23:05:47
 * @FilePath: /kk_frame/src/class/ntp_sync.cc
 * @Description: NTP时间同步类
 *
 * @ServerList:
 *  阿里云 ntp.aliyun.com
 *  苹果 time.apple.com
 *  微软 time.windows.com
 *  腾讯 ntp.tencent.com
 *  NTP服务器池 cn.pool.ntp.org
 *
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "ntp_sync.h"
#include "time_utils.h"
#include "system_utils.h"
#include "library_config.h"

#include <cdlog.h>
#include <core/systemclock.h>
#include <errno.h>

#if PRJ_LIB_ENABLED(NTPCLIENT)
#if defined(__cplusplus)
extern "C" {
#endif
#include <ntp_client.h>
#if defined(__cplusplus)
}
#endif
#endif

#define NTP_SERVER "ntp.aliyun.com"
#define NTP_TIME_OUT 3000

static int64_t s_NTPResult = 0;     // NTP时间戳(毫秒)
static int     s_NTPFailCount = 0;  // 连续失败次数

NtpSync::NtpSync() { }

NtpSync::~NtpSync() {
    stop();
}

/// @brief 设置时区偏移小时数
/// @param offsetHour 偏移小时数，默认8（东八区）
/// @return NtpSync实例指针，便于链式调用
NtpSync* NtpSync::utc(int offsetHour /* = 8 */) {
    mUTC = offsetHour;
    return this;
}

/// @brief 立即同步
/// @note 必须先调用 start() 启动定时同步，才能调用本函数进行立即同步
void NtpSync::sync() {
    if (mIsStarted) {
        if (mLastSyncMs > 0 && cdroid::SystemClock::uptimeMillis() - mLastSyncMs <= mSyncInterval) {
            LOGW("NtpSync is syncing too frequently, please wait for the next scheduled sync");
        } else {
            g_tick->runTickNow(this);
        }
    } else {
        LOGW("NtpSync is not started, call start() before sync()");
    }
}

/// @brief 启动定时同步
/// @param interval 同步间隔，单位毫秒，默认半小时
/// @param delay 延迟启动时间，单位毫秒，默认-1，立即启动
void NtpSync::start(int64_t interval /* = 1800 * 1000 */, int64_t delay /* = -1 */) {
    setTick(interval);
    startTick(delay);
    mIsStarted = true;
    mSyncInterval = interval;
}

/// @brief 停止定时同步
void NtpSync::stop() {
    stopTick();
    if (mNtpTaskId != 0) {
        g_threadMgr->del(mNtpTaskId);
        mNtpTaskId = 0;
    }
    mIsStarted = false;
}

/// @brief Tick回调函数
/// @param nowMs 当前时间戳，单位毫秒
void NtpSync::onTick(int64_t nowMs) {
    doSync();
}

/// @brief 异步任务函数
/// @param id 任务ID
/// @param data 任务数据
/// @return 任务执行结果，0-完成，非0-未完成
int NtpSync::onTask(int id, void* data) {
#if PRJ_LIB_ENABLED(NTPCLIENT)
    int64_t       xntp_msec = 0; /* 毫秒 */
    xtime_vnsec_t xtm_vnsec = XTIME_INVALID_VNSEC;
    xtime_vnsec_t xtm_ltime = XTIME_INVALID_VNSEC;
    xtime_descr_t xtm_descr = { 0 };
    xtime_descr_t xtm_local = { 0 };

    xtm_vnsec = ntpcli_get_time(NTP_SERVER, NTP_PORT, NTP_TIME_OUT);
    if (XTMVNSEC_IS_VALID(xtm_vnsec)) {
        s_NTPFailCount = 0;
        xntp_msec = xtm_vnsec / 10000ULL;
#if defined(DEBUG) || defined(__VSCODE__)
        xtm_ltime = time_vnsec();
        xtm_descr = time_vtod(xtm_vnsec);
        xtm_local = time_vtod(xtm_ltime);

        printf("[NTP CLIENT] NTP response : [ %04d-%02d-%02d %d %02d:%02d:%02d.%03d ]\n",
            xtm_descr.ctx_year,
            xtm_descr.ctx_month,
            xtm_descr.ctx_day,
            xtm_descr.ctx_week,
            xtm_descr.ctx_hour,
            xtm_descr.ctx_minute,
            xtm_descr.ctx_second,
            xtm_descr.ctx_msec);

        printf("[NTP CLIENT] Local time   : [ %04d-%02d-%02d %d %02d:%02d:%02d.%03d ]\n",
            xtm_local.ctx_year,
            xtm_local.ctx_month,
            xtm_local.ctx_day,
            xtm_local.ctx_week,
            xtm_local.ctx_hour,
            xtm_local.ctx_minute,
            xtm_local.ctx_second,
            xtm_local.ctx_msec);

        printf("[NTP CLIENT] Deviation    : %lld us\n",
            ((x_int64_t)(xtm_ltime - xtm_vnsec)) / 10LL);
#endif
    } else {
        if ((++s_NTPFailCount) % 3 == 0) {
            LOGE("%s:%d : errno=%d count=%d", NTP_SERVER, NTP_PORT, errno, s_NTPFailCount);
        }
    }

    s_NTPResult = xntp_msec; // 同步结果
#endif
    return 0;
}

/// @brief 主线程回调函数
/// @param id 任务ID
/// @param data 任务数据
void NtpSync::onMain(int id, void* data) {
    if (id != mNtpTaskId) return; // 非本任务
    mNtpTaskId = 0; // 重置任务ID
    if (s_NTPResult != 0) {
        int64_t seconds = s_NTPResult / 1000 + mUTC * TimeUtils::HOUR_SECONDS;
        SystemUtils::setTime(seconds);
        mLastSyncMs = cdroid::SystemClock::uptimeMillis();
    }
}

/// @brief 执行NTP同步操作
void NtpSync::doSync() {
#if PRJ_LIB_ENABLED(NTPCLIENT)
    if (mNtpTaskId != 0) return; // 同步中
    mNtpTaskId = g_threadMgr->add(this, nullptr);
#else
    LOGW("NTPCLIENT library is not enabled");
#endif
}
