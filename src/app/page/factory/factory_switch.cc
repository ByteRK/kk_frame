/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-10-28 12:00:00
 * @LastEditTime: 2026-10-28 18:39:30
 * @FilePath: /kk_frame/src/app/page/factory/factory_switch.cc
 * @Description: 功能开关
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "factory_switch.h"

#include "config_mgr.h"

/************************** 开关项 **************************/

/// @brief 构造函数
/// @param text 显示名称
/// @param vg 开关项根视图
/// @param ccb 状态检查回调
/// @param ucb 状态更新回调
FactorySwitch::SwitchItem::SwitchItem(std::string text, View* vg, CheckCB ccb, UpdateCB ucb) {
    vg->setVisibility(View::VISIBLE);
    mText = PBase::get<TextView>(vg, AppRid::title);
    mSwitch = PBase::get<Switch>(vg, AppRid::status);
    mCheckCB = ccb;
    if (mText) mText->setText(text);
    if (mSwitch) {
        mSwitch->setOnCheckedChangeListener([ucb](CompoundButton& v, bool s) {
            if (ucb) ucb(s);
        });
    }
    update();
}

/// @brief 从数据源同步开关状态
/// @note 必须判等后再写入：CompoundButton::setChecked 无判等，
///       会无条件走 Switch::doSetChecked，从而重置/取消 thumb 滑动动画
void FactorySwitch::SwitchItem::update() {
    if (mCheckCB == nullptr || mSwitch == nullptr) return;

    const bool checked = mCheckCB();
    if (mSwitch->isChecked() != checked) mSwitch->setChecked(checked);
}

/************************** 功能开关 **************************/

FactorySwitch::FactorySwitch(ViewGroup* container, PageFactory* factory) :
    PageFactory::FactoryItem(container, factory, "@layout/page_factory_switch") {
    mSwitchBox = PBase::get<ViewGroup>(mRootView, AppRid::switch_box);
    FailFast(mSwitchBox == nullptr, "FactorySwitch: switch box not found");

    setExitBtn(AppRid::exit);
    setSwitch();

    // 外部可能通过其它入口改变开关状态，轮询保持显示同步
    mTicker.setTick(1000);
    mTicker.setCallBack([this](int64_t) {
        for (auto& item : mSwitchList) item.update();
    });
}

FactorySwitch::~FactorySwitch() {
    mTicker.stopTick();
}

void FactorySwitch::onShow() {
    mTicker.startTick(-1);

    // 重新挂载会清除 laid-out 状态，而 Switch::doSetChecked 依赖 isLaidOut() 决定是否播放动画，
    // 未重建时会直接跳变，故此处主动请求重排
    for (int i = 0, sum = mSwitchBox->getChildCount(); i < sum; i++) {
        Switch* sw = PBase::get<Switch>(mSwitchBox->getChildAt(i), AppRid::status);
        if (sw)sw->requestLayout();
    }
}

void FactorySwitch::onHide() {
    mTicker.stopTick();
}

void FactorySwitch::setSwitch() {
    mSwitchList.reserve(TYPE_SW_MAX);

    // 示例项：派生项目按此格式替换为自己的开关
    mSwitchList.push_back(SwitchItem(
        "自动锁屏",
        getSwitch(TYPE_AUTO_LOCK),
        std::bind(&FactorySwitch::checkAutoLock, this),
        std::bind(&FactorySwitch::updateAutoLock, this, std::placeholders::_1)
    ));
}

/// @brief 获取指定序号的开关项，不足时按需创建
/// @param type 开关类型
/// @return 开关项根视图
/// @note 实际创建数量取决于 setSwitch() 中引用到的最大序号，布局无需预置占位
View* FactorySwitch::getSwitch(SWITCH_TYPE type) {
    FailFast(mSwitchBox == nullptr, "FactorySwitch::getSwitch : switch box is null");

    while (static_cast<int>(mSwitchBox->getChildCount()) <= static_cast<int>(type)) {
        // 未绑定前先隐藏，避免跳过某个序号时留下空白占位
        View* item = inflateItem("@layout/page_factory_switch_item", mSwitchBox);
        FailFast(item == nullptr, "FactorySwitch::getSwitch : inflate switch item failed");
        item->setVisibility(View::GONE);
    }
    return mSwitchBox->getChildAt(type);
}

/************************** 具体开关项 **************************/

bool FactorySwitch::checkAutoLock() {
    return g_config->autoLock();
}

void FactorySwitch::updateAutoLock(bool on) {
    g_config->autoLock(on);
}

FACTORY_ITEM_REGISTER(AppRid::to_switch, FactorySwitch);
