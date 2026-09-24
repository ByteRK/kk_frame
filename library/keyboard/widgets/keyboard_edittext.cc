/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-23 00:00:00
 * @LastEditTime: 2026-09-23 00:00:00
 * @FilePath: /kk_frame/library/keyboard/widgets/keyboard_edittext.cc
 * @Description: 键盘输入框
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_edittext.h"

#include "quick_define.h"

#include <core/textutils.h>
#include <view/gravity.h>

DECLARE_WIDGET(KeyboardEditText)

/// @brief 取光标纵向内缩量，让光标比行高略短
static int cursorInset(int height) {
    const int inset = height / 8;
    return inset > 0 ? inset : 1;
}

KeyboardEditText::KeyboardEditText(int w, int h) : EditText(w, h) {
    mBlinkTimer = std::bind(&KeyboardEditText::onBlinkTick, this);
    // 光标定位基于单行累加宽度，且键盘输入框本就只应单行
    setSingleLine(true);
}

KeyboardEditText::KeyboardEditText(Context* ctx, const AttributeSet& attr) : EditText(ctx, attr) {
    mBlinkTimer = std::bind(&KeyboardEditText::onBlinkTick, this);
    setSingleLine(true);

    // 光标配置（均在 xml 中可配）
    mCursorWidth = attr.getDimensionPixelSize("cursorWidth", DEFAULT_CURSOR_WIDTH);
    if (mCursorWidth < 1)mCursorWidth = DEFAULT_CURSOR_WIDTH;
    mCursorColor = attr.getColor("cursorColor", 0);
    mBlinkPeriod = attr.getInt("cursorBlinkPeriod", DEFAULT_BLINK_PERIOD);
    if (mBlinkPeriod < 100)mBlinkPeriod = DEFAULT_BLINK_PERIOD;
    mIdleDelay = attr.getInt("cursorBlinkIdle", DEFAULT_IDLE_DELAY);
    if (mIdleDelay < 0)mIdleDelay = DEFAULT_IDLE_DELAY;

    LOGI("KeyboardEditText: width=%d color=0x%x period=%d idle=%d",
        mCursorWidth, mCursorColor, mBlinkPeriod, mIdleDelay);
}

KeyboardEditText::~KeyboardEditText() {
    stopBlink();
}

void KeyboardEditText::setCaretOffset(int offset) {
    const int len = getContentLength();
    if (offset < 0)offset = 0;
    else if (offset > len)offset = len;
    if (offset == mCaretPos)return;

    setCaretPos(offset);
    markInput();
    invalidate(true);
}

int KeyboardEditText::getCaretOffset() const {
    return mCaretPos;
}

int KeyboardEditText::getContentLength() const {
    return mLayout ? (int)mLayout->getText().size() : 0;
}

void KeyboardEditText::setOnCaretChangeListener(OnCaretChangeListener listener) {
    mCaretChangeListener = listener;
}

void KeyboardEditText::onDrawCaret(Canvas& canvas, const Rect& r) {
    // 引擎光标按字符宽度绘制且在文本末尾不绘制，这里全部交给自绘光标
}

bool KeyboardEditText::canBlink() const {
    return isFocused() && isEnabled() && isShown() && (getContentLength() >= 0);
}

void KeyboardEditText::startBlink() {
    stopBlink();
    mCursorOn = true;
    postDelayed(mBlinkTimer, mBlinkPeriod);
}

void KeyboardEditText::stopBlink() {
    removeCallbacks(mBlinkTimer);
    mCursorOn = true;
}

void KeyboardEditText::markInput() {
    mLastInputTime = SystemClock::uptimeMillis();
    mCursorOn = true;
    // 输入中保持闪烁计时连续，避免输入结束后立刻进入半周期
    removeCallbacks(mBlinkTimer);
    postDelayed(mBlinkTimer, mBlinkPeriod);
}

void KeyboardEditText::onBlinkTick() {
    if (!canBlink()) {
        mCursorOn = true;
        invalidate(true);
        return;
    }

    // 输入中不闪烁：最近一次输入后 mIdleDelay 内保持常亮
    const int64_t now = SystemClock::uptimeMillis();
    if (now - mLastInputTime >= mIdleDelay)mCursorOn = !mCursorOn;
    else mCursorOn = true;

    invalidate(true);  // 整控件重绘，避免局部重绘留下的残影
    postDelayed(mBlinkTimer, mBlinkPeriod);
}

void KeyboardEditText::onFocusChanged(bool focus, int direction, Rect* prevfocusrect) {
    EditText::onFocusChanged(focus, direction, prevfocusrect);

    if (focus) {
        markInput();
        startBlink();
    } else {
        stopBlink();
        invalidate(true);
    }
}

void KeyboardEditText::onVisibilityChanged(View& changedView, int visibility) {
    EditText::onVisibilityChanged(changedView, visibility);

    // 不可见时停止闪烁，可见且已聚焦时恢复
    if (isShown() && isFocused()) startBlink();
    else if (!isShown()) stopBlink();
}

void KeyboardEditText::onDetachedFromWindow() {
    stopBlink();
    EditText::onDetachedFromWindow();
}

void KeyboardEditText::buildCaretTable(Canvas& canvas, const std::string& text) {
    if ((text == mTableText) && (mTableFontSize == (int)getTextSize()) && !mCaretXTable.empty())
        return;

    mTableText = text;
    mTableFontSize = (int)getTextSize();

    // 用与文本绘制一致的字体测量：Canvas 继承自 Cairo::Context，
    // 取 x_advance 而非 width —— width 不含尾随空格的进距
    canvas.set_font_size(getTextSize());
    Typeface* typeface = getTypeface();
    if (typeface)canvas.set_font_face(typeface->getFontFace()->get_font_face());

    const std::wstring wtext = cdroid::TextUtils::utf8tounicode(text);
    mCaretXTable.clear();
    mCaretXTable.reserve(wtext.size() + 1);
    mCaretXTable.push_back(0);

    Cairo::TextExtents te;
    for (size_t i = 1; i <= wtext.size(); i++) {
        canvas.get_text_extents(cdroid::TextUtils::unicode2utf8(wtext.substr(0, i)), te);
        mCaretXTable.push_back((int)(te.x_advance + 0.5));
    }
}

int KeyboardEditText::caretXInText(int index) const {
    if (mCaretXTable.empty() || index <= 0)return 0;
    if (index >= (int)mCaretXTable.size())return mCaretXTable.back();
    return mCaretXTable[index];
}

int KeyboardEditText::indexAtX(int x) const {
    if (mCaretXTable.size() < 2)return 0;

    const int local = x - mTextOriginX;
    if (local <= mCaretXTable.front())return 0;
    if (local >= mCaretXTable.back())return (int)mCaretXTable.size() - 1;

    // 就近取整：落在相邻两个位置的中线右侧则取后一个
    for (size_t i = 0; i + 1 < mCaretXTable.size(); i++) {
        const int left = mCaretXTable[i];
        const int right = mCaretXTable[i + 1];
        if (local < right)return (local - left) * 2 >= (right - left) ? (int)i + 1 : (int)i;
    }
    return (int)mCaretXTable.size() - 1;
}

void KeyboardEditText::cursorVerticalRange(int& top, int& height) const {
    const int boxHeight = getHeight() - getCompoundPaddingTop() - getCompoundPaddingBottom();
    height = mLayout ? mLayout->getLineHeight() : boxHeight;
    if (height <= 0)height = boxHeight;
    if (height <= 0)height = getHeight();

    top = getCompoundPaddingTop();
    const int textHeight = mLayout ? mLayout->getHeight(true) : height;
    if (((getGravity() & Gravity::VERTICAL_GRAVITY_MASK) == Gravity::CENTER_VERTICAL)
        && (textHeight < boxHeight))
        top += (boxHeight - textHeight) / 2;
    else if ((getGravity() & Gravity::VERTICAL_GRAVITY_MASK) == Gravity::BOTTOM && (textHeight < boxHeight))
        top += boxHeight - textHeight;
}

void KeyboardEditText::drawCursor(Canvas& canvas) {
    const int len = getContentLength();
    int caret = mCaretPos;
    if (caret < 0)caret = 0;
    else if (caret > len)caret = len;

    int x = mTextOriginX + caretXInText(caret);
    const int viewWidth = getWidth();
    if (x < 0)x = 0;
    else if (x >= viewWidth)x = viewWidth - 1;

    int top = 0, height = 0;
    cursorVerticalRange(top, height);
    const int inset = cursorInset(height);

    canvas.set_color(mCursorColor ? mCursorColor : (uint32_t)getCurrentTextColor());
    canvas.rectangle(x, top + inset, mCursorWidth, height - inset * 2);
    canvas.fill();
}

void KeyboardEditText::onDraw(Canvas& canvas) {
    // 文本与 hint 仍由引擎绘制（引擎光标已在 onDrawCaret 中被屏蔽）
    EditText::onDraw(canvas);

    buildCaretTable(canvas, getText());

    // 文本起点：引擎算好的光标 x 最准（含内边距与水平滚动偏移），
    // 它在文本末尾时不给光标矩形，此时退化为内边距
    int caret = mCaretPos;
    const int len = getContentLength();
    if (caret < 0)caret = 0;
    else if (caret > len)caret = len;
    if (!mCaretRect.empty() && (caret < (int)mCaretXTable.size()))
        mTextOriginX = mCaretRect.left - mCaretXTable[caret];
    else
        mTextOriginX = getCompoundPaddingLeft();

    if (canBlink() && mCursorOn)drawCursor(canvas);
}

bool KeyboardEditText::onTouchEvent(MotionEvent& evt) {
    const int action = evt.getActionMasked();
    if (action == MotionEvent::ACTION_DOWN && isEnabled()) {
        const int offset = indexAtX((int)evt.getX());
        const int len = getContentLength();
        const int clamped = offset < 0 ? 0 : (offset > len ? len : offset);

        if (clamped != mCaretPos) {
            setCaretPos(clamped);
            if (mCaretChangeListener)mCaretChangeListener(clamped);
        }
        markInput();
        invalidate(true);
    }
    // 手势整体消费：屏蔽引擎的拖动选中与长按菜单，交互统一走键盘
    return true;
}
