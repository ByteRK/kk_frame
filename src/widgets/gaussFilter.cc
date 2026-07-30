/*
 * @Author: hanakami
 * @Date: 2026-02-25 17:05:53
 * @email: hanakami@163.com
 * @LastEditTime: 2026-03-05 16:41:31
 * @FilePath: /hana/src/widgets/gaussFilter.cc
 * @Description:
 * Copyright (c) 2026 by hanakami, All Rights Reserved.
 */

#include "gaussFilter.h"
#include "math_utils.h"

GaussFilter::GaussFilter(int radius, float scale, int maskColor, bool dither)
    : mRadius(radius), mScale(scale), mMaskColor(maskColor), mDither(dither)
{
}

void GaussFilter::apply(Canvas &canvas, const Rect &rect)
{
    Cairo::RefPtr<Cairo::Pattern> pattern = canvas.get_source();
    if(pattern->get_type() != Cairo::Pattern::Type::SURFACE){
        LOGE("GaussFilter only supports surface pattern as source");
        return;
    }
    auto surfacePattern = std::dynamic_pointer_cast<Cairo::SurfacePattern>(pattern);
    if (!surfacePattern) {
        LOGE("Failed to cast to SurfacePattern");
        return;
    }
    
    canvas.mask(surfacePattern);
    
    auto target = canvas.get_target();
    auto surface = std::dynamic_pointer_cast<Cairo::ImageSurface>(target);
    
    surface->write_to_png("surface.png");
    auto imageSurface = std::dynamic_pointer_cast<Cairo::ImageSurface>(surface);
    if (!imageSurface) {
        LOGE("Failed to cast to ImageSurface");
        return;
    }
    
    
    LOGE("Got surface from group: %dx%d", imageSurface->get_width(), imageSurface->get_height());
    
    auto workingSurface = Cairo::ImageSurface::create(
        Cairo::Surface::Format::ARGB32,
        imageSurface->get_width(),
        imageSurface->get_height()
    );
    
    memcpy(workingSurface->get_data(),
           imageSurface->get_data(),
           imageSurface->get_height() * imageSurface->get_stride());
    
    //计算需要模糊的区域
    Rect gaussRect = calculateGaussRect(rect, imageSurface);
    
    // 提取并缩放
    unsigned char* scaledData = extractAndScaleData(workingSurface, gaussRect);
    if (!scaledData) return;
    
    int scaledWidth = std::round(gaussRect.width * mScale);
    int scaledHeight = std::round(gaussRect.height * mScale);
    
#if defined(USE_NEON)
    unsigned char* blurredData = (unsigned char*)malloc(scaledWidth * scaledHeight * 4);
    MathUtils::gaussianBlur2(scaledData, blurredData, scaledHeight, scaledWidth, 4, mRadius);
    free(scaledData);
    scaledData = blurredData;
#else
    MathUtils::gaussianBlur(scaledData, scaledWidth, scaledHeight, mRadius);
#endif
    // 应用蒙版
    if (mMaskColor != 0) {
        applyMaskColor(scaledData, scaledWidth, scaledHeight, mMaskColor);
    }
    
    // 缩放回原始尺寸并绘制
    scaleAndDraw(canvas, rect, gaussRect, scaledData, scaledWidth, scaledHeight);
    
    free(scaledData);
}
Rect GaussFilter::calculateGaussRect(const Rect &rect, Cairo::RefPtr<Cairo::ImageSurface> surface)
{
    int radius = std::round((float)mRadius / mScale);

    int left = std::max(0, rect.left - radius);
    int top = std::max(0, rect.top - radius);
    int right = std::min(surface->get_width(), rect.right() + radius);
    int bottom = std::min(surface->get_height(), rect.bottom() + radius);

    return Rect::Make(left, top, right - left, bottom - top);
}

unsigned char *GaussFilter::extractAndScaleData(Cairo::RefPtr<Cairo::ImageSurface> surface, const Rect &rect)
{
    int scaledWidth = std::round(rect.width * mScale);
    int scaledHeight = std::round(rect.height * mScale);

    // 4字节对齐
    if (scaledWidth % 4 != 0)
    {
        scaledWidth = (scaledWidth + 3) & ~3;
        // 重新计算实际缩放比例，确保纵横比一致
        float actualScale = (float)scaledWidth / rect.width;
        scaledHeight = std::round(rect.height * actualScale);
    }

    LOGV("extractAndScaleData: rect(%d,%d,%d,%d) -> scaled(%dx%d) scale=%.2f",
         rect.left, rect.top, rect.width, rect.height,
         scaledWidth, scaledHeight, mScale);

    unsigned char *scaledData = (unsigned char *)malloc(scaledWidth * scaledHeight * 4);
    memset(scaledData, 0, scaledWidth * scaledHeight * 4); // 初始化为0（透明）

    // 使用 pixman 进行缩放
    pixman_image_t *srcImage = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        surface->get_width(),
        surface->get_height(),
        (uint32_t *)surface->get_data(),
        surface->get_stride());

    pixman_image_t *dstImage = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        scaledWidth,
        scaledHeight,
        (uint32_t *)scaledData,
        scaledWidth * 4);

    pixman_transform_t transform;

    pixman_transform_init_scale(&transform,
                                pixman_double_to_fixed(1.0 / mScale),
                                pixman_double_to_fixed(1.0 / mScale));

    pixman_image_set_filter(srcImage, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_set_transform(srcImage, &transform);

    pixman_image_composite(PIXMAN_OP_SRC, srcImage, NULL, dstImage,
                           rect.left, rect.top,        // 源位置
                           0, 0,                       // 目标位置从(0,0)开始
                           0, 0,                       // 蒙版位置
                           scaledWidth, scaledHeight); // 目标大小

    pixman_image_unref(srcImage);
    pixman_image_unref(dstImage);

    return scaledData;
}

void GaussFilter::applyMaskColor(unsigned char *data, int width, int height, int color)
{
    uint32_t *pixels = (uint32_t *)data;
    int count = width * height;

    uint32_t maskColor = color;
    uint8_t maskA = (color >> 24) & 0xFF;

    if (maskA == 0xFF)
    {
        // 完全不透明蒙版：直接替换
        for (int i = 0; i < count; i++)
        {
            pixels[i] = maskColor;
        }
    }
    else if (maskA > 0)
    {
        // 半透明蒙版：混合
        for (int i = 0; i < count; i++)
        {
            uint32_t src = pixels[i];
            uint32_t dst = maskColor;

            // 简单的 alpha 混合
            uint8_t a = (src >> 24) & 0xFF;
            uint8_t r = (src >> 16) & 0xFF;
            uint8_t g = (src >> 8) & 0xFF;
            uint8_t b = src & 0xFF;

            uint8_t ma = (dst >> 24) & 0xFF;
            uint8_t mr = (dst >> 16) & 0xFF;
            uint8_t mg = (dst >> 8) & 0xFF;
            uint8_t mb = dst & 0xFF;

            uint8_t outA = (a * (255 - ma) + ma * 255) / 255;
            uint8_t outR = (r * (255 - ma) + mr * ma) / 255;
            uint8_t outG = (g * (255 - ma) + mg * ma) / 255;
            uint8_t outB = (b * (255 - ma) + mb * ma) / 255;

            pixels[i] = (outA << 24) | (outR << 16) | (outG << 8) | outB;
        }
    }
}

void GaussFilter::scaleAndDraw(Canvas &canvas, const Rect &dstRect, const Rect &srcRect, unsigned char *blurredData, int blurredWidth, int blurredHeight)
{
    // 创建临时表面
    auto tempSurface = Cairo::ImageSurface::create(
        blurredData,
        Cairo::Surface::Format::ARGB32,
        blurredWidth,
        blurredHeight,
        blurredWidth * 4);

    float scaleX = (float)srcRect.width / blurredWidth;
    float scaleY = (float)srcRect.height / blurredHeight;

    canvas.save();

    canvas.rectangle(dstRect.left, dstRect.top, dstRect.width, dstRect.height);
    canvas.clip();

    // 变换到目标位置
    canvas.translate(dstRect.left, dstRect.top);

    canvas.scale(scaleX, scaleY);

    LOGE("scaleAndDraw: dstRect(%d,%d,%d,%d) srcRect(%d,%d,%d,%d) scaleX=%.2f scaleY=%.2f",
         dstRect.left, dstRect.top, dstRect.width, dstRect.height,
         srcRect.left, srcRect.top, srcRect.width, srcRect.height,
         scaleX, scaleY);
    canvas.translate(-srcRect.left, -srcRect.top);

    
    tempSurface->write_to_png("surface111.png");
    // 设置源并绘制
    canvas.set_source(tempSurface, 0, 0);

    auto pattern = canvas.get_source_for_surface();
    if (pattern)
    {
        pattern->set_filter(mDither ? Cairo::SurfacePattern::Filter::BILINEAR : Cairo::SurfacePattern::Filter::NEAREST);
    }
    canvas.paint();
    canvas.restore();
}

void GaussFilter::drawToGroup(Canvas &canvas, const Rect &dstRect, const Rect &srcRect,
                              unsigned char *blurredData, int blurredWidth, int blurredHeight)
{
    auto tempSurface = Cairo::ImageSurface::create(
        blurredData,
        Cairo::Surface::Format::ARGB32,
        blurredWidth,
        blurredHeight,
        blurredWidth * 4);
    
    float scaleX = (float)dstRect.width / srcRect.width;
    float scaleY = (float)dstRect.height / srcRect.height;
    
    // 变换到正确位置
    canvas.translate(dstRect.left, dstRect.top);
    canvas.scale(scaleX, scaleY);
    canvas.translate(-srcRect.left, -srcRect.top);
    
    canvas.set_source(tempSurface, 0, 0);
    
    auto pattern = canvas.get_source_for_surface();
    if (pattern) {
        pattern->set_filter(mDither ? Cairo::SurfacePattern::Filter::BILINEAR : 
                                      Cairo::SurfacePattern::Filter::NEAREST);
    }
    
    canvas.paint();
}

void GaussFilter::saveCanvasToPNG(Canvas &canvas, const std::string &filename)
{
    auto surface = canvas.get_target();
    if (surface)
    {
        surface->write_to_png(filename);
        std::cout << "Saved canvas to " << filename << std::endl;
    }
}