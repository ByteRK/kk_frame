/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2025-11-28 11:24:16
 * @LastEditTime: 2025-12-01 19:07:51
 * @FilePath: /kk_frame/src/viewlibs/kk_keyboard.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2025 by Ricken, All Rights Reserved.
 *
**/

#ifndef KEYBOARD_DISABLE // CMakeLists.txt -> add_definitions(-DKEYBOARD_ENABLE)

#include "R.h"
#include <comm_func.h>
#include "kk_keyboard.h"

# if ENABLE(PINYIN2HZ)
#   include <core/inputmethodmanager.h>
#   ifndef __VSCODE__
#     include <pinyin/pinyinime.h>
#   endif
#   ifdef CDROID_X64
#     define PINYIN_DATA_FILE "dict_pinyin.dat"
#     define USERDICT_FILE "userdict"
#   else
#     define PINYIN_DATA_FILE "dict_pinyin_arm.dat"
#     define USERDICT_FILE "userdict"
#   endif 
# endif

#define ENTERTEXT_COLOR 0xFFFFFFFF
#define DESCRIPTION_COLOR 0x4CFFFFFF

#define ENTERTEXT_SIZE 24
#define DESCRIPTION_SIZE 24

#if ENABLE(PINYIN2HZ)
GooglePinyin* gObjPinyin = nullptr;
std::set<int> gLetterKeys;
#endif
//////////////////////////////////////////////////////////////////////////

/// 注册键盘
DECLARE_WIDGET(KKKeyBoard)

/// @brief 手动构建
/// @param w 
/// @param h 
KKKeyBoard::KKKeyBoard(int w, int h) : RelativeLayout(w, h) {

    initKBDatebase();
#if ENABLE(PINYIN2HZ)
    gObjPinyin = (GooglePinyin*)InputMethodManager::getInstance().getInputMethod("GooglePinyin26");
#endif
}

/// @brief XML构建
/// @param ctx 
/// @param attr 
KKKeyBoard::KKKeyBoard(Context* ctx, const AttributeSet& attr) : RelativeLayout(ctx, attr) {

    initKBDatebase();
#if ENABLE(PINYIN2HZ)
    gObjPinyin = (GooglePinyin*)InputMethodManager::getInstance().getInputMethod("GooglePinyin26");
#endif
}

KKKeyBoard::~KKKeyBoard() {
}

/// @brief 显示键盘
/// @note 当键盘为全局时，需要调用此方法显示键盘
void KKKeyBoard::show() {
}

/// @brief 隐藏键盘
/// @note 当键盘为全局时，需要调用此方法隐藏键盘
void KKKeyBoard::hide() {
}

void KKKeyBoard::setType(KBTYPE type) {
}

void KKKeyBoard::setEditType(EditText::INPUTTYPE editType) {
}

/// @brief 设置输入字最大长度
/// @param count 
void KKKeyBoard::setWordMaxCount(int count) {
    mWordMaxCount = count;

}

void KKKeyBoard::setDescText(const std::string& txt) {
}

void KKKeyBoard::setDefaultText(const std::string& txt) {
}

void KKKeyBoard::setOnCompleteListener(OnCompleteListener l) {
}

void KKKeyBoard::setOnCancelListener(OnCancelListener l) {
}

std::string KKKeyBoard::getEnterText() {
    return mEnterText;
}

/// @brief 初始化键盘数据
/// @note 如果需要根据系统语言进行键盘切换，建议在这里进行处理
void KKKeyBoard::initKBDatebase() {
    mKBDatabase = {
    KeyboardConfig{
        KB_TYPE_EN, "英文", true,
        {{
            {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
            {"a", "s", "d", "f", "g", "h", "j", "k", "l"},
            {"z", "x", "c", "v", "b", "n", "m"}
        }},
        {{
            {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
            {"A", "S", "D", "F", "G", "H", "J", "K", "L"},
            {"Z", "X", "C", "V", "B", "N", "M"}
        }},
        {{
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"-", "/", ":", ";", "(", ")", "_", "$", "&"},
            {"~", ",", "…", "@", "!", "'", "\""}
        }},
        {{
            {"[", "]", "{", "}", "#", "%", "^", "*", "+", "="},
            {"ˇ", "/", "\\", "<", ">", "￥", "€", "£", "₤"},
            {"~", ",", ".", "@", "!", "`", "\""}
        }},
        {"大写", "小写", "?123", "返回", "更多", "123", ".", "空格", "?", "回车"}
    },
    KeyboardConfig{
        KB_TYPE_CN, "中文", false,
        {{
            {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
            {"a", "s", "d", "f", "g", "h", "j", "k", "l"},
            {"z", "x", "c", "v", "b", "n", "m"}
        }},
        {{
            {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
            {"A", "S", "D", "F", "G", "H", "J", "K", "L"},
            {"Z", "X", "C", "V", "B", "N", "M"}
        }},
        {{
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"-", "/", "：", "；", "（", "）", "@", "“", "”"},
            {"…", "～", "、", "？", "！", ".", "—"}
        }},
        {{
            {"【", "】", "｛", "｝", "#", "%", "^", "*", "+", "="},
            {"_", "\\", "|", "《", "》", "￥", "$", "&", "·"},
            {"…", "～", "｀", "？", "！", ".", "’"}
        }},
        {"大写", "小写", "?123", "返回", "更多", "123",  "，", "空格", "。", "回车"}
    }
    };
}

void KKKeyBoard::setText(const std::string& txt) {
    int wordCount = wordLen(txt.c_str());
    if (mWordCount > 0 && wordCount > mWordCount) {
        mLastTxt = getWord(txt.c_str(), mWordCount);
        setText(mLastTxt, mText, 40, 16, 34);
    } else {
        setText(txt, mText, 40, 16, 34);
    }
    LOGI("txt.size() = %d  wordCount = %d/%d", txt.size(), wordCount, mWordCount);
}

void KKKeyBoard::setText(const std::string& txt, TextView* txtView, int maxLen, int minSize, int maxSize) {
    txtView->setTextSize(txt.size() >= maxLen ? (((maxSize - (int)(txt.size() - maxLen)) >= minSize ? (maxSize - (int)(txt.size() - maxLen)) : minSize)) : maxSize);
    if (txtView == mText) {
        setEnterText(txt);
    } else {
        txtView->setText(txt);
        txtView->requestLayout();
    }
}

#endif // KEYBOARD_ENABLE