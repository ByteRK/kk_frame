/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-16 14:03:52
 * @LastEditTime: 2026-09-21 15:13:53
 * @FilePath: /kk_frame/src/class/auto_save.h
 * @Description: 自动保存类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __AUTO_SAVE_H__
#define __AUTO_SAVE_H__

#include <stdint.h>

class AutoSaveItem {
    friend class AutoSaveCtrl;

public:
    uint64_t       mAutoSaveNextCheckTime;   // 下次检查时间
    uint64_t       mAutoSaveNextBackupTime;  // 下次备份时间
    const uint32_t mAutoSaveCheckInterval;   // 检查间隔
    const uint32_t mAutoSaveBackupInterval;  // 备份间隔

public:
    AutoSaveItem(uint32_t checkInterval, uint32_t backupInterval);
    virtual ~AutoSaveItem();

    /// @brief 开始自动保存（加入自动保存列表）
    /// @note 构造时已自动调用，重复调用会被拒绝并打印错误日志
    void startAutoSave();

    /// @brief 停止自动保存（移出自动保存列表）
    /// @note 析构时已自动调用；仅移出列表，不会立即保存，期间的改动不再自动落盘
    void stopAutoSave();

    /// @brief 立即触发保存检查（不受检查间隔限制）
    /// @note 仍需 haveChange() 为真；未加入自动保存列表时不生效
    void triggerSave();

    /// @brief 清空所有自动保存任务
    /// @note 谨慎使用，一般用于恢复出厂时避免意外保存无关数据；
    ///       清空后各对象仍然存活，需要时自行调用 startAutoSave 重新加入
    static void clearAllAutoSaveTasks();

protected:
    void init();
    virtual bool haveChange() = 0;
    virtual bool save(bool isBackup = false) = 0;
};


#endif // !__AUTO_SAVE_H__
