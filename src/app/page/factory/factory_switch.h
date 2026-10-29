/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-29 09:12:10
 * @FilePath: /kk_frame/src/app/page/factory/factory_switch.h
 * @Description: 功能开关
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __FACTORY_SWITCH_H__
#define __FACTORY_SWITCH_H__

#include "page_factory.h"

#include "tick_mgr.h"
#include <widget/switch.h>

#include <functional>
#include <string>
#include <vector>

/// @brief 功能开关
/// @note 开关项视图按 setSwitch() 中引用到的序号动态创建，布局无需预置占位
/// @note 派生项目只需在 SWITCH_TYPE 中顺序声明类型，并在 setSwitch() 中按类型添加开关项
class FactorySwitch : public PageFactory::FactoryItem {
private:
    typedef enum {
        TYPE_AUTO_LOCK = 0,  // 自动锁屏[示例项,派生项目可替换为自己的开关]

        TYPE_SW_MAX,         // 开关项总数
    } SWITCH_TYPE;

    /// @brief 单个开关项
    class SwitchItem {
    public:
        using CheckCB  = std::function<bool()>;     // 状态检查
        using UpdateCB = std::function<void(bool)>; // 状态更新

    private:
        TextView* mText{ nullptr };     // 文本组件
        Switch*   mSwitch{ nullptr };   // 开关组件
        CheckCB   mCheckCB{ nullptr };  // 检查回调

    public:
        SwitchItem(std::string text, View* vg, CheckCB ccb, UpdateCB ucb);
        void update();
    };

private:
    ViewGroup*              mSwitchBox{ nullptr };   // 开关项容器
    std::vector<SwitchItem> mSwitchList;             // 已启用的开关项
    TickMgr::ITickVariable  mTicker;                 // 状态轮询

public:
    FactorySwitch(ViewGroup* container, PageFactory* factory);
    ~FactorySwitch() override;

protected:
    void onShow() override;
    void onHide() override;

private:
    void  setSwitch();
    View* getSwitch(SWITCH_TYPE type);

private: // 具体开关项（派生项目按此格式增删）
    bool  checkAutoLock();
    void  updateAutoLock(bool on);
};

#endif // __FACTORY_SWITCH_H__
