/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-03-18 00:41:31
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_cn.h
 * @Description: 中文键盘
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_CN_H__
#define __KEYBOARD_CN_H__

#include "keyboard_en.h"

#include <widgetEx/recyclerview/recyclerview.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>

/// @brief 中文键盘
class Keyboard_CN : public Keyboard_EN, public RecyclerView::Adapter {
private:
    ViewGroup*    mCandidateBoxes{ nullptr }; // 候选框
    TextView*     mPinyin{ nullptr };         // 拼音
    RecyclerView* mCandidateList{ nullptr };  // 候选框列表

    void*                    mPinyinhandle{ nullptr }; // 拼音引擎句柄
    std::vector<std::string> mCandidateListData;       // 候选框列表数据

public:
    Keyboard_CN(CKeyBoard* parent);
    ~Keyboard_CN();

protected:
    CKeyBoard::KeyBoardType getType() override;
    void init() override;
    void onShow() override;
    void onKeyClick(int key) override;

protected:
    int getItemCount() override;
    RecyclerView::ViewHolder* onCreateViewHolder(ViewGroup* parent, int viewType) override;
    void onBindViewHolder(RecyclerView::ViewHolder& holder, int position) override;

private:
    void pinyinOpen();
    void pinyinClose();
    void pinyinAdd(const char* pinyin);
    void pinyinDel();
    void pinyinSearch(const std::string& pinyin);
    
    void clearCandidate();
};

#endif // __KEYBOARD_CN_H__