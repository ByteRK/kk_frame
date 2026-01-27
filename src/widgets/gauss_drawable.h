/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2025-05-16 14:52:37
 * @LastEditTime: 2026-01-27 13:49:50
 * @FilePath: /kk_frame/src/widgets/gauss_drawable.h
 * @Description: 高斯模糊
 * @BugList:1、不使用fromview的方法，待测试
 *          2、不适用fromview的方法，scale的值为3时，概率会出问题，待处理
 * 
 * @Use:
 * 
 * 1:
 *         GaussDrawable* gaussDrawable = new GaussDrawable({ 78,0,1842,462 }, 50, 0.5f, 0x66000000, true);
 *         View *fromView = g_window->getPage()->getRootView();
 *         Cairo::RefPtr<Cairo::ImageSurface> bitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, fromView->getWidth(), fromView->getHeight());
 *         Canvas* mcanvas = new Canvas(bitmap);
 *         fromView->draw(*mcanvas);
 *         delete mcanvas;
 *         gaussDrawable->setBitmap(bitmap);
 *         mPopGaussVg->setBackground(gaussDrawable);
 * 
 * 2:
 *         mPopGaussVg->setBackground(new GaussDrawable(getRootView(), mGaussRadius, 0.5f, mGaussColor, true));
 * 
 * 
 * Copyright (c) 2025 by cy, All Rights Reserved. 
 * 
*/


#if defined(ENABLE_GAUSS_DRAWABLE) || defined(__VSCODE__)

#ifndef __GAUSS_DRAWABLE_H__
#define __GAUSS_DRAWABLE_H__

#include <view/view.h>

class GaussDrawable:public Drawable{

private:
    unsigned char *mBitmapData = nullptr;
    unsigned char *mGaussData = nullptr;
    
    View        *mFromView = nullptr;
    int         mGaussRadius;
    double      mScale;         // 传入的缩放比例
    double      mGaussScale;    // 高斯计算使用的缩放，与原本的 mScale 可能会不同，需结合缩放后的宽高计算

    Rect        mDrawRegion;    // 绘制区域
    Rect        mGaussRegion;   // 高斯模糊区域，需在绘制区域上，扩大 mGaussRadius 的宽高
    int         mGaussWidth;
    int         mGaussHeight;
    bool        mDrawOnce = false;  // 是否只绘制一次
    Cairo::RefPtr<Cairo::ImageSurface>mBitmap;
    Cairo::RefPtr<Cairo::ImageSurface>mDrawBitmap;
    Cairo::RefPtr<Cairo::ImageSurface>mGaissBitmap;

protected:

    int mRadii[4];

    int mPaddingLeft;
    int mPaddingRight;
    int mPaddingTop;
    int mPaddingBottom;

    int mMaskColor;

public:
    /// @brief 模糊控件
    /// @param fromView 模糊图像的挂载对象
    /// @param ksize 模糊半径（越大越模糊）
    /// @param scale 先将图像缩小的倍率（加快模糊时间，但失真更严重，建议0.3 - 0.5，越小越模糊）
    /// @param maskColor 蒙版的颜色
    /// @param drawOnce 是否只绘制一次
    GaussDrawable(View *fromView,int ksize = 50,double scale  = 2,int maskColor = 0x66000000, bool drawOnce = false);

    /// @brief 模糊控件
    /// @param rect 模糊图像，在fromview相对的位置以及大小
    /// @param ksize 模糊半径（越大越模糊）
    /// @param scale 先将图像缩小的倍率（加快模糊时间，但失真更严重，建议2-3，越大越模糊）
    /// @param maskColor 蒙版的颜色
    /// @param drawOnce 是否只绘制一次
    /// @note 该版本图像源从canvas、获取
    GaussDrawable(Rect rect,int ksize = 50 ,double scale  = 2,int maskColor = 0x66000000, bool drawOnce = false);
    
    ~GaussDrawable();
    void computeBitmapGasuss();         // 计算 高斯模糊
    void computeBitmapSize();           // 计算 size
    void extendedRect(Rect srcRect, Rect &dstRect); // 根据模糊半径，拓展矩形
    void rotatePixelsDirect(uint32_t* srcData, uint32_t* dstData, Rect srcRect, Rect copyRect,  int rotation); // 旋转 并 区域拷贝

    void setBitmap(Cairo::RefPtr<Cairo::ImageSurface> bitmap); // 设置 源图像(该用法是不使用canvas的图像数据,而是直接使用bitmap,搭配 drawOnce 为 true 使用)
    void setGaussRadius(int radius);    // 设置 高斯半径（越高越模糊）
    void setCornerRadii(int radius);    // 设置 四边 的圆角（适合用作弹窗时使用）
    void setCornerRadii(int topLeftRadius,int topRightRadius,int bottomRightRadius,int bottomLeftRadius); // 分别设置 四边 的圆角
    void setPadding(int LeftPadding,int topPadding,int rightPadding,int bottomPadding); // 设置四边padding（若以上的圆角设置负值，则需配合padding来实现）

    void draw(Canvas&canvas)override;
};

#endif

#endif
