/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-04 00:39:16
 * @LastEditTime: 2026-07-05 02:11:23
 * @FilePath: /kk_frame/src/app/managers/auth_mgr.cc
 * @Description: 授权码管理器
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "auth_mgr.h"
#include "wind_mgr.h"
#include "message_mgr.h"
#include "config_mgr.h"

#include "env_utils.h"
#include "network_utils.h"
#include "json_utils.h"
#include "series_info.h"
#include "app_version.h"

#include <cdlog.h>

#define AUTH_SERVICE "http://10.11.12.13:1415"

AuthMgr::AuthMgr() { }

bool AuthMgr::request() {
    // 重复请求检查
    if (mReqId > 0) {
        g_window->showToast("授权码获取中，请勿重复点击...");
        LOGE("AuthMgr::request() repeat request");
        return false;
    }

    // 获取必须信息
    std::string macStr = mac(), pidStr = APP_ID;
    if (macStr.size() == 0) {
        g_window->showToast("设备MAC无法获取，请检查设备状态...");
        LOGE("AuthMgr::request() mac address is empty");
        return false;
    }

    // 构建请求参数
    Json::Value params;
    params["mac"] = macStr;
    params["pid"] = pidStr;
    std::string body = JsonUtils::toString(params);

    // 发起请求
    mReqId = g_http->submit(HttpManager::Request::CreatePost(
        authURL(), body, this
    ));
    g_window->showToast("正在获取授权码，请稍候...");
    LOGI("AuthMgr::request() mac[%s] pid[%s] body[%s]", macStr.c_str(), pidStr.c_str(), body.c_str());
    return true;
}

std::string AuthMgr::mac() {
    static std::string sMac("");
    if (sMac.size() > 0) return sMac;
    static NetworkUtils::InterfaceInfo info;
    // 读取无线网络MAC
    if (NetworkUtils::getInterfaceInfo(NET_WLAN_NAME, info)) {
        LOGI("Try Load WLAN Permanent Mac Address");
        if (info.permanentMacAddress.size() > 0) return (sMac = info.permanentMacAddress);
        LOGI("Try Load WLAN Mac Address");
        if (info.macAddress.size() > 0) return (sMac = info.macAddress);
    }
    // 读取有线网络MAC
    if (NetworkUtils::getInterfaceInfo(NET_LINE_NAME, info)) {
        LOGI("Try Load LINE Permanent Mac Address");
        if (info.permanentMacAddress.size() > 0) return (sMac = info.permanentMacAddress);
        LOGI("Try Load LINE Mac Address");
        if (info.macAddress.size() > 0) return (sMac = info.macAddress);
    }
#ifdef PRODUCT_X64
    sMac = "00:00:00:00:00:00";
#endif
    LOGE("AuthMgr::mac() get mac address failed");
    return sMac;
}

std::string AuthMgr::authURL() {
    static std::string sEnvUrl;
    if (sEnvUrl.size() > 0 || EnvUtils::getString("APP_AUTH_URL", sEnvUrl)) {
        return sEnvUrl;
    } else {
        return std::string(AUTH_SERVICE "/api/device/authorize");
    }
}

void AuthMgr::onHttpEvent(const HttpManager::Event& event) {
    // 请求ID检查
    if (mReqId != event.requestId) {
        LOGW("AuthMgr::onHttpEvent() requestId[%d:%d] not match", mReqId, event.requestId);
        return;
    }

    // 回调类别过滤
    switch (event.type) {
    case HttpManager::EventType::STARTED:
    case HttpManager::EventType::PROGRESS: {
    }   return;
    case HttpManager::EventType::CANCELLED: {
        mReqId = 0;
        LOGI("AuthMgr::onHttpEvent() cancelled");
    }   return;
    default: mReqId = 0; break;
    }

    // CURL结果检查
    if (event.response.curlCode != CURLE_OK) {
        g_window->showToast("本地请求异常，请稍候重试...");
        LOGE("AuthMgr::onHttpEvent() curlCode[%d] curlMsg[%s]", event.response.curlCode, event.response.errorMessage.c_str());
        return;
    }

    // 请求结果处理
    if (event.response.httpStatusCode != 200) {
        g_window->showToast("授权码分发服务异常[" + std::to_string(event.response.httpStatusCode) + "]，请稍候重试...");
        LOGE("AuthMgr::onHttpEvent() httpStatusCode[%d]", event.response.httpStatusCode);
        return;
    }

    // 解析JSON
    Json::Value root;
    std::string body(event.response.body.begin(), event.response.body.end());
    if (!JsonUtils::parse(body, root)) {
        g_window->showToast("授权码解析异常，请稍候重试...");
        LOGE("AuthMgr::onHttpEvent() parse json failed[%s]", body.c_str());
        return;
    }

    // 检查云端响应信息
    if (!JsonUtils::get<bool>(root, "success", false)) {
        std::string resText = JsonUtils::get<std::string>(root, "message", "授权码获取失败，请稍候重试...");
        g_window->showToast(resText);
        LOGE("AuthMgr::onHttpEvent() response failed[%s]", resText.c_str());
        return;
    }

    // 解析授权码
    Json::Value payload = root["data"]["payload"];
    std::string did = JsonUtils::get<std::string>(payload, "did", "");
    std::string license = JsonUtils::get<std::string>(payload, "license", "");
    if (did.empty() || license.empty()) {
        g_window->showToast("授权码无效，请重新获取...");
        LOGE("AuthMgr::onHttpEvent() did[%s] license[%s]", did.c_str(), license.c_str());
        return;
    }

    // 授权码保存
    g_config->setAuthInfo(APP_ID, did, license);
    LOGI("AuthMgr::onHttpEvent() did[%s] license[%s]", did.c_str(), license.c_str());
    g_msg->send(MSG_AUTH_CODE);
    g_window->showToast("授权码获取成功，即将重启设备...");
    g_window->postExit(4000);
}
