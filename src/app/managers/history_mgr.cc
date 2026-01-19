/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-19 15:50:16
 * @LastEditTime: 2026-01-19 17:09:09
 * @FilePath: /kk_frame/src/app/managers/history_mgr.cc
 * @Description: 历史记录管理
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "history_mgr.h"
#include "config_info.h"
#include "utils/json_utils.h"
#include "utils/file_utils.h"
#include <cdlog.h>

/************************* 输入输出交互，根据项目变动 *************************/

/// @brief 打包数据为Json
/// @param data 数据
/// @param json Json
inline void setHistoryDataToJson(HistoryStruct& data, Json::Value& json) {
    json["title"] = data.title;
    json["status"] = data.status;
    json["time"] = data.time;
}

/// @brief 解包Json数据
/// @param data 目标
/// @param json Json
inline void setHistoryDataFromJson(HistoryStruct& data, Json::Value& json) {
    data.title = JsonUtils::get<std::string>(json, "title");
    data.status = JsonUtils::get<std::string>(json, "status");
    data.time = JsonUtils::get<std::string>(json, "time");
}

/*************************       结束自定义部分      *************************/

/// @brief 历史记录管理
HistoryMgr::HistoryMgr() : AutoSaveItem(8000, 10000) {
    mHaveChange = false;
}

/// @brief 析构
HistoryMgr::~HistoryMgr() {
}

/// @brief 初始化
void HistoryMgr::init() {
    load();
    AutoSaveItem::init();
}

/// @brief 重置
void HistoryMgr::reset() {
    std::string command = std::string("rm")  \
        + " " + HISTORY_FILE_PATH + " " + HISTORY_FILE_BAK_PATH;
    std::system(command.c_str());
    FileUtils::sync();
    load();
    mHaveChange = true;
    LOGE("history_data factory reset.");
}

/// @brief 加载数据
/// @return 是否成功
bool HistoryMgr::load() {
    mBuffer.clear();

    Json::Value historyJson;
    std::string loadingPath = "";
    size_t fileLen = 0;

    if (FileUtils::check(HISTORY_FILE_PATH, &fileLen) && fileLen > 0) {
        loadingPath = HISTORY_FILE_PATH;
    } else if (FileUtils::check(HISTORY_FILE_BAK_PATH, &fileLen) && fileLen > 0) {
        loadingPath = HISTORY_FILE_BAK_PATH;
    }

    if (loadingPath.empty() || !JsonUtils::load(loadingPath, historyJson)) {
        LOG(ERROR) << "[history] no local data file found. use null data";
        mHaveChange = true;
        return false;
    }
    LOG(INFO) << "[history] load local data. file=" << loadingPath;

    /**** 开始读取数据 ****/
    for (
        uint32_t count = historyJson.size(), index = 0;
        index < count && index < MAX_COUNT;
        index++
        ) {
        HistoryStruct data;
        setHistoryDataFromJson(data, historyJson[index]);
        mBuffer.push_back(data);
    }

    /**** 结束读取数据 ****/
    return true;
}

/// @brief 保存数据
/// @param isBackup 是否为备份 
/// @return 是否成功
bool HistoryMgr::save(bool isBackup) {
    Json::Value root;

    // 数据
    for (auto& item : mBuffer) {
        Json::Value item_json;
        setHistoryDataToJson(item, item_json);
        root.append(item_json);
    }

    // 复位标记并保存
    mHaveChange = false;
    return JsonUtils::save(
        isBackup ? HISTORY_FILE_BAK_PATH : HISTORY_FILE_PATH,
        root);
}

/// @brief 是否存在变更
/// @return 是否存在变更
bool HistoryMgr::haveChange() {
    return mHaveChange;
}

/// @brief 获取历史记录数量
/// @return 数量
uint8_t HistoryMgr::size() {
    return mBuffer.size();
}

/// @brief 获取全部历史记录
/// @return 历史记录
std::vector<const HistoryStruct*> HistoryMgr::getHistory() {
    std::vector<const HistoryStruct*> result;
    for (auto& item : mBuffer)
        result.push_back(&item);
    return result;
}

/// @brief 获取指定位置的历史记录
/// @param position 位置
/// @return 历史记录
const HistoryStruct* HistoryMgr::getHistory(int position) {
    if (position >= mBuffer.size()) return nullptr;
    auto it = mBuffer.begin();
    std::advance(it, position);
    return &(*it);
}

/// @brief 获取指定范围的历史记录
/// @param start 开始
/// @param end 结束
/// @return 历史记录
std::vector<const HistoryStruct*> HistoryMgr::getHistory(int start, int end) {
    std::vector<const HistoryStruct*> result;
    if (start >= mBuffer.size() || end >= mBuffer.size()) {
        LOGE("history_mgr: start or end out of range.");
        return result;
    };
    auto it = mBuffer.begin();
    std::advance(it, start);
    for (; it != mBuffer.end() && std::distance(it, mBuffer.end()) <= end - start; it++)
        result.push_back(&(*it));
    return result;
}

/// @brief 添加历史记录
/// @param data 数据
void HistoryMgr::addHistory(HistoryStruct& data) {
    if (mBuffer.size() >= MAX_COUNT) mBuffer.pop_back();
    mBuffer.push_front(data);
    mHaveChange = true;
}

/// @brief 删除历史记录
/// @param position 位置
void HistoryMgr::delHistory(int position) {
    if (position >= mBuffer.size()) return;
    auto it = mBuffer.begin();
    std::advance(it, position);
    mBuffer.erase(it);
    mHaveChange = true;
}
