/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-16 14:03:52
 * @LastEditTime: 2026-01-19 10:28:43
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
public:
    uint64_t       mAutoSaveNextCheckTime;   // 下次检查时间
    uint64_t       mAutoSaveNextBackupTime;  // 下次备份时间
    const uint32_t mAutoSaveCheckInterval;   // 检查间隔
    const uint32_t mAutoSaveBackupInterval;  // 备份间隔

public:
    AutoSaveItem(uint32_t checkInterval, uint32_t backupInterval);
    virtual ~AutoSaveItem();

    void init();
    virtual bool haveChange() = 0;
    virtual bool save(bool isBackup = false) = 0;
};


#endif // !__AUTO_SAVE_H__