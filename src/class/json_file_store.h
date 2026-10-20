/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-19 10:00:00
 * @LastEditTime: 2026-10-19 10:00:00
 * @FilePath: /kk_frame/src/class/json_file_store.h
 * @Description: 主/备 JSON 数据文件读写
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __JSON_FILE_STORE_H__
#define __JSON_FILE_STORE_H__

#include <json/json.h>
#include <functional>
#include <string>

/// @brief 主/备 JSON 数据文件读写
/// @note 统一"主文件优先、备份文件兜底"以及"删除主备文件"的持久化策略
namespace JsonFileStore {

    /// @brief 内容校验回调，返回 false 表示该文件不可用，将继续尝试下一个文件
    typedef std::function<bool(const Json::Value&)> Validator;

    /// @brief 加载主/备文件，主文件优先，二者均不可用时返回 false
    /// @param json 输出的 JSON 数据
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径
    /// @param loadingPath 实际加载的文件路径，可传 nullptr
    /// @param validator 内容校验，可传 nullptr
    /// @return true 加载成功
    /// @note 解析失败或校验不通过都会继续尝试下一个文件
    bool load(Json::Value& json, const std::string& filePath,
        const std::string& backupPath, std::string* loadingPath = nullptr,
        const Validator& validator = Validator());

    /// @brief 保存数据到主/备文件
    /// @param json 待保存的 JSON 数据
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径
    /// @param isBackup true 写入备份文件，false 写入主文件
    /// @return true 保存成功
    bool save(const Json::Value& json, const std::string& filePath,
        const std::string& backupPath, bool isBackup = false);

    /// @brief 删除主/备文件并同步
    /// @param filePath 主文件路径
    /// @param backupPath 备份文件路径
    /// @return true 删除成功或文件本来就不存在
    bool remove(const std::string& filePath, const std::string& backupPath);

} // JsonFileStore

#endif // !__JSON_FILE_STORE_H__
