
#include "paint.h"
#include <cdlog.h>
#include <core/color.h>

Paint::Paint(cdroid::Canvas &ctx) : mCanvas(&ctx)
{
    initData();
}

Paint::Paint()
{
    initData();
}

void Paint::initData()
{
    mAntiAlias   = true;
    mColor       = cdroid::Color::WHITE;
    mStrokeWidth = 1;
    mStyle       = STROKE;
    mStrokeCap   = Cairo::Context::LineCap::ROUND;
    mTextSize    = 15;
    mTypeface    = cdroid::Typeface::create("Droid Sans Fallback", cdroid::Typeface::NORMAL);
}

Paint::~Paint()
{
}

void Paint::setStyle(int flags)
{
    mStyle = flags;
}

void Paint::setAntiAlias(bool val)
{
    mAntiAlias = val;
}

void Paint::setColor(UINT val)
{
    mColor = val;
}

void Paint::setFillColor(UINT val)
{
    mFillColor = val;
}

void Paint::setStrokeWidth(int val)
{
    mStrokeWidth = val;
}

void Paint::setStrokeCap(Cairo::Context::LineCap strokeCap)
{
    mStrokeCap = strokeCap;
}

void Paint::setTextSize(int val)
{
    mTextSize = val;
}

void Paint::setTypeface(cdroid::Typeface *typeface) {
    if (typeface){
        mTypeface = typeface;
    }
}

void Paint::getTextSpace(const std::string &txt, double *w, double *h) {
    if (!w && !h) return;

    // 创建一个 Cairo 图面
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);

    // 创建一个 Cairo 绘图上下文
    cairo_t *cr = cairo_create(surface);

    // 设置字体大小和样式
    cairo_select_font_face(cr, mTypeface->getFamily().c_str()
        , mTypeface->isItalic() ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL
        , mTypeface->isBold() ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);

    cairo_set_font_size(cr, mTextSize);

    // 文本内容
    const char *text = txt.c_str();

    // 计算文本的宽度
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);

    if (w) *w = extents.width + extents.x_bearing;
    if (h) *h = extents.height;

    // 销毁绘图上下文和图面
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void Paint::drawText(int x, int y, int w, int h, const std::string &txt
                     , int alignment/* = 0*/
                     , cdroid::Canvas *ctx/* = nullptr*/)
{
    if (ctx == nullptr)
        ctx = mCanvas;

    ctx->begin_new_path();

    if (mAntiAlias)
    {
        ctx->set_antialias(Cairo::Antialias::ANTIALIAS_DEFAULT);
    }

    ctx->set_color(mColor);
    ctx->set_font_size(mTextSize);
    ctx->draw_text(cdroid::RECT::Make(x, y, w, h), txt, alignment);

    LOGV("x=%d y=%d w=%d h=%d txt=[%s] alignment=%d color=%08x size=%d", x, y, w, h
        , txt.c_str(), alignment, mColor, mTextSize);
}

void Paint::drawArc(int x, int y, int radius, int sAngle, int eAngle
                    , cdroid::Canvas *ctx/* = nullptr*/)
{
    if (ctx == nullptr)
        ctx = mCanvas;

    ctx->begin_new_path();

    if (mAntiAlias)
    {
        ctx->set_antialias(Cairo::Antialias::ANTIALIAS_DEFAULT);
    }

    ctx->set_line_cap(mStrokeCap);
    ctx->set_line_width(mStrokeWidth);
    ctx->set_color(mColor);
    ctx->arc(x, y, radius, sAngle / 180.0 * M_PI, eAngle / 180.0 * M_PI);

    if (mStyle & STROKE)
    {
        ctx->stroke_preserve();
    }

    if (mStyle & FILL)
    {
        ctx->set_color(mFillColor);
        ctx->fill();
    }
}
