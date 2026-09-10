/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-09-10 17:09:31
 * @LastEditTime: 2026-09-10 17:18:24
 * @FilePath: /kk_frame/src/widgets/time_textview.h
 * @Description: 等宽时间控件 - 数字等宽居中、冒号定时闪烁
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __TIME_TEXT_VIEW_H__
#define __TIME_TEXT_VIEW_H__

#include <widget/textview.h>

#include <map>
#include <string>
#include <vector>

/// @brief 等宽时间控件
///
/// 将文本按 ':' 拆分为若干数字段，每个数字字符绘制在统一宽度的单元格内并水平居中，
/// 分隔符绘制在独立的居中单元格内，再配合定时器控制冒号闪烁，
/// 从而避免时间刷新时因数字宽度不同导致的整体抖动。
///
/// 支持的 XML 属性：
///   - autoStart  创建后是否自动开始冒号闪烁（默认 true）
///   - tickerTime 冒号闪烁周期，单位毫秒（默认 1000）
///   - midWidth   分隔符单元格宽度，单位像素（默认 0，表示按 ':' 自然宽度）
class TimeTextView : public TextView {
public:
    TimeTextView(int w, int h);
    TimeTextView(const std::string& txt, int w, int h);
    TimeTextView(Context* ctx, const AttributeSet& attrs);
    ~TimeTextView() override;

public:
    /// @brief 启动冒号闪烁（先隐藏冒号，随后按周期翻转）
    void start();
    /// @brief 停止冒号闪烁，并恢复冒号常显
    void stop();
    /// @brief 是否正在闪烁
    bool isRunning() const;

protected:
    void onAttachedToWindow() override;
    void onDetachedFromWindow() override;
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onDraw(Canvas& canvas) override;

private:
    /// @brief 单个字符的自然显示尺寸
    struct CharMetric {
        int width{ 0 };
        int height{ 0 };
    };

    bool                       mAutoStart{ true };      // 创建后是否自动开始闪烁
    int                        mTickerTime{ 1000 };     // 冒号闪烁周期(ms)
    int                        mMidWidth{ 0 };          // 分隔符单元格宽度(0=按自然宽度)

    bool                       mRunning{ false };       // 是否正在闪烁
    bool                       mColonVisible{ true };   // 冒号当前是否可见
    Runnable                   mTicker;                 // 闪烁定时器

    float                      mMeasuredTextSize{ 0 };  // 测量时使用的字号
    Typeface*                  mMeasuredTypeface{ nullptr }; // 测量时使用的字体
    int                        mDigitWidth{ 0 };        // 数字单元格宽度
    int                        mDigitHeight{ 0 };       // 数字单元格高度
    int                        mSepWidth{ 0 };          // 分隔符单元格宽度
    int                        mSepHeight{ 0 };         // 分隔符单元格高度
    std::map<char, CharMetric> mCharMetrics;            // 各字符自然尺寸缓存

private:
    void init();
    /// @brief 测量并缓存数字、分隔符的单元格尺寸（字号或字体变化时自动重测）
    void measureChars();
    /// @brief 当前生效的字体（为空时回退到默认字体）
    Typeface* currentTypeface() const;
    /// @brief 按 ':' 拆分文本，返回各数字段（空文本返回空容器）
    std::vector<std::string> splitSegments(const std::string& text) const;
    /// @brief 计算内容区总宽高（数字单元格 + 分隔符单元格）
    void computeContentSize(const std::vector<std::string>& segments, int& width, int& height) const;
    /// @brief 分隔符单元格的实际宽度
    int  separatorCellWidth() const;
    /// @brief 取字符的自然尺寸，未缓存的字符返回零尺寸
    CharMetric getCharMetric(char ch) const;
    /// @brief 在单元格内绘制单个字符
    void drawCell(Canvas& canvas, char ch, const CharMetric& metric,
                  int cellLeft, int contentTop, int contentHeight, int cellWidth);
    /// @brief 定时器回调：翻转冒号可见性并重新排期
    void onTicker();
};

#endif // !__TIME_TEXT_VIEW_H__
