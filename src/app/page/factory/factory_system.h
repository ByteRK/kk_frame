/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 12:00:00
 * @FilePath: /kk_frame/src/app/page/factory/factory_system.h
 * @Description: 系统信息
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_SYSTEM_H__
#define __FACTORY_SYSTEM_H__

#include "page_factory.h"

/// @brief 系统信息
class FactorySystem : public PageFactory::FactoryItem {
private:
    /// @brief 静态信息（运行期不变，构造时采集一次）
    struct StaticInfo {
        std::string hostName;            // 主机名称
        std::string kernel;              // 内核版本
        std::string cpuModel;            // CPU 型号
        long        cpuCount{ 0 };       // CPU 核心数
        uint64_t    memTotalKb{ 0 };     // 内存总量（KB）
        uint64_t    swapTotalKb{ 0 };    // 交换分区总量（KB）
        int         pid{ 0 };            // 进程 PID
        uint64_t    rootDiskTotal{ 0 };  // 根分区总量（字节）
        uint64_t    dataDiskTotal{ 0 };  // 数据分区总量（字节）
    };

private:
    TextView*              mSystemInfo{ nullptr };  // 信息显示区
    TickMgr::ITickVariable mTicker;                 // 刷新定时器
    StaticInfo             mStatic;                 // 静态信息缓存

public:
    FactorySystem(ViewGroup* container, PageFactory* factory);
    ~FactorySystem() override;

protected:
    void onShow() override;
    void onHide() override;

private:
    void buildStaticInfo();
    void setInfo();
    void onTick(int64_t);
};

#endif // __FACTORY_SYSTEM_H__
