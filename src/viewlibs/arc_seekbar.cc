#include "arc_seekbar.h"
#include <core/pools.h>
DECLARE_WIDGET(ArcSeekBar)

double ArcSeekBar::ARC_EXTEND_ANGLE = 10.0;

class VISUAL_ARC_PROGRESS:public Property{
public:
    VISUAL_ARC_PROGRESS():Property("visual_arc_progress"){}
    void set(void*object,const AnimateValue&value)override {
        float fv = GET_VARIANT(value,float);
        ((ArcSeekBar*)object)->setVisualProgress(fv);
        ((ArcSeekBar*)object)->mVisualProgress = fv;
    }
    AnimateValue get(void*object) override{
        return ((ArcSeekBar*)object)->mVisualProgress;
    }
};

ArcSeekBar::ArcSeekBar(int w, int h) : View(w, h) {
    initViewData();
}

ArcSeekBar::ArcSeekBar(Context *ctx, const AttributeSet &attrs) : View(ctx, attrs) {
    initViewData();

    mBackgroundColor = attrs.getColor("colorBackground", 0xff424242);
    mBackgroundWidth = attrs.getDimensionPixelSize("widthBackground", mBackgroundWidth);

    mForegroundColor  = attrs.getColor("colorForeground", mForegroundColor);
    mForegroundColor2 = attrs.getColor("colorForeground2", mForegroundColor);
    mForegroundWidth  = attrs.getDimensionPixelSize("widthForeground", mBackgroundWidth);
    mClickWidth       = attrs.getDimensionPixelSize("clickWidth");
    mClickWidth       = std::max(mClickWidth, std::max(mBackgroundWidth, mForegroundWidth));

    mRadius = attrs.getDimensionPixelSize("radius", 80);

    mStartAngle = attrs.getInt("startAngle", 150);
    mEndAngle   = attrs.getInt("endAngle", 390);

    mMin      = attrs.getInt("min", mMin);
    mMax      = attrs.getInt("max", mMax);
    setProgress(attrs.getInt("progress", mValue));

    mSliderRadius    = attrs.getDimensionPixelSize("sliderRadius", 14);
    mSliderWidth     = attrs.getDimensionPixelSize("sliderWidth", 1);
    mSliderColor     = attrs.getColor("sliderColor", mSliderColor);
    mSliderFillColor = attrs.getColor("sliderFillColor", mSliderFillColor);

    mSlider             = attrs.getDrawable("slider");
    mShowSlider         = attrs.getBoolean("showSlider", true);
    mSliderRotate       = attrs.getBoolean("sliderRotate", false);
    mSliderRotateRadius = attrs.getDimensionPixelSize("sliderRotateRadius");
    mShowProgressTxt    = attrs.getBoolean("showProgressText", mShowProgressTxt);
    mIsPercentProgress  = attrs.getBoolean("isPercentProcess", mIsPercentProgress);

    mStrokeCap = (Cairo::Context::LineCap)attrs.getInt(
        "strokeCap",
        std::unordered_map<std::string, int>{{"butt", (int)Cairo::Context::LineCap::BUTT},
                                         {"round", (int)Cairo::Context::LineCap::ROUND},
                                         {"square", (int)Cairo::Context::LineCap::SQUARE}},
        (int)mStrokeCap);

    mStrokeCapMiddle = (Cairo::Context::LineCap)attrs.getInt(
        "strokeCapMiddle",
        std::unordered_map<std::string, int>{{"butt", (int)Cairo::Context::LineCap::BUTT},
                                         {"round", (int)Cairo::Context::LineCap::ROUND},
                                         {"square", (int)Cairo::Context::LineCap::SQUARE}},
        (int)mStrokeCap);

    mBorderColor = attrs.getColor("borderColor", mBorderColor);
    mBorderWidth = attrs.getDimensionPixelSize("borderWidth", mBorderWidth);
    mIsReverse   = attrs.getBoolean("reverse", mIsReverse);    

    LOG(VERBOSE) << "radius=" << mRadius << " startAngle=" << mStartAngle << " endAngle=" << mEndAngle;
}

ArcSeekBar::~ArcSeekBar() {
    delete mSlider;
    delete mLastProgressAnimator;
}

void ArcSeekBar::initViewData() {
    mBackgroundColor       = 0xff424242;
    mBackgroundWidth       = 20;
    mForegroundColor       = 0xffB7B7B7;
    mForegroundWidth       = 20;
    mClickWidth            = 20;
    mRadius                = 80;
    mStartAngle            = 150;
    mEndAngle              = 390;
    mMin                   = 0;
    mMax                   = 100;
    mProgress              = 0.f;
    mValue                 = 30;
    mSliderRadius          = 14;
    mSliderWidth           = 1;
    mSliderColor           = 0xffd9d9d9;
    mSliderFillColor       = 0xffd9d9d9;
    mSliderRotate          = false;
    mSliderRotateRadius    = 0;
    mShowSlider            = true;
    mSliderDown            = false;
    mOnChangeListener      = nullptr;
    mOnChangeListener2     = nullptr;
    mOnChangeCheckListener = nullptr;
    mLastProgressAnimator  = nullptr;
    mForegroundColor2      = mForegroundColor;
    mTouchLastX            = 0;
    mTouchLastY            = 0;
    mFromUser              = false;
    mBorderWidth           = 0;
    mBorderColor           = 0xFFFF0000;
    mIsReverse             = false;
    mVisualProgress        = 0;

    mShowProgressTxt   = false;
    mIsPercentProgress = false;
    mPaintTxt.setColor(Color::WHITE);
    mPaintTxt.setTextSize((mRadius - MAX(mBackgroundWidth, mForegroundWidth) / 2.0) * 0.65);

    mStrokeCap       = Cairo::Context::LineCap::ROUND;
    mStrokeCapMiddle = mStrokeCap;
    bzero(mRange, sizeof(mRange));

    if(Property::fromName("visual_arc_progress")==nullptr){
        Property::reigsterProperty("visual_arc_progress",new VISUAL_ARC_PROGRESS());
    }
}

void ArcSeekBar::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    View::onMeasure(widthMeasureSpec, heightMeasureSpec);

    mWidth  = getMeasuredWidth();
    mHeight = getMeasuredHeight();

    mCircleX = mWidth / 2;
    mCircleY = mHeight / 2;

    LOG(VERBOSE) << "w=" << mWidth << " h=" << mHeight << " cx=" << mCircleX << " cy=" << mCircleY;
}

void ArcSeekBar::onDraw(Canvas &ctx) {
    View::onDraw(ctx);

    float progress = mIsReverse ? (1 - mVisualProgress) : mVisualProgress;

    checkRange(progress);

    // 背景
    if (mBackgroundWidth > 0 && (mBackgroundColor & 0xFF000000)) {
        Paint fPaint(ctx);
        fPaint.setStrokeWidth(mBackgroundWidth);
        fPaint.setColor(mBackgroundColor);
        fPaint.setStrokeCap(mStrokeCap);
        fPaint.drawArc(mCircleX, mCircleY, mRadius, mStartAngle, mEndAngle);
    }

    // 边框
    onDrawStroke(ctx);

    int startAngle, endAngle;
    if (mIsReverse) {
        startAngle = mStartAngle + (mEndAngle - mStartAngle) * progress;
        endAngle   = mEndAngle;
    } else {
        startAngle = mStartAngle;
        endAngle   = mStartAngle + (mEndAngle - mStartAngle) * progress;
    }
    LOG(VERBOSE) << getId() << " progress=" << progress << " end=" << endAngle;

    // 前景
    if (endAngle > startAngle) {
        if (mForegroundColor == mForegroundColor2) {
            Paint sPaint(ctx);
            sPaint.setStrokeWidth(mForegroundWidth);
            sPaint.setColor(mForegroundColor);
            sPaint.setStrokeCap(mStrokeCap);
            if (mStrokeCap == mStrokeCapMiddle || endAngle == mEndAngle) {
                sPaint.drawArc(mCircleX, mCircleY, mRadius, startAngle, endAngle);
            } else {
                if (mIsReverse) {
                    int endAngle2 = endAngle - 1;
                    sPaint.drawArc(mCircleX, mCircleY, mRadius, endAngle2, endAngle);
                    if (endAngle2 > startAngle) {
                        sPaint.setStrokeCap(mStrokeCapMiddle);                    
                        if (mStrokeCapMiddle == Cairo::Context::LineCap::BUTT && (startAngle % 360 >= 210 || startAngle % 360 <= 60)) {
                            // 项目浮动值
                            sPaint.drawArc(mCircleX, mCircleY, mRadius, std::max(mStartAngle, startAngle - 1), endAngle2);
                        } else {
                            sPaint.drawArc(mCircleX, mCircleY, mRadius, startAngle, endAngle2);
                        }
                    }
                } else {
                    int endAngle2 = std::min(startAngle + 1, endAngle);
                    sPaint.drawArc(mCircleX, mCircleY, mRadius, startAngle, endAngle2);
                    if (endAngle2 < endAngle) {
                        sPaint.setStrokeCap(mStrokeCapMiddle);                    
                        if (mStrokeCapMiddle == Cairo::Context::LineCap::BUTT && endAngle % 360 < 210) {
                            // 项目浮动值
                            sPaint.drawArc(mCircleX, mCircleY, mRadius, endAngle2, endAngle + 1.0);
                        } else {
                            sPaint.drawArc(mCircleX, mCircleY, mRadius, endAngle2, endAngle);
                        }
                    }
                }
            }
        } else {
            cairo_t *cr = ctx.cobj();
            // 创建线性渐变
            cairo_pattern_t *gradient =
                cairo_pattern_create_linear(mCircleX - mRadius, mCircleY, mCircleX + mRadius, mCircleY);
            Color gradient_c(mForegroundColor);
            Color gradient_c2(mForegroundColor2);

            cairo_pattern_add_color_stop_rgb(gradient, 0, gradient_c.red(), gradient_c.green(),
                                             gradient_c.blue()); // 起始颜色

            cairo_pattern_add_color_stop_rgb(gradient, 1, gradient_c2.red(), gradient_c2.green(),
                                             gradient_c2.blue()); // 终止颜色

            cairo_new_path(cr);
            cairo_arc(cr, mCircleX, mCircleY, mRadius, startAngle * M_PI / 180.0, endAngle * M_PI / 180.0);
            cairo_set_line_width(cr, mForegroundWidth);
            cairo_set_line_cap(cr, static_cast<cairo_line_cap_t>(mStrokeCap));
            cairo_set_source(cr, gradient);
            cairo_stroke(cr);

            cairo_pattern_destroy(gradient);
        }
    }

    // 滑块
    if (mShowSlider) {
        int sliderAngle = mIsReverse ? startAngle : endAngle;
        int x = mCircleX + mRadius * cos(sliderAngle * M_PI / 180.0);
        int y = mCircleY + mRadius * sin(sliderAngle * M_PI / 180.0);

        if (mSlider != nullptr) {
            BitmapDrawable *bitmap = dynamic_cast<BitmapDrawable *>(mSlider);

            mRectSlider.width  = mSlider->getIntrinsicWidth();
            mRectSlider.height = mSlider->getIntrinsicHeight();
            mRectSlider.left   = x - mRectSlider.width / 2;
            mRectSlider.top    = y - mRectSlider.height / 2;

            if (mSliderRotate && bitmap) {
                Cairo::RefPtr<Cairo::ImageSurface> originalSurface = bitmap->getBitmap();
                int                                dw              = originalSurface->get_width();
                int                                dh              = originalSurface->get_height();

                /* 创建新的 Cairo::ImageSurface 对象作为旋转后的结果图像目标 */
                Cairo::RefPtr<Cairo::ImageSurface> rotatedSurface =
                    Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, dw, dh);

                Canvas cans(rotatedSurface);
                cans.translate(dw / 2.0, dh / 2.0);
                cans.rotate(sliderAngle * M_PI / 180.0);
                cans.draw_image(originalSurface, Rect::Make(-dw / 2, -dh / 2, dw, dh), nullptr);

                if (mSliderRotateRadius > 0) {
                    mRectSlider.left += mSliderRotateRadius * cos(sliderAngle * M_PI / 180.0);
                    mRectSlider.top  += mSliderRotateRadius * sin(sliderAngle * M_PI / 180.0);
                }

                ctx.begin_new_path();
                ctx.draw_image(rotatedSurface, mRectSlider, nullptr);

            } else {
                ctx.begin_new_path();
                mSlider->setBounds(mRectSlider);
                mSlider->draw(ctx);
            }
        } else {
            Paint silder(ctx);
            silder.setStrokeWidth(mSliderWidth);
            silder.setColor(mSliderColor);
            silder.setFillColor(mSliderFillColor);
            silder.setStyle(Paint::FILL_AND_STROKE);
            silder.drawArc(x, y, mSliderRadius, 0, 360);

            mRectSlider.width  = (mSliderWidth + mSliderRadius) * 2;
            mRectSlider.height = mRectSlider.width;
            mRectSlider.left   = x - mRectSlider.width / 2;
            mRectSlider.top    = y - mRectSlider.height / 2;
        }
    }

    // 进度值
    if (mShowProgressTxt) {
        std::string showTxt = std::to_string((int)progress);
        if (mIsPercentProgress) showTxt += "%";
        mPaintTxt.drawText(mCircleX - mRadius, mCircleY - mRadius, mRadius * 2, mRadius * 2, showTxt, Gravity::CENTER,
                           &ctx);
    }
}

bool ArcSeekBar::onTouchEvent(MotionEvent &evt) {
    if (!mShowSlider || !isEnabled()) { return false; }

    LOG(VERBOSE) << "action=" << evt.getAction() << " x=" << evt.getX() << " y=" << evt.getY();

    switch (evt.getAction()) {
    case MotionEvent::ACTION_DOWN:{
        // 判断是否点击在滑块上
        // mSliderDown = mRectSlider.contains(evt.getX(), evt.getY());
        // if (!mSliderDown) {
            // 未点击滑块，必须点击在弧上（目前是直接判断是否在弧上）
            int angle = getRelativeDegrees(evt.getX(), evt.getY());
            if (angle >= mStartAngle - ARC_EXTEND_ANGLE && angle <= mEndAngle + ARC_EXTEND_ANGLE) {
                // 满足弧线内
                RECT rc;
                int  x    = mCircleX + mRadius * cos(angle * M_PI / 180.0);
                int  y    = mCircleY + mRadius * sin(angle * M_PI / 180.0);
                rc.width  = mClickWidth;
                rc.height = mClickWidth;
                rc.left   = x - rc.width / 2;
                rc.top    = y - rc.height / 2;
                if (rc.contains(evt.getX(), evt.getY())) {
                    float progress = (float)(angle - mStartAngle) / (mEndAngle - mStartAngle);
                    if (mIsReverse) progress = 1 - progress;
                    mFromUser = true;
                    setProgress(progress);
                    mFromUser = false;
                    mSliderDown = true;
                }
            }
        // }
    }
        
        break;
    case MotionEvent::ACTION_UP: 
        mSliderDown = false; 
        setProgress((int)(mProgress * (mMax - mMin) + mMin),true);
        break;
    case MotionEvent::ACTION_MOVE:
        if (mSliderDown) {
            calcTouchState(evt.getX(), evt.getY());
            int angle = getRelativeDegrees(evt.getX(), evt.getY());
            if (angle >= mStartAngle - ARC_EXTEND_ANGLE) {
                float lastProgress = mProgress;
                float progress     = (float)(angle - mStartAngle) / (mEndAngle - mStartAngle);            
                if (mIsReverse) {                    
                    progress     = 1 - progress;
                    mAngleAdd    = !mAngleAdd;
                }
                /* 防止循环滑 */
                if (mAngleAdd) {
                    if (progress < lastProgress && lastProgress  > 7.f / 8.f) {
                        progress = 1.f; // 已经滑到最大[边缘抖动]
                    } else if (lastProgress == 0.f) {
                        if (progress > 1.f / 8) {
                            break; // 需要逐渐增加
                        }
                    }
                } else {
                    if (progress > lastProgress && lastProgress< 1.f/ 8.f) {
                        progress = 0.f; // 已经滑到最小
                    } else if (lastProgress == 1.f) {
                        if (progress < 7.f / 8.f) {
                            break; // 需要逐渐减少
                        }
                    }
                }
                LOG(VERBOSE) << "angle=" << angle << " progress=" << progress;
                mFromUser = true;
                setProgress(progress);
                mFromUser = false;
            }
        }
        break;
    }

    mTouchLastX = evt.getX();
    mTouchLastY = evt.getY();

    return mSliderDown || View::onTouchEvent(evt);
}

/**
 * 获取触摸坐标的夹角度数
 * @param x
 * @param y
 * @return
 */
float ArcSeekBar::getTouchDegrees(float x, float y) {
    double x1 = x - mCircleX;
    double y1 = y - mCircleY;

    double arcNum   = atan2(y1, x1);
    double angleNum = arcNum * 180 / M_PI;

    LOG(VERBOSE) << "y=" << y1 << " x=" << x1 << " arc=" << arcNum << " angle=" << angleNum;

    return angleNum;
}

float ArcSeekBar::getRelativeDegrees(float x, float y) {
    double angleNum = getTouchDegrees(x, y);

    if (angleNum < 0 || angleNum < mStartAngle) { angleNum += 360; }
    if (angleNum < mStartAngle) { angleNum += 360; }

    LOG(VERBOSE) << "relative angle=" << angleNum;

    return angleNum;
}

void ArcSeekBar::setMin(int min) {
    if (mMin != min) {
        mMin = min;

        if (mValue < mMin) {
            mProgress = 0.f;
            doRefreshProgress(mProgress,false,false);
            mValue = mMin;
            onChangeProgress();
        }
        invalidate();
    }
}

int ArcSeekBar::getMin() {
    return mMin;
}

void ArcSeekBar::setMax(int max) {
    if (mMax != max) {
        mMax = max;
        if (mValue > mMax) {
            mProgress = 1.f;
            doRefreshProgress(mProgress,false,false);
            mValue = mMax;
            onChangeProgress();
        }
        invalidate();
    }
}

int ArcSeekBar::getMax() {
    return mMax;
}

void ArcSeekBar::setProgressInternal(float progress, bool fromUser,bool animate){
    if (mOnChangeCheckListener && !mOnChangeCheckListener(*this, progress)) return;

    if (progress > 1.f){
        progress = 1.f;
    } else if (progress < 0.f){
        progress = 0.f;
    }

    checkRange(progress);
    int newValue = progress * (mMax - mMin) + mMin;

    if (progress == mProgress) return;

    mProgress = progress;
    if(mValue != newValue){
        mValue    = newValue;
        onChangeProgress();
    }

    doRefreshProgress(mProgress,fromUser,animate);
}

void ArcSeekBar::doRefreshProgress(float progress, bool fromUser, bool animate){
    if (animate) {
        Animator::AnimatorListener animListener;
        ObjectAnimator* animator = ObjectAnimator::ofFloat(this,"visual_arc_progress",{progress});
        animator->setAutoCancel(true);
        animator->setDuration(PROGRESS_ANIM_DURATION);
        animator->setInterpolator(DecelerateInterpolator::gDecelerateInterpolator.get());
        AnimatorListenerAdapter animtorListener;
        animtorListener.onAnimationEnd=[this](Animator&anim,bool){
            delete mLastProgressAnimator;
            mLastProgressAnimator = nullptr;
        };
        animator->addListener(animtorListener);
        animator->start();
        mLastProgressAnimator = animator;
    } else {
        if(mLastProgressAnimator){
            mLastProgressAnimator->cancel();
            delete mLastProgressAnimator;
            mLastProgressAnimator = nullptr;
        }
        setVisualProgress(progress);
    }
}

void ArcSeekBar::setProgress(float progress){
    setProgressInternal(progress,false,false);
}

void ArcSeekBar::setProgress(int progress) {
    if(progress>=mMax) setProgress(1.f);
    else if(progress <= mMin) setProgress(0.f);
    else    setProgressInternal((float)(progress - mMin)/(mMax - mMin),false,false);
}

void ArcSeekBar::setProgress(int progress, bool animate){
    if(progress>=mMax) setProgress(1.f);
    else if(progress <= mMin) setProgress(0.f);
    else    setProgressInternal((float)(progress - mMin)/(mMax - mMin),false,animate);
}

int ArcSeekBar::getProgress() {    
    return mValue;
}

void ArcSeekBar::setVisualProgress(float progress){
    mVisualProgress = progress;
    invalidate(true);
}

void ArcSeekBar::setOnChangeListener(OnArcSeekBarChangeListener l) {
    mOnChangeListener = l;
}

void ArcSeekBar::setOnChangeListener2(OnArcSeekBarChangeListener2 l) {
    mOnChangeListener2 = l;
}

void ArcSeekBar::setOnChangeCheckListener(OnArcSeekBarChangeCheckListener l) {
    mOnChangeCheckListener = l;
}
void ArcSeekBar::setShowSlider(bool Show) {mShowSlider = Show; }
void ArcSeekBar::setForegroundColor(int color){ 
    if(mForegroundColor != color){
        mForegroundColor = color; 
        mForegroundColor2 = color;
        invalidate();
    }
    
}
void ArcSeekBar::setForegroundColor2(int color){ 
    if(mForegroundColor2 != color){
        mForegroundColor2 = color;
        invalidate();
    }
    
}
int ArcSeekBar::getWidthProgress() {
    return std::max(1, mMax - mMin);
}

void ArcSeekBar::calcTouchState(int x, int y) {
    float lastAngle = getTouchDegrees(mTouchLastX, mTouchLastY);
    float currAngle = getTouchDegrees(x, y);

    // 顺时针，加
    if (x < mCircleX || y < mCircleY) {
        if (lastAngle > 0) {
            if (currAngle > lastAngle || currAngle < 0) {
                mAngleAdd = 1;
                LOGV("angle add %.2f %.2f", lastAngle, currAngle);
                return;
            }
        } else {
            if (currAngle < 0 && currAngle > lastAngle) {
                mAngleAdd = 1;
                LOGV("angle add %.2f %.2f", lastAngle, currAngle);
                return;
            }
        }
    } else if (currAngle > lastAngle) {
        mAngleAdd = 1;
        LOGV("angle add %.2f %.2f", lastAngle, currAngle);
        return;
    }

    // 逆时针，减
    mAngleAdd = 0;
    LOGV("angle dec %.2f %.2f", lastAngle, currAngle);
}

void ArcSeekBar::setRange(int r0, int r2) {
    if (r2 < r0) r2 = mMax;
    if (r0 >= mMin && r2 <= mMax && r0 < r2) {
        LOGV("Range[%d,%d] => [%d,%d]", mRange[0], mRange[1], r0, r2);
        mRange[0] = r0;
        mRange[1] = r2;
    } else {
        bzero(mRange, sizeof(mRange));
    }
}

void ArcSeekBar::checkRange(float &progress) {
    int value = progress*(mMax - mMin) + mMin;
    if (mRange[0] >= mMin && mRange[1] <= mMax && mRange[0] < mRange[1]) {
        if (value > mRange[1]) {
            LOGV("[%d,%d]change to up %f=>%d", mRange[0], mRange[1], progress, mRange[1]);
            progress = (mRange[1] - mMin)/(mMax - mMin);
        } else if (value < mRange[0]) {
            LOGV("[%d,%d]change to down %f=>%d", mRange[0], mRange[1], progress, mRange[0]);
            progress = (mRange[0] - mMin)/(mMax - mMin);
        }
    }
}

void ArcSeekBar::onChangeProgress() {    
    if (mOnChangeListener) { mOnChangeListener(*this, mValue); }
    if (mOnChangeListener2) { mOnChangeListener2(*this, mValue, mFromUser); }
}

void ArcSeekBar::onDrawStroke(Canvas &ctx) {
    if (mBorderWidth <= 0 || (mBorderColor & 0xFF000000) == 0 || mStrokeCap != Cairo::Context::LineCap::BUTT) {
        return;
    }

    cairo_t *cr = ctx.cobj();
    int      cx, cy, r1, r2;
    double   x1, y1, x2, y2;
    int      borderWidth = std::max(mBackgroundWidth, mForegroundWidth);
    double   startAngle  = M_PI * mStartAngle / 180.0;
    double   endAngle    = M_PI * mEndAngle / 180.0;
    cx                   = mCircleX;
    cy                   = mCircleY;
    r1                   = mRadius - borderWidth / 2;
    r2                   = mRadius + borderWidth / 2;

    x1 = cx + r1 * cos(startAngle);
    y1 = cy + r1 * sin(startAngle);
    x2 = cx + r2 * cos(startAngle);
    y2 = cy + r2 * sin(startAngle);

    cairo_new_path(cr);

    cairo_move_to(cr, x2, y2);
    cairo_line_to(cr, x1, y1);
    cairo_arc(cr, cx, cy, r1, startAngle, endAngle);

    cairo_move_to(cr, x2, y2);
    cairo_arc(cr, cx, cy, r2, startAngle, endAngle);

    x1 = cx + r1 * cos(endAngle);
    y1 = cy + r1 * sin(endAngle);
    cairo_line_to(cr, x1, y1);

    // 设置边框线宽和颜色
    cairo_set_line_width(cr, mBorderWidth);

    Color bc(mBorderColor);
    cairo_set_source_rgba(cr, bc.red(), bc.green(), bc.blue(), bc.alpha());

    // 绘制边框
    cairo_stroke(cr);
}

