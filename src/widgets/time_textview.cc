/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-10 17:09:31
 * @LastEditTime: 2026-09-10 17:18:35
 * @FilePath: /kk_frame/src/widgets/time_textview.cc
 * @Description: 等宽时间控件 - 数字等宽居中、冒号定时闪烁
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "time_textview.h"

#include <algorithm>
#include <cmath>

DECLARE_WIDGET(TimeTextView)

TimeTextView::TimeTextView(int w, int h)
    : TimeTextView(std::string(), w, h) {
}

TimeTextView::TimeTextView(const std::string& txt, int w, int h)
    : TextView(txt, w, h) {
    init();
    if (mAutoStart) start();
}

TimeTextView::TimeTextView(Context* ctx, const AttributeSet& attrs)
    : TextView(ctx, attrs) {
    init();

    mAutoStart  = attrs.getBoolean("autoStart", mAutoStart);
    mTickerTime = attrs.getInt("tickerTime", mTickerTime);
    mMidWidth   = attrs.getDimensionPixelSize("midWidth", mMidWidth);

    // 闪烁周期必须有意义，非法值回退为默认周期
    if (mTickerTime <= 0) mTickerTime = 1000;

    if (mAutoStart) start();
}

TimeTextView::~TimeTextView() {
    stop();
}

/// @brief 初始化
void TimeTextView::init() {
    mTicker = std::bind(&TimeTextView::onTicker, this);
}

/// @brief 启动冒号闪烁
void TimeTextView::start() {
    if (mRunning) return;
    mRunning = true;
    mColonVisible = false;
    invalidate();
    postDelayed(mTicker, mTickerTime);
}

/// @brief 停止冒号闪烁
void TimeTextView::stop() {
    if (!mRunning) return;
    mRunning = false;
    mColonVisible = true;
    removeCallbacks(mTicker);
    invalidate();
}

/// @brief 是否正在闪烁
bool TimeTextView::isRunning() const {
    return mRunning;
}

/// @brief 定时器回调
void TimeTextView::onTicker() {
    if (!mRunning) return;
    mColonVisible = !mColonVisible;
    invalidate();
    postDelayed(mTicker, mTickerTime);
}

/// @brief 挂载到窗口
void TimeTextView::onAttachedToWindow() {
    TextView::onAttachedToWindow();
    // 自动播放模式下，重新挂载后恢复闪烁（已运行时 start 会直接返回）
    if (mAutoStart) start();
}

/// @brief 从窗口卸载
void TimeTextView::onDetachedFromWindow() {
    // 卸载后不再需要刷新，停止定时器避免无谓回调
    stop();
    TextView::onDetachedFromWindow();
}

/// @brief 测量
void TimeTextView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    TextView::onMeasure(widthMeasureSpec, heightMeasureSpec);

    const std::vector<std::string> segments = splitSegments(getText());
    if (segments.empty()) return;

    measureChars();

    int contentWidth = 0;
    int contentHeight = 0;
    computeContentSize(segments, contentWidth, contentHeight);

    LayoutParams* lp = getLayoutParams();
    if (lp == nullptr) return;

    int width = getMeasuredWidth();
    int height = getMeasuredHeight();
    bool changed = false;

    // 仅当控件声明为 WRAP_CONTENT 且现有测量结果不足时，才扩展到内容所需尺寸
    if (lp->width == LayoutParams::WRAP_CONTENT) {
        const int desired = contentWidth + getPaddingLeft() + getPaddingRight();
        if (width < desired) {
            width = desired;
            changed = true;
        }
    }
    if (lp->height == LayoutParams::WRAP_CONTENT) {
        const int desired = contentHeight + getPaddingTop() + getPaddingBottom();
        if (height < desired) {
            height = desired;
            changed = true;
        }
    }

    if (changed) setMeasuredDimension(width, height);
}

/// @brief 绘制
void TimeTextView::onDraw(Canvas& canvas) {
    const std::string text = getText();
    if (text.empty()) return;

    const std::vector<std::string> segments = splitSegments(text);
    if (segments.empty()) return;

    measureChars();

    int contentWidth = 0;
    int contentHeight = 0;
    computeContentSize(segments, contentWidth, contentHeight);
    if (contentWidth <= 0 || contentHeight <= 0) return;

    const int availWidth = getWidth() - getPaddingLeft() - getPaddingRight();
    const int availHeight = getHeight() - getPaddingTop() - getPaddingBottom();
    const int gravity = getGravity();

    int left = getPaddingLeft();
    int top = getPaddingTop();
    if (availWidth > contentWidth && (gravity & Gravity::CENTER_HORIZONTAL)) {
        left += (availWidth - contentWidth) / 2;
    }
    if (availHeight > contentHeight && (gravity & Gravity::CENTER_VERTICAL)) {
        top += (availHeight - contentHeight) / 2;
    }

    canvas.save();
    canvas.set_font_face(currentTypeface()->getFontFace()->get_font_face());
    canvas.set_font_size(getTextSize());
    canvas.set_color(static_cast<uint32_t>(getCurrentTextColor()));

    const int sepCellWidth = separatorCellWidth();
    int cellLeft = left;
    for (size_t i = 0; i < segments.size(); i++) {
        // 段与段之间绘制分隔符，无论冒号是否可见都占用同样的宽度，避免位置跳动
        if (i > 0) {
            if (mColonVisible) {
                drawCell(canvas, ':', getCharMetric(':'), cellLeft, top, contentHeight, sepCellWidth);
            }
            cellLeft += sepCellWidth;
        }
        for (size_t n = 0; n < segments[i].size(); n++) {
            const char ch = segments[i][n];
            drawCell(canvas, ch, getCharMetric(ch), cellLeft, top, contentHeight, mDigitWidth);
            cellLeft += mDigitWidth;
        }
    }

    canvas.restore();
}

/// @brief 测量字符尺寸
void TimeTextView::measureChars() {
    Typeface* typeface = currentTypeface();
    const float textSize = getTextSize();

    // 字体或字号未变化时复用已有测量结果
    if (mMeasuredTypeface == typeface &&
        std::fabs(mMeasuredTextSize - textSize) < 0.01f &&
        !mCharMetrics.empty()) {
        return;
    }

    // 借助 1x1 的临时画布获取文本度量，避免依赖真实的绘制上下文
    Cairo::RefPtr<Cairo::Surface> surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 1, 1);
    Cairo::RefPtr<Cairo::Context> ctx = Cairo::Context::create(surface);
    ctx->set_font_face(typeface->getFontFace()->get_font_face());
    ctx->set_font_size(textSize);

    static const std::string kChars = "-0123456789: ";
    mDigitWidth = mDigitHeight = mSepWidth = mSepHeight = 0;
    mCharMetrics.clear();

    for (size_t i = 0; i < kChars.size(); i++) {
        const char ch = kChars[i];
        Cairo::TextExtents extents;
        ctx->get_text_extents(std::string(1, ch), extents);

        CharMetric metric;
        metric.width = static_cast<int>(std::ceil(extents.width + extents.x_bearing));
        metric.height = static_cast<int>(std::ceil(extents.height));
        mCharMetrics[ch] = metric;

        // 数字（含占位符 '-'）使用统一单元格，其余字符（':'、' '）使用分隔符单元格
        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            mDigitWidth = std::max(mDigitWidth, metric.width);
            mDigitHeight = std::max(mDigitHeight, metric.height);
        } else {
            mSepWidth = std::max(mSepWidth, metric.width);
            mSepHeight = std::max(mSepHeight, metric.height);
        }
    }

    mMeasuredTypeface = typeface;
    mMeasuredTextSize = textSize;
}

/// @brief 当前生效字体
Typeface* TimeTextView::currentTypeface() const {
    Typeface* typeface = mLayout ? mLayout->getTypeface() : nullptr;
    return typeface ? typeface : Typeface::DEFAULT;
}

/// @brief 按 ':' 拆分文本
std::vector<std::string> TimeTextView::splitSegments(const std::string& text) const {
    std::vector<std::string> segments;
    if (text.empty()) return segments;

    std::string current;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == ':') {
            segments.push_back(current);
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    segments.push_back(current);
    return segments;
}

/// @brief 计算内容区总宽高
void TimeTextView::computeContentSize(const std::vector<std::string>& segments, int& width, int& height) const {
    width = 0;
    height = 0;
    if (segments.empty()) return;

    int digitCells = 0;
    for (size_t i = 0; i < segments.size(); i++) {
        digitCells += static_cast<int>(segments[i].size());
    }
    const int sepCells = static_cast<int>(segments.size()) - 1;

    width = digitCells * mDigitWidth + sepCells * separatorCellWidth();
    height = std::max(mDigitHeight, mSepHeight);
}

/// @brief 分隔符单元格宽度
int TimeTextView::separatorCellWidth() const {
    return std::max(mSepWidth, mMidWidth);
}

/// @brief 取字符自然尺寸
TimeTextView::CharMetric TimeTextView::getCharMetric(char ch) const {
    const auto it = mCharMetrics.find(ch);
    if (it == mCharMetrics.end()) return CharMetric();
    return it->second;
}

/// @brief 绘制字符单元格
void TimeTextView::drawCell(Canvas& canvas, char ch, const CharMetric& metric,
                            int cellLeft, int contentTop, int contentHeight, int cellWidth) {
    // 水平方向：字符在单元格内居中
    const double x = cellLeft + (cellWidth - metric.width) / 2.0;
    // 垂直方向：'-' 位于行中部，其余字符按字形高度居中
    const double y = (ch == '-')
        ? contentTop + contentHeight - metric.height / 2.0
        : contentTop + contentHeight - (contentHeight - metric.height) / 2.0;

    canvas.move_to(x, y);
    canvas.show_text(std::string(1, ch));
}
