/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-01-16 14:13:34
 * @LastEditTime: 2026-06-24 18:27:07
 * @FilePath: /kk_frame/src/class/auto_save.cc
 * @Description: 自动保存类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "auto_save.h"
#include "template/singleton.h"
#include "utils/file_utils.h"
#include <core/uieventsource.h>
#include <core/systemclock.h>
#include <algorithm>
#include <vector>


/********************************************* 母项声明 *********************************************/

/// @brief 自动保存控制
class AutoSaveCtrl : public EventHandler,
    public Singleton<AutoSaveCtrl> {
    friend class Singleton<AutoSaveCtrl>;
private:
    bool mIsInit;
    std::vector<AutoSaveItem*> mAutoSaveList;

protected:
    /// @brief 自动保存控制
    AutoSaveCtrl();

public:
    ~AutoSaveCtrl();
    void init();
    int  checkEvents() override;
    int  handleEvents() override;
    bool handleItem(AutoSaveItem* autoSave, uint64_t nowTime, bool forceCheck = false);
    void add(AutoSaveItem* autoSave);
    void remove(AutoSaveItem* autoSave);
    void triggerSave(AutoSaveItem* autoSave);
    void sort();
};

/********************************************* 子项定义 *********************************************/

/// @brief 自动存储
/// @param checkInterval 检查间隔
/// @param backupInterval 备份间隔
AutoSaveItem::AutoSaveItem(uint32_t checkInterval, uint32_t backupInterval) :
    mAutoSaveCheckInterval(checkInterval), mAutoSaveBackupInterval(backupInterval) {
    mAutoSaveNextCheckTime = cdroid::SystemClock::uptimeMillis() + mAutoSaveCheckInterval;
    mAutoSaveNextBackupTime = UINT64_MAX;

    AutoSaveCtrl::instance()->add(this);
}

/// @brief 析构
AutoSaveItem::~AutoSaveItem() {
    AutoSaveCtrl::instance()->remove(this);
}

/// @brief 立即触发保存检查
void AutoSaveItem::triggerSave() {
    AutoSaveCtrl::instance()->triggerSave(this);
}

/// @brief 初始化
void AutoSaveItem::init() {
    AutoSaveCtrl::instance()->init();
}

/********************************************* 母项定义 *********************************************/

AutoSaveCtrl::AutoSaveCtrl() {
    mIsInit = false;
}

AutoSaveCtrl::~AutoSaveCtrl() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
}

/// @brief 初始化
void AutoSaveCtrl::init() {
    if (mIsInit) return;
    mIsInit = true;
    cdroid::Looper::getMainLooper()->addEventHandler(this);
}

/// @brief 判断事件
/// @return 
int AutoSaveCtrl::checkEvents() {
    if (mAutoSaveList.size() == 0) return 0;
    int64_t nowTime = cdroid::SystemClock::uptimeMillis();
    AutoSaveItem* autoSave = mAutoSaveList.front();
    uint64_t headTime = std::min(autoSave->mAutoSaveNextCheckTime, autoSave->mAutoSaveNextBackupTime);
    return headTime > nowTime ? 0 : 1;
}

/// @brief 处理事件
/// @return 
int AutoSaveCtrl::handleEvents() {
    uint64_t nowTime = cdroid::SystemClock::uptimeMillis();
    uint8_t dealCount = 0;
    for (;;) {
        if (mAutoSaveList.empty()) break;
        AutoSaveItem* autoSave = mAutoSaveList.front();
        uint64_t headTime = std::min(autoSave->mAutoSaveNextCheckTime, autoSave->mAutoSaveNextBackupTime);
        if (headTime > nowTime) break;
        LOGV("nowTime:%llu, autoSave:%p, nextCheckTime:%llu, nextBackupTime:%llu", nowTime, autoSave, autoSave->mAutoSaveNextCheckTime, autoSave->mAutoSaveNextBackupTime);
        dealCount++;

        handleItem(autoSave, nowTime);
        sort();
    }
    LOGV("dealCount:%d", dealCount);
    return dealCount;
}

/// @brief 处理单个自动保存项
/// @param autoSave 自动保存项
/// @param nowTime 当前时间
/// @param forceCheck 是否立即执行检查
/// @return 是否执行了保存
bool AutoSaveCtrl::handleItem(AutoSaveItem* autoSave, uint64_t nowTime, bool forceCheck) {
    if (!autoSave) {
        LOGE("autoSave is null");
        return false;
    }

    bool saved = false;
    if ((forceCheck || nowTime >= autoSave->mAutoSaveNextCheckTime) && autoSave->haveChange()) {
        saved = autoSave->save();
        FileUtils::sync();
        autoSave->mAutoSaveNextBackupTime = nowTime + autoSave->mAutoSaveBackupInterval;
    } else if (nowTime >= autoSave->mAutoSaveNextBackupTime) {
        saved = autoSave->save(true);
        FileUtils::sync();
        autoSave->mAutoSaveNextBackupTime = UINT64_MAX;
    }
    autoSave->mAutoSaveNextCheckTime = nowTime + autoSave->mAutoSaveCheckInterval;

    return saved;
}

/// @brief 添加自动保存项
/// @param autoSave 
void AutoSaveCtrl::add(AutoSaveItem* autoSave) {
    if (!autoSave) {
        LOGE("autoSave is null");
        return;
    };
    if (std::find(mAutoSaveList.begin(), mAutoSaveList.end(), autoSave) != mAutoSaveList.end()) {
        LOGE("autoSave already exists");
        return;
    };
    mAutoSaveList.push_back(autoSave);
    sort();
}

/// @brief 移除自动保存项
/// @param autoSave 
void AutoSaveCtrl::remove(AutoSaveItem* autoSave) {
    if (!autoSave) {
        LOGE("autoSave is null");
        return;
    }
    mAutoSaveList.erase(std::remove_if(mAutoSaveList.begin(), mAutoSaveList.end(),
        [autoSave](AutoSaveItem* ptr) { return ptr == autoSave; }), mAutoSaveList.end());
}

/// @brief 立即触发保存
/// @param autoSave 自动保存项
void AutoSaveCtrl::triggerSave(AutoSaveItem* autoSave) {
    if (std::find(mAutoSaveList.begin(), mAutoSaveList.end(), autoSave) == mAutoSaveList.end()) {
        LOGE("autoSave not exists");
        return;
    }

    uint64_t nowTime = cdroid::SystemClock::uptimeMillis();
    handleItem(autoSave, nowTime, true);
    sort();
}

/// @brief 排序
void AutoSaveCtrl::sort() {
    std::sort(mAutoSaveList.begin(), mAutoSaveList.end(), [](AutoSaveItem* a, AutoSaveItem* b) {
        return std::min(a->mAutoSaveNextCheckTime, a->mAutoSaveNextBackupTime) <
            std::min(b->mAutoSaveNextCheckTime, b->mAutoSaveNextBackupTime);
    });
}
