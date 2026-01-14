/*
 * @Author: xialc
 * @Email:
 * @Date: 2026-01-14 16:18:31
 * @LastEditTime: 2026-01-14 16:51:16
 * @FilePath: /kk_frame/src/app/page/components/button_group.cc
 * @Description: 按键组
 *               SingleChoice:单选  MultiChoice:多选
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "button_group.h"

SingleChoiceG::SingleChoiceG() {
    mLastID = 0;
}

SingleChoiceG::~SingleChoiceG() {
}

/// @brief 添加按键进行托管
/// @param view 按键
/// @return true:成功 false:失败
bool SingleChoiceG::addView(View* view) {
    if (!view) {
        LOG(ERROR) << "button is null";
        return false;
    }
    auto it = mButtons.find(view->getId());
    if (it != mButtons.end()) {
        LOG(ERROR) << "button id exists. id=" << view->getId();
        return false;
    }
    view->setOnClickListener([this](View& v) { onButtonClick(v); });
    mButtons.insert(std::make_pair(view->getId(), view));
    return true;
}

/// @brief 设置按键组字符串表
/// @param values 字符串表
void SingleChoiceG::setStrMap(const ButtonSTRValues& values) {
    mSTRValues = values;
}

/// @brief 设置按键组整型表
/// @param values 整型表
void SingleChoiceG::setIntMap(const ButtonINTValues& values) {
    mINTValues = values;
}

/// @brief 设置按键组监听器
/// @param l 监听
void SingleChoiceG::setOnClickListener(View::OnClickListener l) {
    mOnClickListener = l;
}

/// @brief 设置点击的控件
/// @param id 控件id
/// @return true:成功 false:失败
bool SingleChoiceG::clickView(int id) {
    bool find_view = false;
    for (auto& kv : mButtons) {
        if (kv.second->getId() == id) {
            find_view = true;
            if (kv.second->isEnabled()) { kv.second->setEnabled(false); }
            mLastID = id;
            continue;
        }
        if (!kv.second->isEnabled()) { kv.second->setEnabled(true); }
    }
    if (!find_view) { LOG(ERROR) << "not find button id. ID=" << id; }
    return find_view;
}

/// @brief 获取当前选中项ID
/// @return 当前选中项ID
int SingleChoiceG::getID() {
    return mLastID;
}

/// @brief 获取当前选中项对应字符串
/// @return 当前选中项对应字符串
std::string SingleChoiceG::getString() {
    auto it = mSTRValues.find(mLastID);
    if (it != mSTRValues.end()) return it->second;
    return "";
}

/// @brief 获取当前选中项对应整型
/// @return 当前选中项对应整型
int SingleChoiceG::getInt() {
    auto it = mINTValues.find(mLastID);
    if (it != mINTValues.end()) return it->second;
    return -1;
}

/// @brief 获取指定ID的按键
/// @param id 按键ID
/// @return 按键
View* SingleChoiceG::get(int id) {
    return mButtons[id];
}

/// @brief 获取按键组
/// @return 按键组
SingleChoiceG::ButtonViews* SingleChoiceG::getButtons() {
    return &mButtons;
}

/// @brief 按键点击事件
/// @param v 按键
void SingleChoiceG::onButtonClick(View& v) {
    clickView(v.getId());
    if (mOnClickListener)mOnClickListener(v);
}
