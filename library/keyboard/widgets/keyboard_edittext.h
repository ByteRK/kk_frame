/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-23 00:00:00
 * @LastEditTime: 2026-09-23 00:00:00
 * @FilePath: /kk_frame/library/keyboard/widgets/keyboard_edittext.h
 * @Description: 键盘输入框
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __KEYBOARD_EDITTEXT_H__
#define __KEYBOARD_EDITTEXT_H__

#include <widget/edittext.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/// @brief 键盘输入框
/// @note 相对引擎 EditText 补齐两点：
///       1. 自绘细光标：引擎光标宽度取"光标所在字符的宽度"，且在文本末尾不绘制，
///          导致光标过粗、且末尾无光标；
///       2. 支持点击定位光标，便于在文本中间插入。
///       闪烁由独立定时器驱动并整控件重绘，避免局部重绘（引擎按字符宽度失效一块区域）
///       与绘制驱动翻转（引擎在 onDraw 末尾翻转状态）叠加造成的残影。
class KeyboardEditText : public EditText {
public:
    /// @brief 光标位置变化回调（仅由用户点击触发），参数为宽字符索引
    typedef std::function<void(int)> OnCaretChangeListener;

    static constexpr int DEFAULT_CURSOR_WIDTH = 2;    // 光标线宽(px)
    static constexpr int DEFAULT_BLINK_PERIOD = 500;  // 闪烁周期(ms)
    static constexpr int DEFAULT_IDLE_DELAY = 1000;   // 输入后暂停闪烁的时长(ms)

public:
    KeyboardEditText(int w, int h);
    KeyboardEditText(Context* ctx, const AttributeSet& attr);
    ~KeyboardEditText();

    /// @brief 设置光标偏移（宽字符索引，0 表示最前，内容长度表示最后），不触发回调
    void setCaretOffset(int offset);
    /// @brief 获取光标偏移（宽字符索引）
    int  getCaretOffset() const;
    /// @brief 获取内容长度（宽字符数）
    int  getContentLength() const;

    void setOnCaretChangeListener(OnCaretChangeListener listener);

protected:
    void onDraw(Canvas& canvas) override;
    /// @brief 屏蔽引擎光标（宽度为字符宽，与自绘冲突）
    void onDrawCaret(Canvas& canvas, const Rect& r) override;
    bool onTouchEvent(MotionEvent& evt) override;
    void onFocusChanged(bool focus, int direction, Rect* prevfocusrect) override;
    void onVisibilityChanged(View& changedView, int visibility) override;
    void onDetachedFromWindow() override;

private:
    /// @brief 光标当前是否允许显示/闪烁
    bool canBlink() const;
    void startBlink();
    void stopBlink();
    /// @brief 记录一次输入：点亮光标并暂停闪烁，让输入过程中光标保持稳定
    void markInput();
    void onBlinkTick();

    /// @brief 建立"宽字符索引 -> 相对文本起点的 x"表（仅在文本或字号变化时重建）
    void buildCaretTable(Canvas& canvas, const std::string& text);
    /// @brief 索引 i 处光标相对文本起点的 x
    int  caretXInText(int index) const;
    /// @brief 由 x（视图坐标）反查最近的宽字符索引
    int  indexAtX(int x) const;
    /// @brief 光标纵向范围（含内边距与 gravity，与文本绘制位置一致）
    void cursorVerticalRange(int& top, int& height) const;
    void drawCursor(Canvas& canvas);

private:
    int mCursorWidth{ DEFAULT_CURSOR_WIDTH };  // 光标线宽
    int mCursorColor{ 0 };                     // 光标颜色，0 表示跟随文字颜色
    int mBlinkPeriod{ DEFAULT_BLINK_PERIOD };  // 闪烁周期
    int mIdleDelay{ DEFAULT_IDLE_DELAY };      // 输入后暂停闪烁时长

    bool    mCursorOn{ true };         // 光标当前是否点亮
    int64_t mLastInputTime{ 0 };       // 最近一次输入时间
    Runnable mBlinkTimer;              // 闪烁定时器

    std::vector<int> mCaretXTable;     // 宽字符索引 -> 相对文本起点的 x
    std::string      mTableText;       // 建表时的文本
    int              mTableFontSize{ 0 };// 建表时的字号
    int              mTextOriginX{ 0 };  // 文本起点在视图中的 x

    OnCaretChangeListener mCaretChangeListener{ nullptr };
};

#endif // __KEYBOARD_EDITTEXT_H__
