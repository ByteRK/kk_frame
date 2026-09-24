/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-22 00:00:00
 * @LastEditTime: 2026-09-22 00:00:00
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_qwerty.h
 * @Description: QWERTY 类键盘基类
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_QWERTY_H__
#define __KEYBOARD_QWERTY_H__

#include "keyboard_base.h"

#include <initializer_list>
#include <string>
#include <vector>

/// @brief QWERTY 类键盘基类（英文/俄文等共用）
/// @note 按键排列约定，各子类在 collectKeys() 中必须按此顺序注册：
///       [字母/符号区 mMainKeyCount 个] [左] [空格] [右] [符号1] [符号2] [语言] [大小写] [退格]
class Keyboard_Qwerty : public CKeyBoardChild {
protected:
    typedef enum {
        DISPLAY_TYPE_DEFAULT = 0,   // 常规
        DISPLAY_TYPE_UPPER,         // 大写
        DISPLAY_TYPE_UPPER_PLUS,    // 大写锁定
        DISPLAY_TYPE_NUMBER,        // 数字
        DISPLAY_TYPE_MORE,          // 更多

        DISPLAY_TYPE_MAX
    } DISPLAY_TYPE;

    /// @brief 尾部按键数量：左/空格/右 + 符号1/符号2/语言/大小写/退格
    static constexpr int TAIL_KEY_COUNT = 8;
    /// @brief 退格键长按判定时间(ms)
    static constexpr int BACKSPACE_LONG_PRESS_MS = 1200;

public:
    /// @param mainKeyCount 字母/符号区按键数量（即尾部按键之前的所有按键）
    Keyboard_Qwerty(CKeyBoard* parent, const std::string& layout, int mainKeyCount);
    virtual ~Keyboard_Qwerty();

protected:  // CKeyBoardChild
    virtual void init() override;
    virtual void onShow() override;
    virtual void onHide() override;

protected:
    /// @brief 收集按键：资源ID -> mKeyList 下标，实现顺序必须与按键表一致
    virtual void collectKeys() = 0;
    /// @brief 按键响应
    virtual void onKeyClick(int key);
    /// @brief 退格键长按（默认清除光标前的内容，子键盘可重写）
    virtual void onBackspaceLongPress();
    /// @brief 刷新按键显示（文案 + 功能键状态）
    virtual void refreshDisplay();

    /// @brief 设置指定显示模式的按键文案
    void setKeyStr(DISPLAY_TYPE type, std::initializer_list<const char*> keys);
    /// @brief 复制按键文案（如大写与大写锁定共用一份）
    void copyKeyStr(DISPLAY_TYPE from, DISPLAY_TYPE to);
    /// @brief 获取指定按键在当前显示模式下的文案（带边界保护）
    const std::string& keyStr(int key) const;

    /// @brief 是否为字母/符号区按键
    bool isLetterKey(int key) const { return key >= 0 && key < mMainKeyCount; }
    /// @brief 是否为常规按键（字母/符号区 + 左/空格/右）
    bool isMainKey(int key) const { return key >= 0 && key < mMainKeyCount + 3; }

    // 尾部按键下标
    int idxLeft() const { return mMainKeyCount; }
    int idxSpace() const { return mMainKeyCount + 1; }
    int idxRight() const { return mMainKeyCount + 2; }
    int idxMath() const { return mMainKeyCount + 3; }
    int idxMore() const { return mMainKeyCount + 4; }
    int idxLang() const { return mMainKeyCount + 5; }
    int idxShift() const { return mMainKeyCount + 6; }
    int idxBackspace() const { return mMainKeyCount + 7; }

protected:
    DISPLAY_TYPE mDisplayType{ DISPLAY_TYPE_DEFAULT };  // 显示模式
    std::vector<Button*> mKeyList;                      // 按键列表，顺序与按键表一致
    std::vector<std::vector<std::string>> mKeyStr;      // 按键文案[显示模式][按键下标]
    int mMainKeyCount{ 0 };                             // 字母/符号区按键数量
    int64_t mShiftLastClickTime{ 0 };                   // 上次大小写键点击时间
    Runnable mBackspaceTimer;                           // 退格键长按判定计时
    bool mBackspaceHandled{ false };                    // 本次长按是否已处理（避免抬手时重复退格）

private:
    void setKeyAction();
    void updateKeyText();
    /// @brief 长按判定到时：执行长按动作
    void onBackspaceTimer();
    /// @brief 停止长按判定（抬手/取消/隐藏时调用）
    void stopBackspaceTimer();
};

#endif // __KEYBOARD_QWERTY_H__
