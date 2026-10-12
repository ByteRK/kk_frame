/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-10-12 00:00:00
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

#include <cstdint>
#include <widgetEx/recyclerview/recyclerview.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>

/// @brief 中文键盘
class Keyboard_CN : public Keyboard_EN, public RecyclerView::Adapter {
private:
    static constexpr size_t CANDIDATE_MAX_SIZE = 80;    // 单次最多展示的候选词数量
    static constexpr size_t CANDIDATE_BUFF_SIZE = 128;  // 单个候选词的最大 utf16 长度

    /// @brief 拼音原串长度上限
    /// @note 引擎内的标记表容量固定（见 cdroid 的 dicttrie.h：kMaxMileStone = 100、
    ///       kMaxParsingMark = 600），而标记占用随原串长度与歧义度成倍增长：
    ///       连续键入同一字母（如 zzz…）时，约第 15 个字母就会耗尽标记表，
    ///       其后 extend_dict2() 会按越界的 mark_start 读取标记并段错误（实测 15 个 z 必崩）。
    ///       这是引擎侧缺陷，客户端只能在入口限制长度，故取实测安全值（并留有余量）
    static constexpr size_t PINYIN_MAX_LEN = 13;

private:
    ViewGroup*    mCandidateBoxes{ nullptr }; // 候选框
    TextView*     mPinyin{ nullptr };         // 拼音显示（带音节分隔符，如 ni'hao）
    RecyclerView* mCandidateList{ nullptr };  // 候选框列表

    void*                    mPinyinhandle{ nullptr }; // 拼音引擎句柄
    bool                     mPinyinUsable{ false };   // 引擎是否真的可用（未初始化时不能 close）
    std::string              mPinyinRaw{ "" };         // 拼音原串（不含分隔符，作为检索与删除的依据）
    std::vector<std::string> mCandidateListData;       // 候选框列表数据
    std::vector<uint16_t>    mScanBuffer;              // 候选词读取缓冲（成员而非静态，避免多实例共享）

public:
    Keyboard_CN(CKeyBoard* parent);
    ~Keyboard_CN();

protected:
    CKeyBoard::KeyBoardType getType() override;
    void init() override;
    void onShow() override;
    void onHide() override;
    void onKeyClick(int key) override;
    void onBackspaceLongPress() override;
    void onTextCleared() override;

protected:
    int getItemCount() override;
    RecyclerView::ViewHolder* onCreateViewHolder(ViewGroup* parent, int viewType) override;
    void onBindViewHolder(RecyclerView::ViewHolder& holder, int position) override;

private:
    void pinyinOpen();
    void pinyinClose();
    void pinyinAdd(const std::string& pinyin);
    void pinyinDel();
    void pinyinSearch(const std::string& pinyin);
    /// @brief 无拼音引擎时的兜底显示（原串即唯一候选）
    void pinyinShowRaw();

    void clearCandidate();
};

#endif // __KEYBOARD_CN_H__