/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-02 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_http.h
 * @Description: 网络测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_HTTP_H__
#define __FACTORY_HTTP_H__

#include "page_factory.h"
#include "http_mgr.h"
#include "tick_mgr.h"

#include <widget/button.h>

/// @brief 网络测试
/// @note 验证 Http 各类请求行为，需配合script/http_test_server.py脚本
class FactoryHttp : public PageFactory::FactoryItem,
    public HttpManager::IEventListener {
private:
    TextView*                 mResult{ nullptr };      // 结果滚动显示
    TextView*                 mStressStat{ nullptr };  // 挂测统计显示
    Button*                   mStressBtn{ nullptr };   // 挂测启停按钮

    HttpManager::RequestId    mSlowId{ 0 };            // 慢速下载请求id（用于取消）
    HttpManager::RequestId    mFlakyId{ 0 };           // 重试测试请求id
    int                       mFlakyStartCount{ 0 };   // 重试测试发起次数（含重试）

    std::string               mLog;                    // 结果累积文本

    /* 挂测状态 */
    TickMgr::ITickVariable    mStressTicker;           // 挂测驱动定时器
    bool                      mStressRunning{ false }; // 挂测是否运行中
    uint64_t                  mStressRounds{ 0 };      // 已发起轮数
    uint64_t                  mStressTotal{ 0 };       // 总请求数
    uint64_t                  mStressSuccess{ 0 };     // 成功数
    uint64_t                  mStressFail{ 0 };        // 失败数
    int                       mStressInFlight{ 0 };    // 当前在途挂测请求数
    int64_t                   mStressStartMs{ 0 };     // 挂测起始时间
    int                       mStressSeq{ 0 };         // 请求类型轮换序号

public:
    FactoryHttp(ViewGroup* container, PageFactory* factory);
    ~FactoryHttp() override;

protected:
    void onHide() override;
    void onHttpEvent(const HttpManager::Event& event) override;

private:
    void appendResult(const std::string& line);
    void logOnly(const std::string& line);
    void refreshStressStat();

    void testPing();
    void testDownload();
    void testCancel();
    void testRetry();
    void testPost();
    void testHead();
    void testUpload();
    void testBig();
    void testAll();

    /* 挂测 */
    void toggleStress();
    void startStress();
    void stopStress();
    void onStressTick(int64_t nowMs);
    void submitStressOne();
};

#endif // __FACTORY_HTTP_H__
