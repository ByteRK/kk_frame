/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-26 18:07:44
 * @LastEditTime: 2026-03-26 19:09:51
 * @FilePath: /kk_frame/src/widgets/touch_test_view.cc
 * @Description: 触摸测试组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "touch_test_view.h"

DECLARE_WIDGET(TouchTestView);

TouchTestView::TouchTestView(int w, int h) : cdroid::View(w, h) {
    init();
}

TouchTestView::TouchTestView(cdroid::Context* ctx, const cdroid::AttributeSet& attrs) : cdroid::View(ctx, attrs) {
    init();
    cdroid::View::setPadding(0, 0, 0, 0);
    cdroid::View::setPaddingRelative(0, 0, 0, 0);
}

bool TouchTestView::onTouchEvent(cdroid::MotionEvent& event) {
    switch (event.getAction()) {
    case cdroid::MotionEvent::ACTION_UP:
    case cdroid::MotionEvent::ACTION_DOWN:
    case cdroid::MotionEvent::ACTION_MOVE:
        invalidate(
            mTouchPoints.x - mTouchPoints.r,
            mTouchPoints.y - mTouchPoints.r,
            mTouchPoints.r + mTouchPoints.r,
            mTouchPoints.r + mTouchPoints.r
        );

        mTouchPoints.x = event.getX();
        mTouchPoints.y = event.getY();

        invalidate(
            mTouchPoints.x - mTouchPoints.r,
            mTouchPoints.y - mTouchPoints.r,
            mTouchPoints.r + mTouchPoints.r,
            mTouchPoints.r + mTouchPoints.r
        );

        checkTouchPoint();
        break;
    default:break;
    }
    return true;
}

void TouchTestView::onDraw(cdroid::Canvas& canvas) {
    // drawGrid(canvas);
    drawTestBlock(canvas);

    canvas.set_source_rgb(mTouchPoints.red, mTouchPoints.green, mTouchPoints.blue);
    canvas.arc(mTouchPoints.x, mTouchPoints.y, mTouchPoints.r, 0, M_PI * 2.f);
    canvas.fill();
}

void TouchTestView::setPadding(int left, int top, int right, int bottom) {
    mExcludePadding.left = left;
    mExcludePadding.top = top;
    mExcludePadding.right = right;
    mExcludePadding.bottom = bottom;
    mRealBlocks.init = false;
    invalidate();
}

void TouchTestView::setBlockSize(int width, int height) {
    mSetBlocks.width = width;
    mSetBlocks.height = height;
    mRealBlocks.init = false;
    invalidate();
}

void TouchTestView::setBlockCount(int verCount, int horCount) {
    mSetBlocks.verCount = verCount;
    mSetBlocks.horCount = horCount;
    mRealBlocks.init = false;
    invalidate();
}

void TouchTestView::setBlockColor(double red, double green, double blue) {
    mSetBlocks.red = red;
    mSetBlocks.green = green;
    mSetBlocks.blue = blue;
    invalidate();
}

void TouchTestView::setPointColor(double red, double green, double blue) {
    mTouchPoints.red = red;
    mTouchPoints.green = green;
    mTouchPoints.blue = blue;
    invalidate(
        mTouchPoints.x - mTouchPoints.r,
        mTouchPoints.y - mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r
    );
}

void TouchTestView::setPointRadius(int r) {
    invalidate(
        mTouchPoints.x - mTouchPoints.r,
        mTouchPoints.y - mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r
    );
    mTouchPoints.r = r;
    invalidate(
        mTouchPoints.x - mTouchPoints.r,
        mTouchPoints.y - mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r,
        mTouchPoints.r + mTouchPoints.r
    );
}

void TouchTestView::init() {
    mGridSize = 10;
}

void TouchTestView::drawGrid(cdroid::Canvas& canvas) {
    for (int x = 0, i = 0;x < getWidth();x += mGridSize, i++) {
        canvas.move_to(x, 0);
        canvas.line_to(x, getHeight());
        canvas.set_source_rgba(.5, .5, .5, (i % 10 == 0) ? 1.f : 0.4f);
        canvas.stroke();
    }
    for (int y = 0, i = 0;y < getHeight();y += mGridSize, i++) {
        canvas.move_to(0, y);
        canvas.line_to(getWidth(), y);
        canvas.set_source_rgba(.5, .5, .5, (i % 10 == 0) ? 1.f : 0.4f);
        canvas.stroke();
    }
}

void TouchTestView::drawTestBlock(cdroid::Canvas& canvas) {
    int i = 0;

    // 当前方块序号
    int index(0);

    // 当前坐标
    int x(mExcludePadding.left), y(mExcludePadding.top);

    // 初始化信息
    if (!mRealBlocks.init) {
        mRealBlocks.width =
            (getWidth() - mExcludePadding.left - mExcludePadding.right - 2 * mSetBlocks.width) /
            (mSetBlocks.horCount - 2);
        mRealBlocks.height =
            (getHeight() - mExcludePadding.top - mExcludePadding.bottom - 2 * mSetBlocks.height) /
            (mSetBlocks.verCount - 2);

        mBlockStatus.clear();
        int blocksCount = (mSetBlocks.horCount * 3) + ((mSetBlocks.verCount - 3) * 3);
        mBlockStatus.resize(blocksCount, false);
        LOGI("blocksCount:%d", blocksCount);

        mRealBlocks.init = true;
    }

    // 首个
    drawTestBlock(
        canvas, index++,
        x, y, mSetBlocks.width, mSetBlocks.height,
        1.f - mSetBlocks.red, 1.f - mSetBlocks.green, 1.f - mSetBlocks.blue);

    // 顶部左到右
    for (i = 0;i < mSetBlocks.horCount - 2;i++) {
        x += (i == 0 ? mSetBlocks.width : mRealBlocks.width);
        drawTestBlock(
            canvas, index++,
            x, y, mRealBlocks.width, mSetBlocks.height,
            mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
        );
    }
    x += mRealBlocks.width;
    drawTestBlock(
        canvas, index++,
        x, y, mSetBlocks.width, mSetBlocks.height,
        mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue);


    // 右边上到下
    for (i = 0;i < mSetBlocks.verCount - 2;i++) {
        y += (i == 0 ? mSetBlocks.height : mRealBlocks.height);
        drawTestBlock(
            canvas, index++,
            x, y, mSetBlocks.width, mRealBlocks.height,
            mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
        );
    }
    y += mRealBlocks.height;
    drawTestBlock(
        canvas, index++,
        x, y, mSetBlocks.width, mSetBlocks.height,
        mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
    );


    // 底部右到左
    for (i = 0;i < mSetBlocks.horCount - 2;i++) {
        x -= mRealBlocks.width;
        drawTestBlock(
            canvas, index++,
            x, y, mRealBlocks.width, mSetBlocks.height,
            mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
        );
    }
    x -= mSetBlocks.width;
    drawTestBlock(
        canvas, index++,
        x, y, mSetBlocks.width, mSetBlocks.height,
        mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
    );


    // 左边下到上
    for (i = 0;i < mSetBlocks.verCount - 2;i++) {
        y -= mRealBlocks.height;
        drawTestBlock(
            canvas, index++,
            x, y, mSetBlocks.width, mRealBlocks.height,
            mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
        );
    }


    // 中间左到右
    y += ((mSetBlocks.verCount - 3) / 2) * mRealBlocks.height;
    for (i = 0;i < mSetBlocks.horCount - 2;i++) {
        x += (i == 0 ? mSetBlocks.width : mRealBlocks.width);
        drawTestBlock(
            canvas, index++,
            x, y, mRealBlocks.width, mSetBlocks.height,
            mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
        );
    }


    // 中间上到下
    y -= ((mSetBlocks.verCount - 3) / 2) * mRealBlocks.height;
    x -= ((mSetBlocks.horCount - 3) / 2) * mRealBlocks.width;
    int center = (mSetBlocks.verCount - 3) / 2;
    for (i = 0;i < mSetBlocks.verCount - 2;i++) {
        y += (i == 0 ? 0 : mRealBlocks.height);
        if (i != center) {
            drawTestBlock(
                canvas, index++,
                x, y, mSetBlocks.width, mRealBlocks.height,
                mSetBlocks.red, mSetBlocks.green, mSetBlocks.blue
            );
        }
    }
}

void TouchTestView::drawTestBlock(cdroid::Canvas& c, int i, int x, int y, int w, int h, double r, double g, double b) {
    // LOGI("i:%d x:%d y:%d w:%d h:%d r:%f g:%f b:%f", i, x, y, w, h, r, g, b);
    c.rectangle(x, y, w, h);
    if (i >= mBlockStatus.size()) {
        LOGE("i:%d >= mBlockStatus.size():%d", i, mBlockStatus.size());
        c.set_source_rgb(1.f, 0.f, 0.f);
        c.fill();
    } else if (mBlockStatus[i]) {
        c.set_source_rgb(r, g, b);
        c.fill_preserve();
        c.set_line_width(2);
        c.stroke();
    } else {
        c.set_source_rgb(r, g, b);
        c.set_line_width(2);
        c.stroke();
    }
}

void TouchTestView::checkTouchPoint() {
    int startPosition;
    int x, y;

    // PADDING区域
    if (
        mTouchPoints.x < mExcludePadding.left || mTouchPoints.x >(getWidth() - mExcludePadding.right) ||
        mTouchPoints.y < mExcludePadding.top || mTouchPoints.y >(getHeight() - mExcludePadding.bottom)
        ) {
        goto resetPointStatus;
    }

    // 处于顶部
    startPosition = 0;
    if (mTouchPoints.y <= mExcludePadding.top + mSetBlocks.height) {
        y = mExcludePadding.top;

        x = mTouchPoints.x - mExcludePadding.left;
        if (x < mSetBlocks.width) {
            if (!mBlockStatus[startPosition]) {
                mBlockStatus[startPosition] = true;
                invalidate(
                    mExcludePadding.left, y,
                    mSetBlocks.width, mSetBlocks.height
                );
            }
        } else {
            x -= mSetBlocks.width;
            int position = x / mRealBlocks.width,
                maxPosition = mSetBlocks.horCount - 2;
            if (position > maxPosition) position = maxPosition;
            if (!mBlockStatus[position + 1]) {
                mBlockStatus[position + 1] = true;
                invalidate(
                    mExcludePadding.left + mSetBlocks.width + position * mRealBlocks.width, y,
                    position == maxPosition ? mSetBlocks.width : mRealBlocks.width, mSetBlocks.height
                );
            }
        }
        return;
    }

    // 处于右侧
    startPosition += mSetBlocks.horCount;
    if (mTouchPoints.x >= (getWidth() - mExcludePadding.right - mSetBlocks.width)) {
        x = getWidth() - mExcludePadding.right - mSetBlocks.width;

        y = mTouchPoints.y - mExcludePadding.top - mSetBlocks.height;
        int position = y / mRealBlocks.height,
            maxPosition = mSetBlocks.verCount - 2;
        if (position > maxPosition) position = maxPosition;
        if (!mBlockStatus[position + startPosition]) {
            mBlockStatus[position + startPosition] = true;
            invalidate(
                x, mExcludePadding.top + mSetBlocks.height + position * mRealBlocks.height,
                mSetBlocks.width, position == maxPosition ? mSetBlocks.height : mRealBlocks.height
            );
        }
        return;
    }

    // 处于底部
    startPosition += mSetBlocks.verCount - 1;
    if (mTouchPoints.y >= (getHeight() - mExcludePadding.bottom - mSetBlocks.height)) {
        y = getHeight() - mExcludePadding.bottom - mSetBlocks.height;
        int forSwap = mSetBlocks.horCount - 2;

        x = mTouchPoints.x - mExcludePadding.left;
        if (x < mSetBlocks.width) {
            if (!mBlockStatus[startPosition + forSwap]) {
                mBlockStatus[startPosition + forSwap] = true;
                invalidate(
                    mExcludePadding.left, y,
                    mSetBlocks.width, mSetBlocks.height
                );
            }
        } else {
            x -= mSetBlocks.width;
            forSwap--;
            int position = x / mRealBlocks.width;
            if (!mBlockStatus[startPosition + (forSwap - position)]) {
                mBlockStatus[startPosition + (forSwap - position)] = true;
                invalidate(
                    mExcludePadding.left + mSetBlocks.width + position * mRealBlocks.width, y,
                    mRealBlocks.width, mSetBlocks.height
                );
            }
        }

        return;
    }

    // 处于左侧
    startPosition += mSetBlocks.horCount - 1;
    if (mTouchPoints.x <= mExcludePadding.left + mSetBlocks.width) {
        x = mExcludePadding.left;
        int forSwap = mSetBlocks.verCount - 3;

        y = mTouchPoints.y - mExcludePadding.top - mSetBlocks.height;
        int position = y / mRealBlocks.height;
        if (!mBlockStatus[startPosition + (forSwap - position)]) {
            mBlockStatus[startPosition + (forSwap - position)] = true;
            invalidate(
                x, mExcludePadding.top + mSetBlocks.height + position * mRealBlocks.height,
                mSetBlocks.width, mSetBlocks.height
            );
        }

        return;
    }

    // 处于中间横向
    startPosition += mSetBlocks.verCount - 2;
    y = mExcludePadding.top + mSetBlocks.height + ((mSetBlocks.verCount - 3) / 2) * mRealBlocks.height;
    if (
        mTouchPoints.y > y &&
        mTouchPoints.y < y + mRealBlocks.height
        ) {
        x = mTouchPoints.x - mExcludePadding.left - mSetBlocks.width;

        int position = x / mRealBlocks.width;
        int maxPosition = mSetBlocks.horCount - 2;
        if (position > maxPosition) position = maxPosition;
        if (!mBlockStatus[startPosition + position]) {
            mBlockStatus[startPosition + position] = true;
            invalidate(
                mExcludePadding.left + mSetBlocks.width + position * mRealBlocks.width, y,
                mRealBlocks.width, mSetBlocks.height
            );
        }

        return;
    }

    // 处于中间纵向
    startPosition += mSetBlocks.horCount - 2;
    x = mExcludePadding.left + mSetBlocks.width + ((mSetBlocks.horCount - 3) / 2) * mRealBlocks.width;
    if (
        mTouchPoints.x > x &&
        mTouchPoints.x < x + mRealBlocks.width
        ) {
        int halfHeight = ((mSetBlocks.verCount - 3) / 2) * mRealBlocks.height;

        y = mTouchPoints.y - mExcludePadding.top - mSetBlocks.height;
        if (y > halfHeight) startPosition--;

        int position = y / mRealBlocks.height;
        if (!mBlockStatus[startPosition + position]) {
            mBlockStatus[startPosition + position] = true;
            invalidate(
                x, mExcludePadding.top + mSetBlocks.height + position * mRealBlocks.height,
                mRealBlocks.width, mRealBlocks.height
            );
        }
        return;
    }

resetPointStatus:
    for (auto it = mBlockStatus.begin();it != mBlockStatus.end();it++) {
        if (*it) {
            *it = false;
            invalidate();
        }
    }
}