
#ifndef __paint_h__
#define __paint_h__

#include <core/canvas.h>
#include <core/typeface.h>

class Paint
{
public:
    typedef enum
    {
        FILL   = 0x01,
        STROKE = 0x02,
        FILL_AND_STROKE = FILL | STROKE,
    } Style;

    Paint(cdroid::Canvas &ctx);
    Paint();
    ~Paint();

    void setStyle(int flags);
    void setAntiAlias(bool val);
    void setColor(UINT val);
    void setFillColor(UINT val);
    void setStrokeWidth(int val);
    void setStrokeCap(Cairo::Context::LineCap strokeCap);
    void setTextSize(int val);
    void setTypeface(cdroid::Typeface *typeface);
    void getTextSpace(const std::string &txt, double *w, double *h);

protected:
    void initData();

public:
    //************************************
    // Method:    drawText
    // FullName:  Paint::drawText
    // Access:    public
    // Returns:   void
    // Qualifier: 写字
    // Parameter: int x 文字起始位置
    // Parameter: int y 文字底部位置
    // Parameter: int w 宽度
    // Parameter: int h 高度
    // Parameter: const std::string & txt 内容
    //************************************
    void drawText(int x, int y, int w, int h, const std::string &txt, int alignment = 0, cdroid::Canvas *ctx = nullptr);
    //************************************
    // Method:    drawArc
    // FullName:  Paint::drawArc
    // Access:    public
    // Returns:   void
    // Qualifier: 画弧
    // Parameter: int x 圆心x
    // Parameter: int y 圆心y
    // Parameter: int radius 半径
    // Parameter: int sAngle 起始角度 (起点在3点位置)
    // Parameter: int eAngle 终止角度
    //************************************
    void drawArc(int x, int y, int radius, int sAngle, int eAngle, cdroid::Canvas *ctx = nullptr);

private:
    cdroid::Canvas *        mCanvas;        // 画布
    bool                    mAntiAlias;     // 抗锯齿
    UINT                    mColor;         // 颜色
    UINT                    mFillColor;     // 填充颜色
    int                     mStrokeWidth;   // 画笔宽度
    Cairo::Context::LineCap mStrokeCap;     // 端点样式
    int                     mStyle;         // 填充样式
    int                     mTextSize;      // 文字大小
    cdroid::Typeface        *mTypeface;
};

#endif
