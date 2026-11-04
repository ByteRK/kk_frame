/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-11-02 12:00:00
 * @LastEditTime: 2026-11-02 16:28:25
 * @FilePath: /kk_frame/src/app/page/factory/factory_http.cc
 * @Description: 网络测试
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_http.h"

#include "file_utils.h"
#include "path_utils.h"
#include "string_utils.h"
#include "time_utils.h"

namespace {

    const char* kBaseUrl = "http://127.0.0.1:18080";
    const char* kTestDir = "/tmp/kk_http_test";
    const char* kDownloadPath = "/tmp/kk_http_test/download.bin";
    const char* kSlowPath = "/tmp/kk_http_test/slow.bin";
    const char* kBigPath = "/tmp/kk_http_test/big.bin";
    const char* kUploadPath = "/tmp/kk_http_test/upload.bin";

    /* 挂测参数 */
    const int64_t kStressIntervalMs = 2000;   // 每轮发起间隔
    const int kStressMaxInFlight = 2;         // 在途请求上限（防堆积）
    const char* kStressTagPrefix = "stress."; // 挂测请求 tag 前缀

    /// @brief 生成一个用于上传测试的本地文件
    bool prepareUploadFile(const std::string& path) {
        const std::string payload = "kk_frame upload test data\n";
        std::string content;
        content.reserve(payload.size() * 64);
        for (int i = 0; i < 64; ++i) content += payload;
        return FileUtils::write(path, content);
    }

    /// @brief 将 HTTP 方法转为可读字符串（用于日志）
    static const char* methodName(HttpManager::HttpMethod method) {
        switch (method) {
        case HttpManager::HttpMethod::GET:     return "GET";
        case HttpManager::HttpMethod::POST:    return "POST";
        case HttpManager::HttpMethod::PUT:     return "PUT";
        case HttpManager::HttpMethod::PATCH:   return "PATCH";
        case HttpManager::HttpMethod::DELETE_: return "DELETE";
        case HttpManager::HttpMethod::HEAD:    return "HEAD";
        case HttpManager::HttpMethod::CUSTOM:  return "CUSTOM";
        }
        return "?";
    }

} // namespace

FactoryHttp::FactoryHttp(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_http") {
    setExitBtn(AppRid::exit);

    mResult = PBase::get<TextView>(mRootView, AppRid::http_result);
    mStressStat = PBase::get<TextView>(mRootView, AppRid::http_stress_stat);
    mStressBtn = PBase::get<Button>(mRootView, AppRid::http_stress);

    FileUtils::mkdir(kTestDir);
    prepareUploadFile(kUploadPath);

    PBase::click(mRootView, AppRid::http_ping, [this](View&) { testPing(); });
    PBase::click(mRootView, AppRid::http_download, [this](View&) { testDownload(); });
    PBase::click(mRootView, AppRid::http_cancel, [this](View&) { testCancel(); });
    PBase::click(mRootView, AppRid::http_retry, [this](View&) { testRetry(); });
    PBase::click(mRootView, AppRid::http_post, [this](View&) { testPost(); });
    PBase::click(mRootView, AppRid::http_head, [this](View&) { testHead(); });
    PBase::click(mRootView, AppRid::http_upload, [this](View&) { testUpload(); });
    PBase::click(mRootView, AppRid::http_big, [this](View&) { testBig(); });
    PBase::click(mRootView, AppRid::http_all, [this](View&) { testAll(); });
    PBase::click(mRootView, AppRid::http_stress, [this](View&) { toggleStress(); });

    mStressTicker.setTick(kStressIntervalMs);
    mStressTicker.setCallBack(std::bind(&FactoryHttp::onStressTick, this,
        std::placeholders::_1));

    appendResult("网络测试就绪");
    appendResult(StringUtils::format("服务器: %s", kBaseUrl));
#if !ENABLED(CURL)
    appendResult("警告: 当前构建未启用 HTTP 支持");
#endif
}

FactoryHttp::~FactoryHttp() {
    stopStress();
    if (mSlowId) g_http->cancel(mSlowId);
    if (mFlakyId) g_http->cancel(mFlakyId);
}

void FactoryHttp::onHide() {
    // 离开测试项时终止挂测及可能长时间运行的请求，避免后台继续占用
    stopStress();
    if (mSlowId) { g_http->cancel(mSlowId); mSlowId = 0; }
    if (mFlakyId) { g_http->cancel(mFlakyId); mFlakyId = 0; }
}

void FactoryHttp::appendResult(const std::string& line) {
    mLog += line;
    mLog += "\n";
    if (mResult) mResult->setText(mLog.c_str());

    LOGI("%s", line.c_str());
}

void FactoryHttp::logOnly(const std::string& line) {
    LOGI("%s", line.c_str());
}

void FactoryHttp::testPing() {
    mLog.clear();
    appendResult("--- 连通性测试: GET /ping ---");
    g_http->submit(HttpManager::Request::CreateGet(
        std::string(kBaseUrl) + "/ping", this, "http.ping"));
}

void FactoryHttp::testDownload() {
    mLog.clear();
    appendResult("--- 下载落盘测试: GET /download ---");
    g_http->submit(HttpManager::Request::CreateDownload(
        std::string(kBaseUrl) + "/download", kDownloadPath, this, "http.download"));
}

void FactoryHttp::testCancel() {
    mLog.clear();
    appendResult("--- 取消下载测试: GET /slow ---");
    HttpManager::Request req = HttpManager::Request::CreateDownload(
        std::string(kBaseUrl) + "/slow", kSlowPath, this, "http.slow");
    req.emitProgress = false;
    mSlowId = g_http->submit(req);
}

void FactoryHttp::testRetry() {
    mLog.clear();
    mFlakyStartCount = 0;
    appendResult("--- 退避重试测试: GET /flaky ---");
    HttpManager::Request req = HttpManager::Request::CreateGet(
        std::string(kBaseUrl) + "/flaky", this, "http.flaky");
    // 短超时，配合服务器 /flaky 前 2 次延迟，触发网络层超时重试
    req.connectTimeoutMs = 2000;
    req.requestTimeoutMs = 2000;
    mFlakyId = g_http->submit(req);
}

void FactoryHttp::testPost() {
    mLog.clear();
    appendResult("--- POST 回显测试: POST /echo ---");
    g_http->submit(HttpManager::Request::CreatePost(
        std::string(kBaseUrl) + "/echo", "hello-from-kk_frame", this, "http.post"));
}

void FactoryHttp::testHead() {
    mLog.clear();
    appendResult("--- HEAD 测试: HEAD /ping ---");
    HttpManager::Request req = HttpManager::Request::CreateGet(
        std::string(kBaseUrl) + "/ping", this, "http.head");
    req.method = HttpManager::HttpMethod::HEAD;
    g_http->submit(req);
}

void FactoryHttp::testUpload() {
    mLog.clear();
    appendResult("--- 上传测试: PUT /upload ---");
    if (!PathUtils::exists(kUploadPath)) {
        prepareUploadFile(kUploadPath);
    }
    g_http->submit(HttpManager::Request::CreateUpload(
        std::string(kBaseUrl) + "/upload", kUploadPath, this, "http.upload"));
}

void FactoryHttp::testBig() {
    mLog.clear();
    appendResult("--- 大文件下载测试: GET /big (4MB) ---");
    HttpManager::Request req = HttpManager::Request::CreateDownload(
        std::string(kBaseUrl) + "/big", kBigPath, this, "http.big");
    req.emitProgress = true;
    req.progressIntervalMs = 500;
    g_http->submit(req);
}

void FactoryHttp::testAll() {
    mLog.clear();
    appendResult("--- 并发测试: ping + download + post ---");
    g_http->submit(HttpManager::Request::CreateGet(
        std::string(kBaseUrl) + "/ping", this, "http.ping"));
    g_http->submit(HttpManager::Request::CreateDownload(
        std::string(kBaseUrl) + "/download", kDownloadPath, this, "http.download"));
    g_http->submit(HttpManager::Request::CreatePost(
        std::string(kBaseUrl) + "/echo", "hello-from-kk_frame", this, "http.post"));
}

void FactoryHttp::onHttpEvent(const HttpManager::Event& event) {
    const std::string& tag = event.tag;

    // 挂测请求独立统计，不进入普通日志
    if (tag.find(kStressTagPrefix) == 0) {
        switch (event.type) {
        case HttpManager::EventType::COMPLETED:
        case HttpManager::EventType::CANCELLED: {
            if (mStressInFlight > 0) --mStressInFlight;
            ++mStressTotal;
            if (event.type == HttpManager::EventType::COMPLETED
                && event.response.isSuccess()) {
                ++mStressSuccess;
            } else {
                ++mStressFail;
                // 失败/取消时记录终端日志，便于日后问题分析（成功静默）
                logOnly(StringUtils::format(
                    "[%s] 失败 id=%llu code=%d http=%d 原因=%s (累计 成功%llu/失败%llu)",
                    tag.c_str(),
                    static_cast<unsigned long long>(event.requestId),
                    static_cast<int>(event.response.curlCode),
                    static_cast<int>(event.response.httpStatusCode),
                    event.response.errorMessage.empty()
                    ? "-" : event.response.errorMessage.c_str(),
                    static_cast<unsigned long long>(mStressSuccess),
                    static_cast<unsigned long long>(mStressFail)));
            }
            refreshStressStat();
        } break;
        default: {
        } break;
        }
        return;
    }

    switch (event.type) {
    case HttpManager::EventType::STARTED: {
        appendResult(StringUtils::format("[%s] 开始 id=%llu %s %s",
            tag.c_str(),
            static_cast<unsigned long long>(event.requestId),
            methodName(event.method),
            event.url.c_str()));
        if (tag == "http.flaky") ++mFlakyStartCount;
        if (tag == "http.slow") {
            appendResult("[slow] 已开始下载，立即取消...");
            g_http->cancel(event.requestId);
            mSlowId = 0;
        }
    } break;

    case HttpManager::EventType::PROGRESS: {
        if (tag == "http.big" && event.response.downloadTotalBytes > 0) {
            const int percent = static_cast<int>(
                event.response.downloadedBytes * 100
                / event.response.downloadTotalBytes);
            appendResult(StringUtils::format("   大文件进度: %d%%", percent));
        }
    } break;

    case HttpManager::EventType::COMPLETED: {
        const bool ok = event.response.isSuccess();
        appendResult(StringUtils::format(
            "[%s] 完成 id=%llu code=%d http=%d %s 耗时%.2fs",
            tag.c_str(),
            static_cast<unsigned long long>(event.requestId),
            static_cast<int>(event.response.curlCode),
            static_cast<int>(event.response.httpStatusCode),
            ok ? "成功" : "失败",
            event.response.totalTimeSeconds));
        if (!event.response.errorMessage.empty()) {
            appendResult("   错误: " + event.response.errorMessage);
        }
        if (!event.response.effectiveUrl.empty()
            && event.response.effectiveUrl != event.url) {
            appendResult("   重定向: " + event.response.effectiveUrl);
        }
        if (!event.response.outputFilePath.empty()) {
            appendResult("   文件: " + event.response.outputFilePath);
        }
        if (event.response.downloadedBytes > 0) {
            appendResult(StringUtils::format("   下载: %lld 字节",
                static_cast<long long>(event.response.downloadedBytes)));
        }
        if (event.response.uploadedBytes > 0) {
            appendResult(StringUtils::format("   上传: %lld 字节",
                static_cast<long long>(event.response.uploadedBytes)));
        }
        if (tag == "http.flaky") {
            appendResult(StringUtils::format("   发起次数(含重试): %d", mFlakyStartCount));
        }
        if (tag == "http.upload" && !event.response.body.empty()) {
            appendResult("   回显: " + std::string(
                event.response.body.begin(), event.response.body.end()));
        }
        if (tag == "http.post" && !event.response.body.empty()) {
            appendResult("   回显: " + std::string(
                event.response.body.begin(), event.response.body.end()));
        }
    } break;

    case HttpManager::EventType::CANCELLED: {
        appendResult(StringUtils::format("[%s] 已取消", tag.c_str()));
        if (tag == "http.slow") {
            const std::string partPath = std::string(kSlowPath) + ".part";
            appendResult(StringUtils::format("   临时文件残留: %s",
                PathUtils::exists(partPath) ? "有(异常!)" : "无"));
            appendResult(StringUtils::format("   目标文件存在: %s",
                PathUtils::exists(kSlowPath) ? "是(异常!)" : "否"));
        }
    } break;

    default: {
    } break;
    }
}

/* ============================ 挂测 ============================ */

void FactoryHttp::toggleStress() {
    if (mStressRunning) {
        stopStress();
    } else {
        startStress();
    }
}

void FactoryHttp::startStress() {
    if (mStressRunning) return;

    mStressRunning = true;
    mStressRounds = 0;
    mStressTotal = 0;
    mStressSuccess = 0;
    mStressFail = 0;
    mStressInFlight = 0;
    mStressSeq = 0;
    mStressStartMs = cdroid::SystemClock::uptimeMillis();

    if (mStressStat) mStressStat->setVisibility(View::VISIBLE);
    if (mStressBtn) {
        mStressBtn->setText("停止挂测");
        mStressBtn->setActivated(true);
    }

    mLog.clear();
    appendResult("--- 挂测开始（循环 ping/download/post，请观察统计区） ---");

    mStressTicker.startTick(-1);  // 立即发起第一轮
    refreshStressStat();
}

void FactoryHttp::stopStress() {
    if (!mStressRunning) return;

    mStressRunning = false;
    mStressTicker.stopTick();

    if (mStressStat) mStressStat->setVisibility(View::GONE);
    if (mStressBtn) {
        mStressBtn->setText("挂测");
        mStressBtn->setActivated(false);
    }

    refreshStressStat();
    appendResult(StringUtils::format(
        "--- 挂测结束: 发起 %llu 完成 %llu 成功 %llu 失败 %llu ---",
        static_cast<unsigned long long>(mStressRounds),
        static_cast<unsigned long long>(mStressTotal),
        static_cast<unsigned long long>(mStressSuccess),
        static_cast<unsigned long long>(mStressFail)));
}

void FactoryHttp::onStressTick(int64_t /*nowMs*/) {
    if (!mStressRunning) return;

    // 在途请求少于上限时补发，保持稳定压力且不堆积
    int submitCount = 0;
    while (mStressInFlight < kStressMaxInFlight && submitCount < kStressMaxInFlight) {
        submitStressOne();
        ++submitCount;
    }
    refreshStressStat();
}

void FactoryHttp::submitStressOne() {
    const int type = mStressSeq++ % 3;
    ++mStressRounds;
    ++mStressInFlight;

    switch (type) {
    case 0: {
        HttpManager::Request req = HttpManager::Request::CreateGet(
            std::string(kBaseUrl) + "/ping", this, "stress.ping");
        req.requestTimeoutMs = 5000;
        g_http->submit(req);
    } break;
    case 1: {
        HttpManager::Request req = HttpManager::Request::CreateDownload(
            std::string(kBaseUrl) + "/download", kDownloadPath, this, "stress.download");
        req.requestTimeoutMs = 5000;
        req.emitProgress = false;
        g_http->submit(req);
    } break;
    default: {
        HttpManager::Request req = HttpManager::Request::CreatePost(
            std::string(kBaseUrl) + "/echo", "stress", this, "stress.post");
        req.requestTimeoutMs = 5000;
        g_http->submit(req);
    } break;
    }
}

void FactoryHttp::refreshStressStat() {
    if (mStressStat == nullptr) return;
    if (!mStressRunning && mStressTotal == 0) {
        mStressStat->setVisibility(View::GONE);
        return;
    }

    const int64_t elapsedMs = cdroid::SystemClock::uptimeMillis() - mStressStartMs;
    const std::string elapsed = TimeUtils::formatDuration(elapsedMs / 1000, false);

    const int successRate = mStressTotal > 0
        ? static_cast<int>(mStressSuccess * 100 / mStressTotal)
        : 0;

    mStressStat->setVisibility(View::VISIBLE);
    mStressStat->setText(StringUtils::format(
        mStressRunning
        ? "挂测运行中 | %s | 发起 %llu 完成 %llu\n成功 %llu 失败 %llu | 成功率 %d%%"
        : "挂测已停止 | %s | 发起 %llu 完成 %llu\n成功 %llu 失败 %llu | 成功率 %d%%",
        elapsed.c_str(),
        static_cast<unsigned long long>(mStressRounds),
        static_cast<unsigned long long>(mStressTotal),
        static_cast<unsigned long long>(mStressSuccess),
        static_cast<unsigned long long>(mStressFail),
        successRate).c_str());
}

FACTORY_ITEM_REGISTER(AppRid::to_http, FactoryHttp);
