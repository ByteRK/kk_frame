/*
 * @Author: hanakami
 * @Date: 2026-02-25 17:06:03
 * @email: hanakami@163.com
 * @LastEditTime: 2026-02-26 04:46:08
 * @FilePath: /hana/src/widgets/gaussFilter.h
 * @Description: 
 * Copyright (c) 2026 by hanakami, All Rights Reserved. 
 */
#ifndef GAUSS_FILTER_H
#define GAUSS_FILTER_H

#include <view/view.h>
#include <pixman.h>
#include <drawable/colorfilters.h>

class GaussFilter : public ColorFilter {
protected:
    int mRadius;        // 模糊半径
    float mScale;       // 缩放比例
    int mMaskColor;     // 蒙版颜色
    bool mDither;       // 是否抖动

public:

    GaussFilter(int radius = 50, float scale = 2.0f, int maskColor = 0x66000000, bool dither = false);
    void apply(Canvas& canvas, const Rect& rect) override;
    // void apply(Canvas& canvas, const Rect& rect,Cairo::RefPtr<Cairo::ImageSurface> source) override;
    void setRadius(int radius) { mRadius = radius; }
    void setScale(float scale) { mScale = scale; }
    void setMaskColor(int color) { mMaskColor = color; }
    void setDither(bool dither) { mDither = dither; }

    int getRadius() const { return mRadius; }
    float getScale() const { return mScale; }
    int getMaskColor() const { return mMaskColor; }

private:
    Rect calculateGaussRect(const Rect& rect, Cairo::RefPtr<Cairo::ImageSurface> surface);

    unsigned char* extractAndScaleData(Cairo::RefPtr<Cairo::ImageSurface> surface, const Rect& rect);

    void applyMaskColor(unsigned char* data, int width, int height, int color);

    void scaleAndDraw(Canvas& canvas, const Rect& dstRect, const Rect& srcRect,
                     unsigned char* blurredData, int blurredWidth, int blurredHeight);
    void drawToGroup(Canvas &canvas, const Rect &dstRect, const Rect &srcRect, unsigned char *blurredData, int blurredWidth, int blurredHeight);
    void saveCanvasToPNG(Canvas &canvas, const std::string &filename);
};

#endif // GAUSS_FILTER_H