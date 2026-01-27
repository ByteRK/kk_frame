/*
 * @Author: xialc
 * @Email: 
 * @Date: 2026-01-27 11:27:25
 * @LastEditTime: 2026-01-27 11:43:58
 * @FilePath: /kk_frame/src/widgets/gauss_view.cc
 * @Description: 毛玻璃效果
 * @BugList: 
 * 
 * Copyright (c) 2026 by xialc, All Rights Reserved. 
 * 
**/

#if defined(ENABLE_GAUSS_VIEW) || defined(__VSCODE__)

#include "gauss_view.h"
#include "utils/math_utils.h"
#include <core/windowmanager.h>
typedef unsigned char uchar;

DECLARE_WIDGET(GaussView)

class GaussViewPrivate {
private:
    int                                mLeft;
    int                                mTop;
    int                                mWidth;
    int                                mHeight;
    uchar *                            mData;
    uchar *                            mData2;
    cairo_surface_t *                  mSurface;
    Cairo::RefPtr<Cairo::ImageSurface> mSurface2;

public:
    GaussViewPrivate() {
        mLeft    = 0;
        mTop     = 0;
        mWidth   = 0;
        mHeight  = 0;
        mData    = 0;
        mData2   = 0;
        mSurface = 0;
    }
    ~GaussViewPrivate() {
        if (mSurface) cairo_surface_destroy(mSurface);
        if (mData) free(mData);
        if (mData2) free(mData2);
    }
    bool moved(int x, int y, int w, int h) {
        int stride;
        if (mData && (x == mLeft && mTop == y && mWidth == w && mHeight == h)) return false;
        if (!mSurface || w != mWidth || h != mHeight) {
            if (mSurface) {
                cairo_surface_destroy(mSurface);
                mSurface = 0;
            }
            if (mData) {
                free(mData);
                mData = 0;
            }
            stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
            mData  = (uchar *)malloc(h * stride);
            mSurface2.reset();
            if (mData2) {
                free(mData2);
                mData2 = 0;
            }
        }
        if (!mData) {
            LOGE("data is null. size=%dx%d stride=%d", w, h, stride);
            return false;
        }
        if (!mSurface && !(mSurface = cairo_image_surface_create_for_data(mData, CAIRO_FORMAT_ARGB32, w, h, stride))) {
            LOGE("cairo_image_surface_create_for_data fail. size=%dx%d stride=%d", w, h, stride);
            return false;
        }
        mLeft   = x;
        mTop    = y;
        mWidth  = w;
        mHeight = h;
        return true;
    }
    cairo_surface_t *surface() {
        return mSurface;
    }
    uchar *data() {
        return mData;
    }
    void dump() {
        if (mSurface) cairo_surface_write_to_png(mSurface, "glass.png");
    }
    cairo_surface_t *surface2() {
        if (!mSurface2) mSurface2 = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, mWidth, mHeight);
        return mSurface2->cobj();
    }
    uchar *data2() {
        return mSurface2->get_data();
    }
    void dump2() {
        if (mSurface2) mSurface2->write_to_png("glass2.png");
    }
    void save() {
        if (!mData2) mData2 = (uchar *)malloc(mHeight * cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, mWidth));
        memcpy(mData2, data2(), mHeight * cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, mWidth));
    }
    void restore() {
        if (mData2) memcpy(data2(), mData2, mHeight * cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, mWidth));
    }
};

GaussView::GaussView(Context *ctx, const AttributeSet &attrs) : View(ctx, attrs) {
    initGaussViewData();

    mRadius   = attrs.getDimensionPixelSize("radius", mRadius);
    mOnceDraw = attrs.getBoolean("onceDraw", true);

    int fillet_radius  = attrs.getDimensionPixelSize("filletRadius", 0);
    mTopLeftRadius     = attrs.getDimensionPixelSize("topLeftRadius", fillet_radius);
    mTopRightRadius    = attrs.getDimensionPixelSize("topRightRadius", fillet_radius);
    mBottomLeftRadius  = attrs.getDimensionPixelSize("bottomLeftRadius", fillet_radius);
    mBottomRightRadius = attrs.getDimensionPixelSize("bottomRightRadius", fillet_radius);
}

GaussView::GaussView(int w, int h) : View(w, h) {
    initGaussViewData();
}

GaussView::~GaussView() {}

void GaussView::setRadius(int mRadius) {
    if (mRadius != mRadius) {
        mRadius = mRadius;
        invalidate();
    }
}

void GaussView::setOnceDraw(bool flag) {
    mOnceDraw = flag;
}

void GaussView::setColor(uint color) {
    if (mColor == color) return;
    mColor = color;
    invalidate();
}

void GaussView::setFilletRadius(int r) {
    if (mTopLeftRadius == r) return;
    mTopLeftRadius = mTopRightRadius = mBottomLeftRadius = mBottomRightRadius = r;
    invalidate();
}

void GaussView::setTopLeftRadius(int r) {
    if (mTopLeftRadius == r) return;
    mTopLeftRadius = r;
    invalidate();
}

void GaussView::setTopRightRadius(int r) {
    if (mTopRightRadius == r) return;
    mTopRightRadius = r;
    invalidate();
}

void GaussView::setBottomLeftRadius(int r) {
    if (mBottomLeftRadius == r) return;
    mBottomLeftRadius = r;
    invalidate();
}

void GaussView::setBottomRightRadius(int r) {
    if (mBottomRightRadius == r) return;
    mBottomRightRadius = r;
    invalidate();
}

void GaussView::rotatePos(int cw, int ch, int w, int h, int &x, int &y) {
    int rotation = WindowManager::getInstance().getDefaultDisplay().getRotation();
    switch (rotation) {
    case Display::ROTATION_90: {
        int t = x;
        x     = y;
        y     = ch - t - w;
    } break;
    case Display::ROTATION_180: {
        x = cw - x - w;
        y = ch - y - h;
    } break;
    case Display::ROTATION_270: {
        int t = x;
        x     = cw - h - y;
        y     = t;
    } break;
    default: break;
    }
}

void GaussView::draw(Canvas &ctx) {
    if (mRadius <= 0) {
        LOGW("Params not support!!! id=%d-%p", getId(), this);
        return;
    }

    int w = getWidth();
    int h = getHeight();
    if (w == 1 || h == 1) return;

    int xy[2];
    getLocationInWindow(xy);

    int    channel        = 4;
    double scale_factor   = 0.5;
    double restore_factor = 1 / scale_factor;

    cairo_surface_t *src_sf, *dst_sf;
    cairo_t *        src_cr, *dst_cr;

    int scaled_width  = w * scale_factor;
    int scaled_height = h * scale_factor;

    if (scaled_width < 1 || scaled_height < 1) return;

    Cairo::RefPtr<Cairo::ImageSurface> img = std::dynamic_pointer_cast<Cairo::ImageSurface>(ctx.get_target());

    int cw = img->get_width();
    int ch = img->get_height();

    int rotation = WindowManager::getInstance().getDefaultDisplay().getRotation();

    // int cn     = 0;
    // int ct[10] = {0};
    // SC_CT_INIT(0);

    src_sf = ctx.get_target()->cobj();
    src_cr = ctx.cobj();

    rotatePos(cw, ch, w, h, xy[0], xy[1]);
    if (rotation == Display::ROTATION_90 || rotation == Display::ROTATION_270) {
        std::swap<int>(scaled_width, scaled_height);
    }

    if (d_ptr->moved(xy[0], xy[1], scaled_width, scaled_height) || !mOnceDraw) {
        if (mOnceDraw) {
            /* 裁剪 */
            dst_sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, scaled_width, scaled_height);
            dst_cr = cairo_create(dst_sf);
            cairo_scale(dst_cr, scale_factor, scale_factor); /* 缩小 */
            cairo_set_operator(dst_cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(dst_cr, src_sf, -xy[0], -xy[1]);
            cairo_paint(dst_cr);
            // SC_CT_DIFF(0, ct[cn++]); // 8

            /* 高斯处理 */
            MathUtils::gaussianBlur3(cairo_image_surface_get_data(dst_sf), d_ptr->data(), scaled_width, scaled_height, channel,
                          mRadius * scale_factor);
            // SC_CT_DIFF(0, ct[cn++]); // 81

            cairo_destroy(dst_cr);
            cairo_surface_destroy(dst_sf);
        } else {
#if 0
            /* 被遮盖的脏区边缘有像素变化的需要触发该控件全区域刷新 invalidate */
            std::vector<Cairo::RectangleInt> clip_rectangle;
            ctx.get_clip_rectangle_list(clip_rectangle);

            cairo_t *d_cr = cairo_create(d_ptr->surface2());

            // static int glass_index = 0;
            // glass_index++;

            double              ox, oy;
            Cairo::RectangleInt ir;
            Cairo::RectangleInt sr = {xy[0], xy[1], w, h};
            if (rotation == Display::ROTATION_90 || rotation == Display::ROTATION_270) {
                std::swap<int>(sr.width, sr.height);
            }
            // LOGD("%d. rect(%d,%d)[%dx%d]", glass_index, sr.x, sr.y, sr.width, sr.height);
            for (int i = 0; i < clip_rectangle.size(); i++) {
                Cairo::RectangleInt r = clip_rectangle.at(i);
                rotatePos(cw, ch, r.width, r.height, r.x, r.y);
                if (rotation == Display::ROTATION_90 || rotation == Display::ROTATION_270) {
                    std::swap<int>(r.width, r.height);
                }
                ir.x      = _MAX(sr.x, r.x);
                ir.y      = _MAX(sr.y, r.y);
                ir.width  = _MIN(sr.x + sr.width, r.x + r.width) - ir.x;
                ir.height = _MIN(sr.y + sr.height, r.y + r.height) - ir.y;

                // LOGD("%d-%d. rect(%d,%d)[%dx%d] intersection(%d,%d)[%dx%d] (%.f,%.f)", glass_index, i, r.x, r.y,
                // r.width, r.height, ir.x, ir.y, ir.width,
                //      ir.height, ox, oy);

                if (ir.width <= 0 || ir.height <= 0) continue;

                ox = (ir.x - sr.x) * scale_factor;
                oy = (ir.y - sr.y) * scale_factor;

                cairo_save(d_cr);
                cairo_translate(d_cr, ox, oy);
                cairo_rectangle(d_cr, 0, 0, ir.width * scale_factor, ir.height * scale_factor);
                cairo_clip(d_cr);
                cairo_scale(d_cr, scale_factor, scale_factor);
                cairo_set_operator(d_cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_surface(d_cr, src_sf, -ir.x, -ir.y);
                cairo_paint(d_cr);
                cairo_restore(d_cr);

                // char       filename[256];
                // snprintf(filename, sizeof(filename), "glass2_%d_%d.png", glass_index, i);
                // cairo_surface_write_to_png(d_ptr->surface2(), filename);
            }
            // SC_CT_DIFF(0, ct[cn++]); // ?

            d_ptr->save();
            MathUtils::gaussianBlur3(d_ptr->data2(), d_ptr->data(), cairo_image_surface_get_width(d_ptr->surface()),
                          cairo_image_surface_get_height(d_ptr->surface()), channel, mRadius * scale_factor);
            d_ptr->restore();
            // SC_CT_DIFF(0, ct[cn++]); // ?

            cairo_destroy(d_cr);
#endif
        }
    }

    /* 绘制 */
    if (d_ptr->surface()) {

        /* 圆角 */
        if (mTopLeftRadius > 0 || mTopRightRadius > 0 || mBottomLeftRadius > 0 || mBottomRightRadius > 0) {
            int      r;
            int      x  = 0;
            int      y  = 0;
            cairo_t *cr = src_cr;

            r = mTopLeftRadius;
            cairo_new_path(cr);
            cairo_move_to(cr, x + r, y);

            r = mTopRightRadius;
            cairo_line_to(cr, x + w - r, y);
            if (r > 0) { cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0); }

            r = mBottomRightRadius;
            cairo_line_to(cr, x + w, y + h - r);
            if (r > 0) { cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2); }

            r = mBottomLeftRadius;
            cairo_line_to(cr, x + r, y + h);
            if (r > 0) { cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI); }

            r = mTopLeftRadius;
            cairo_line_to(cr, x, y + r);
            if (r > 0) { cairo_arc(cr, x + r, y + r, r, M_PI, M_PI * 3 / 2); }

            cairo_close_path(cr);
            cairo_clip(cr);
        }

        if (rotation != Display::ROTATION_0) {
            ctx.save();
            switch (rotation) {
            case Display::ROTATION_90: {
                ctx.translate(w, 0);
                ctx.rotate(M_PI_2);
            } break;
            case Display::ROTATION_180: {
                ctx.translate(w, h);
                ctx.rotate(M_PI);
            } break;
            case Display::ROTATION_270: {
                ctx.translate(0, h);
                ctx.rotate(-M_PI_2);
            } break;
            default: break;
            }
        }
        ctx.scale(restore_factor, restore_factor); /* 放大 */
        cairo_set_source_surface(src_cr, d_ptr->surface(), 0, 0);
        ctx.paint();
        if (rotation != Display::ROTATION_0) ctx.restore();

        if (mColor >> 24) {
            Color cr(mColor);
            ctx.set_source_rgba(cr.red(), cr.green(), cr.blue(), cr.alpha());
            ctx.fill();
        }

        // SC_CT_DIFF(0, ct[cn++]); // 24
    }

    // LOGE("Size=(%dx%d) GlassTime=(%d,%d,%d,%d,%d,%d)", w, h, ct[0], ct[1], ct[2], ct[3], ct[4], ct[5]);
}

void GaussView::initGaussViewData() {
    mRadius            = 10;
    mOnceDraw          = true;
    mTopLeftRadius     = 0;
    mTopRightRadius    = 00;
    mBottomLeftRadius  = 0;
    mBottomRightRadius = 0;
    mColor             = 0;
    d_ptr              = std::make_shared<GaussViewPrivate>();
}

#endif
