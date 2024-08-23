/************************************************************
 * Description: 弧形进度条，弧形缺口>20度
 * Author     : hookjc
 * C-Date     : 2023/4/29
 ************************************************************/

#ifndef __arc_seekbar_h__
#define __arc_seekbar_h__

#include <view/view.h>
#include "paint.h"

class ArcSeekBar : public View {
public:
    DECLARE_UIEVENT(bool, OnArcSeekBarChangeCheckListener, View &v, int progress);
    DECLARE_UIEVENT(void, OnArcSeekBarChangeListener, View &v, int progress);
    DECLARE_UIEVENT(void, OnArcSeekBarChangeListener2, View &v, int progress, bool fromUser);

public:
    ArcSeekBar(int w, int h);
    ArcSeekBar(Context *ctx, const AttributeSet &attr);
    ~ArcSeekBar();

    void setMin(int min);
    int  getMin();
    void setMax(int max);
    int  getMax();
    void setRange(int r0, int r2);
    void setProgress(int progress);
    int  getProgress();
    void setOnChangeListener(OnArcSeekBarChangeListener l);
    void setOnChangeListener2(OnArcSeekBarChangeListener2 l);
    void setOnChangeCheckListener(OnArcSeekBarChangeCheckListener l);


    void setBackgroundColor(int color);
    void setForegroundColor(int color);
    void setForegroundColor2(int color);

protected:
    virtual void onMeasure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void onDraw(Canvas &ctx);
    virtual bool onTouchEvent(MotionEvent &evt);
    virtual void onChangeProgress();
    virtual void onDrawStroke(Canvas &ctx);

protected:
    static double ARC_EXTEND_ANGLE;
    enum emArcDrawStatus {
        ADS_DRAWING = 1, // 绘制中
        ADS_COMPLETE,    // 绘制完成
    };
    void  initViewData();
    float getRelativeDegrees(float x, float y);
    float getTouchDegrees(float x, float y);
    void  onDrawTimer();
    int   getWidthProgress();
    void  calcTouchState(int x, int y);
    void  checkRange(int &progress);

protected:
    int                             mWidth;                 // 可绘制圆的宽度
    int                             mHeight;                // 可绘制圆的宽度
    int                             mCircleX;               // 圆弧中心x
    int                             mCircleY;               // 圆弧中心Y
    int                             mStartAngle;            // m/180.0*PI 起始角度
    int                             mEndAngle;              // M/180.0*PI 结束角度
    int                             mBackgroundColor;       // 背景色
    int                             mBackgroundWidth;       // 背景弧线宽度
    int                             mForegroundColor;       // 前景色
    int                             mForegroundColor2;      // 前景色2
    Cairo::Context::LineCap         mStrokeCap;             // 线条端点样式
    Cairo::Context::LineCap         mStrokeCapMiddle;       // 线条中间端点样式
    int                             mForegroundWidth;       // 前景弧线宽度
    int                             mClickWidth;            // 弧线点击宽度
    int                             mRadius;                // 圆弧半径
    int                             mMin;                   // 最小值
    int                             mMax;                   // 最大值
    int                             mProgress;              // 当前进度
    int                             mSliderWidth;           // 滑块线条宽度
    int                             mSliderColor;           // 滑块线条颜色
    int                             mSliderFillColor;       // 滑块圆形填充颜色
    Drawable                       *mSlider;                // 滑块
    bool                            mSliderRotate;          // 滑块旋转
    int                             mSliderRotateRadius;    // 滑块旋转半径
    bool                            mShowSlider;            // 是否显示滑块
    int                             mSliderRadius;          // 滑块半径
    RECT                            mRectSlider;            // 滑块区域
    bool                            mSliderDown;            // 是否点击滑块
    int                             mDrawStatus;            // 滑块绘制状态
    bool                            mShowProgressTxt;       // 显示进度数值
    bool                            mIsPercentProgress;     // 百分比进度
    Paint                           mPaintTxt;              // 文本属性
    OnArcSeekBarChangeCheckListener mOnChangeCheckListener; // 变化检测
    OnArcSeekBarChangeListener      mOnChangeListener;      // 滑块变化事件
    OnArcSeekBarChangeListener2     mOnChangeListener2;     // 滑块变化事件
    int                             mTouchLastX;            // 上一次触摸X点
    int                             mTouchLastY;            // 上一次触摸Y点
    int                             mAngleAdd;              // 滑动增加
    int                             mRange[2];              // 可滑动范围
    bool                            mFromUser;              // 用户操作引发进度变化
    int                             mBorderColor;           // 线条颜色
    int                             mBorderWidth;           // 线条宽
    bool                            mIsReverse;             // 反向进度[逆时针]
};

#endif
