#ifndef __GAUSS_FILTER_DRAWABLE_H__
#define __GAUSS_FILTER_DRAWABLE_H__
// #include <drawables/drawable.h>
#include <view/view.h>

class GaussFilterDrawable:public Drawable{

private:
    unsigned char *mBitmapData;
    unsigned char *mGaussData;
    unsigned char *bitmapRGBData;
    unsigned char *bitmapRGBGaussData;
    View        *mFromView;
    int         mGaussRadius;
    int         mScale;     // 先将图层进行缩放，再进行模糊计算

    Rect        mGaussRegion;
    int         mGaussWidth;
    int         mGaussHeight;
    Cairo::RefPtr<Cairo::ImageSurface>mBitmap;
    Cairo::RefPtr<Cairo::ImageSurface>mGaissBitmap;

protected:

    int mRadii[4];

    int mPaddingLeft;
    int mPaddingRight;
    int mPaddingTop;
    int mPaddingBottom;

    int mMaskColor;

    float mColorDev;
    int mColorNoiseMin;
    int mColorNoiseMax;
public:
    // 使用每个像素赋值的做法，ARGB转换RGB
    // fromview：模糊图像的源控件
    // rect：模糊图像，在fromview相对的位置以及大小
    // ksize：模糊半径（越大越模糊）
    // scale：先将图像缩小的倍率（加快模糊时间，但失真更严重，建议2-3，越大越模糊）
    // colorDev：每个像素乘以一个系数，以达到类似蒙版的效果
    // noiseMin、noiseMax：抖动随机数的最大值最小值，建议是 -2、2
    GaussFilterDrawable(View *fromView,Rect rect,int ksize,int scale,float colorDev /* = 1.0 */, int noiseMin /* = -1 */, int noiseMax /* = 1 */);
    // 使用 pixman ，ARGB转换RGB。
    // fromview：模糊图像的源控件
    // rect：模糊图像，在fromview相对的位置以及大小
    // ksize：模糊半径（越大越模糊）
    // scale：先将图像缩小的倍率（加快模糊时间，但失真更严重，建议2-3，越大越模糊）
    // maskColor：蒙版的颜色
    GaussFilterDrawable(View *fromView,Rect rect,int ksize,int scale /* = 2*/,int maskColor/* = 0x66000000 */ , bool isGauss = true);
    ~GaussFilterDrawable();
    void setGaussBitmip(Cairo::RefPtr<Cairo::ImageSurface> &bmp,Rect rect);
    void setGaussBitmip(View *fromView,Rect rect);
    void setGaussRadius(int radius);    // 设置 高斯半径（越高越模糊）
    void setGaussRegion(Rect rect);     // 设置 高斯模糊 的区域
    void setGaussColorDev(float colorDev);// 设置 高斯模糊 像素的偏差值（*colorDev,如0.4，则把整个模糊区域变暗
    void setGaussColorNoiseRange(int min,int max);// 设置 高斯模糊 像素抖动的取值范围(减少颜色的条纹)
    void computeBitmapGasuss();         // 计算 高斯模糊
    void computeBitmapSize();           // 计算 size

    void setCornerRadii(int radius);    // 设置 四边 的圆角（适合用作弹窗时使用）
    void setCornerRadii(int topLeftRadius,int topRightRadius,int bottomRightRadius,int bottomLeftRadius); // 分别设置 四边 的圆角
    void setPadding(int LeftPadding,int topPadding,int rightPadding,int bottomPadding); // 设置四边padding（若以上的圆角设置负值，则需配合padding来实现）

    void draw(Canvas&canvas)override;
};

#endif

