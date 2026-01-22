/*
 * @Author: Ricken
 * @Email:
 * @Date: 2026-01-14 16:18:31
 * @LastEditTime: 2026-01-23 00:14:35
 * @FilePath: /kk_frame/src/class/button_group.cc
 * @Description: 按键组(灵感来源于xialc)
 *               SingleChoice:单选  MultiChoice:多选
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "button_group.h"

// ==================== SingleChoiceG 实现 ====================

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
void SingleChoiceG::setOnClickListener(OnItemCLickListener l) {
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
View* SingleChoiceG::getView(int id) {
    auto it = mButtons.find(id);
    if (it != mButtons.end()) return it->second;
    return nullptr;
}

/// @brief 获取按键组
/// @return 按键组
SingleChoiceG::ButtonViews* SingleChoiceG::getButtons() {
    return &mButtons;
}

/// @brief 按键点击事件
/// @param v 按键
void SingleChoiceG::onButtonClick(View& v) {
    if (
        mOnClickListener &&
        !mOnClickListener(getID(), v.getId())
        ) return;
    clickView(v.getId());
}

// ==================== MultiChoiceG 实现 ====================

MultiChoiceG::MultiChoiceG() {
}

MultiChoiceG::~MultiChoiceG() {
}

/// @brief 添加按键进行托管
/// @param view 按键
/// @return true:成功 false:失败
bool MultiChoiceG::addView(View* view) {
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
void MultiChoiceG::setStrMap(const ButtonSTRValues& values) {
    mSTRValues = values;
}

/// @brief 设置按键组整型表
/// @param values 整型表
void MultiChoiceG::setIntMap(const ButtonINTValues& values) {
    mINTValues = values;
}

/// @brief 设置按键组监听器
/// @param l 监听
void MultiChoiceG::setOnClickListener(OnItemCLickListener l) {
    mOnClickListener = l;
}

/// @brief 设置指定ID的选中状态
/// @param id 控件id
/// @param selected 是否选中
/// @return true:成功 false:失败
bool MultiChoiceG::selectView(int id, bool selected) {
    auto it = mButtons.find(id);
    if (it == mButtons.end()) {
        LOG(ERROR) << "not find button id. ID=" << id;
        return false;
    }

    View* view = it->second;
    bool currentlySelected = (mSelectedIDs.find(id) != mSelectedIDs.end());

    if (selected && !currentlySelected) {
        mSelectedIDs.insert(id);
        view->setActivated(true);
    } else if (!selected && currentlySelected) {
        mSelectedIDs.erase(id);
        view->setActivated(false);
    }

    return true;
}

/// @brief 切换指定ID的选中状态
/// @param id 控件id
/// @return true:成功 false:失败
bool MultiChoiceG::toggleView(int id) {
    auto it = mButtons.find(id);
    if (it == mButtons.end()) {
        LOG(ERROR) << "not find button id. ID=" << id;
        return false;
    }

    View* view = it->second;
    bool currentlySelected = (mSelectedIDs.find(id) != mSelectedIDs.end());

    if (currentlySelected) {
        mSelectedIDs.erase(id);
        if (view->isActivated())
            view->setActivated(false);
    } else {
        mSelectedIDs.insert(id);
        if (!view->isActivated())
            view->setActivated(true);
    }

    return true;
}

/// @brief 清除所有选中项
void MultiChoiceG::clearSelection() {
    for (auto id : mSelectedIDs) {
        auto it = mButtons.find(id);
        if (it != mButtons.end())
            it->second->setActivated(false);
    }
    mSelectedIDs.clear();
}

/// @brief 判断指定ID是否被选中
/// @param id 控件id
/// @return true:选中 false:未选中
bool MultiChoiceG::isSelected(int id) const {
    return mSelectedIDs.find(id) != mSelectedIDs.end();
}

/// @brief 获取所有选中项的ID集合
/// @return 选中项ID集合
MultiChoiceG::SelectedSet MultiChoiceG::getSelectedIDs() const {
    return mSelectedIDs;
}

/// @brief 获取所有选中项对应的字符串
/// @return 选中项字符串列表
std::vector<std::string> MultiChoiceG::getSelectedStrings() const {
    std::vector<std::string> result;
    for (auto id : mSelectedIDs) {
        auto it = mSTRValues.find(id);
        if (it != mSTRValues.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

/// @brief 获取所有选中项对应的整型值
/// @return 选中项整型值列表
std::vector<int> MultiChoiceG::getSelectedInts() const {
    std::vector<int> result;
    for (auto id : mSelectedIDs) {
        auto it = mINTValues.find(id);
        if (it != mINTValues.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

/// @brief 获取指定ID的按键
/// @param id 按键ID
/// @return 按键
View* MultiChoiceG::getView(int id) {
    auto it = mButtons.find(id);
    if (it != mButtons.end()) return it->second;
    return nullptr;
}

/// @brief 获取按键组
/// @return 按键组
MultiChoiceG::ButtonViews* MultiChoiceG::getButtons() {
    return &mButtons;
}

/// @brief 获取选中项数量
/// @return 选中项数量
size_t MultiChoiceG::getSelectedCount() const {
    return mSelectedIDs.size();
}

/// @brief 按键点击事件
/// @param v 按键
void MultiChoiceG::onButtonClick(View& v) {
    int id = v.getId();
    bool oldSelected = isSelected(id);
    bool newSelected = !oldSelected;

    if (
        mOnClickListener &&
        !mOnClickListener(id, newSelected)
        ) return;
    toggleView(id);
}