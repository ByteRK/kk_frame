/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-03-18 11:59:22
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_cn.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_cn.h"
#include "config_info.h"

#if defined(ENABLE_KEYBOARD_PINYIN)
#include <pinyinime.h>
#include <utils/textutils.h>
#include <unistd.h>
#define PINYIN_DAT_PATH LOCAL_DATA_DIR "pinyin/"
#endif


Keyboard_CN::Keyboard_CN(CKeyBoard* parent) :Keyboard_EN(parent, "@keyboard:layout/keyboard_cn") {
    for (uint8_t i = 0; i < DISPLAY_TYPE_MAX; i++) {
        mKeyStr[i][KEY_EN_LANG] = "拼音";
        mKeyStr[i][KEY_EN_LEFT] = "，";
        mKeyStr[i][KEY_EN_RIGHT] = "。";

        // TODO

        // {{
        //     {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
        //     {"-", "/", "：", "；", "（", "）", "@", "“", "”"},
        //     {"…", "～", "、", "？", "！", ".", "—"}
        // }},
        // {{
        //     {"【", "】", "｛", "｝", "#", "%", "^", "*", "+", "="},
        //     {"_", "\\", "|", "《", "》", "￥", "$", "&", "·"},
        //     {"…", "～", "｀", "？", "！", ".", "’"}
        // }},
    }

    LOGI("Keyboard_CN::Keyboard_CN() Created");
}

Keyboard_CN::~Keyboard_CN() {
    pinyinClose();
}

CKeyBoard::KeyBoardType Keyboard_CN::getType() {
    return CKeyBoard::KB_TYPE_CN;
}

void Keyboard_CN::init() {
    Keyboard_EN::init();
    mCandidateBoxes = __dc(ViewGroup, mRootView->findViewById(LibRid::candidate_box));
    mCandidateBoxes->setEnabled(false);

    mPinyin = __dc(TextView, mRootView->findViewById(LibRid::pinyin));
    mCandidateList = __dc(RecyclerView, mRootView->findViewById(LibRid::candidate_list));

    mCandidateList->setLayoutManager(new LinearLayoutManager(mRootView->getContext(), LinearLayoutManager::HORIZONTAL, false));
    mCandidateList->setAdapter(this);
}

void Keyboard_CN::onShow() {
    Keyboard_EN::onShow();
    pinyinOpen();
    clearCandidate();
}

void Keyboard_CN::onKeyClick(int key) {
    if (mDisplayType != DISPLAY_TYPE_DEFAULT || (key > KEY_EN_M && key != KEY_EN_BACKSPACE)) {
        // 非拼音，不接管
        clearCandidate();
        Keyboard_EN::onKeyClick(key);
        return;
    }

    if (key == KEY_EN_BACKSPACE) {
        mPinyin->getText().size() ? pinyinDel() : Keyboard_EN::onKeyClick(key);
    } else {
        pinyinAdd(mKeyStr[mDisplayType][key]);
    }
}

int Keyboard_CN::getItemCount() {
    return mCandidateListData.size();
}

RecyclerView::ViewHolder* Keyboard_CN::onCreateViewHolder(ViewGroup* parent, int viewType) {
    TextView* item = __dc(TextView, LayoutInflater::from(parent->getContext())->inflate("@keyboard:layout/keyboard_cn_candidate", parent, false));
    item->setOnClickListener([this](View &v) {
        mParent->appendText(__dc(TextView, &v)->getText());
        clearCandidate();
    });
    return new RecyclerView::ViewHolder(item);
}

void Keyboard_CN::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    TextView* item = __dc(TextView, holder.itemView);
    item->setText(position < mCandidateListData.size() ? mCandidateListData[position] : "OUT_OF_RANGE");
}

void Keyboard_CN::pinyinOpen() {
    if (mPinyinhandle) return;
#if defined(ENABLE_KEYBOARD_PINYIN)
    if (access(PINYIN_DAT_PATH "dict_pinyin.dat", F_OK) != 0) LOGE("pinyin dat[%s] file not exist", PINYIN_DAT_PATH "dict_pinyin.dat");
    if (access(PINYIN_DAT_PATH "user.dat", F_OK) != 0) LOGE("pinyin dat[%s] file not exist", PINYIN_DAT_PATH "user.dat");

    mPinyinhandle = ime_pinyin::im_open_decoder(PINYIN_DAT_PATH "dict_pinyin.dat", PINYIN_DAT_PATH "user.dat");
    LOGI("pinyinOpen: %p", mPinyinhandle);
#else
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinClose() {
    if (!mPinyinhandle) return;
#if defined(ENABLE_KEYBOARD_PINYIN)
    ime_pinyin::im_close_decoder(mPinyinhandle);
    mPinyinhandle = nullptr;
#endif
}

void Keyboard_CN::pinyinAdd(const char* pinyin) {
    std::string nowPinyin = mPinyin->getText();
#if defined(ENABLE_KEYBOARD_PINYIN)
    if (nowPinyin.empty()) {
        ime_pinyin::im_flush_cache(mPinyinhandle);
        ime_pinyin::im_reset_search(mPinyinhandle);
    }
    nowPinyin.push_back(*pinyin);
    pinyinSearch(nowPinyin);
#else
    nowPinyin.push_back(*pinyin);
    mPinyin->setText(nowPinyin);
    mCandidateListData.clear();
    mCandidateListData.push_back(nowPinyin);
    notifyDataSetChanged();
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinDel() {
    std::string nowPinyin = mPinyin->getText();
    if (nowPinyin.size() <= 1) {
        clearCandidate();
        return;
    }
    nowPinyin.pop_back();
#if defined(ENABLE_KEYBOARD_PINYIN)
    pinyinSearch(nowPinyin);
#else
    mPinyin->setText(nowPinyin);
    mCandidateListData.clear();
    mCandidateListData.push_back(nowPinyin);
    notifyDataSetChanged();
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinSearch(const std::string& pinyin) {
#if defined(ENABLE_KEYBOARD_PINYIN)

#define MAX_CANDIDATE_SIZE 80
#define MAX_CANDIDATE_BUFF 128

    size_t resSize = ime_pinyin::im_search(mPinyinhandle, pinyin.c_str(), pinyin.length());
    size_t needResSize = resSize > MAX_CANDIDATE_SIZE ? MAX_CANDIDATE_SIZE : resSize;
    LOGD("search [%s] -> %d/%d", pinyin.c_str(), needResSize, resSize);

    mPinyin->setText(pinyin);
    mCandidateListData.clear();

    static uint16_t scanbuf[MAX_CANDIDATE_BUFF] = { 0 };
    for (size_t i = 0; i < needResSize; i++) {
        uint16_t* scan = ime_pinyin::im_get_candidate(mPinyinhandle, i, scanbuf, MAX_CANDIDATE_BUFF);
        std::string u8s = cdroid::TextUtils::utf16_utf8(scan, ime_pinyin::utf16_strlen(scan));
        if (u8s.size())mCandidateListData.push_back(u8s);
    }
    notifyDataSetChanged();

#undef MAX_CANDIDATE_SIZE
#undef MAX_CANDIDATE_BUFF

#else
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::clearCandidate() {
    mPinyin->setText("");
    if (mCandidateListData.size()) {
        mCandidateListData.clear();
        notifyDataSetChanged();
    }
}
