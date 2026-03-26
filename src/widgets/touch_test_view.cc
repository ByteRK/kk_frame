/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-26 18:06:25
 * @LastEditTime: 2026-03-26 22:11:28
 * @FilePath: /kk_frame/src/widgets/touch_test_view.cc
 * @Description: 触摸测试组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "touch_test_view.h"
#include <cassert>
#include <cmath>

DECLARE_WIDGET(TouchTestView);

TouchTestView::TouchTestView(int w, int h) : View(w, h) {
    mLayoutDirty = true;
}

TouchTestView::TouchTestView(cdroid::Context* ctx, const cdroid::AttributeSet& attrs)
    : View(ctx, attrs) {
    mLayoutDirty = true;
}

void TouchTestView::setTestPadding(int l, int t, int r, int b) {
    mPadding.left = std::max(mPadding.left, 0);
    mPadding.top = std::max(mPadding.top, 0);
    mPadding.right = std::max(mPadding.right, 0);
    mPadding.bottom = std::max(mPadding.bottom, 0);
    mLayoutDirty = true;
    invalidate();
}

void TouchTestView::setBlockSize(int w, int h) {
    mCfg.blockW = w;
    mCfg.blockH = h;
    mLayoutDirty = true;
    invalidate();
}

void TouchTestView::setBlockCount(int v, int h) {
    mCfg.verCount = v;
    mCfg.horCount = h;
    mLayoutDirty = true;
    invalidate();
}

void TouchTestView::setBlockColor(double r, double g, double b) {
    mCfg.r = r;
    mCfg.g = g;
    mCfg.b = b;
    invalidate();
}

void TouchTestView::setPointColor(double r, double g, double b) {
    mPt.rC = r;
    mPt.gC = g;
    mPt.bC = b;
    invalidate(mPt.x - mPt.r, mPt.y - mPt.r, 2 * mPt.r, 2 * mPt.r);
}

void TouchTestView::setPointRadius(int r) {
    invalidate(mPt.x - mPt.r, mPt.y - mPt.r, 2 * mPt.r, 2 * mPt.r);
    mPt.r = r;
    invalidate(mPt.x - mPt.r, mPt.y - mPt.r, 2 * mPt.r, 2 * mPt.r);
}

void TouchTestView::setOnAllBlocksActivated(OnAllBlocksActivated cb) {
    mOnAllBlocksActivated = std::move(cb);
}

void TouchTestView::setOnTouchPointChanged(OnTouchPointChange cb) {
    mOnTouchPointChanged = std::move(cb);
}

void TouchTestView::validateConfig() {
    assert(mCfg.verCount >= 3 && mCfg.verCount % 2 == 1);
    assert(mCfg.horCount >= 3 && mCfg.horCount % 2 == 1);
    assert(mCfg.blockW > 0 && mCfg.blockH > 0);
}

void TouchTestView::rebuildLayout() {
    LOGV("rebuildLayout");

    validateConfig();

    mBlocks.clear();

    int contentW = getWidth() - mPadding.left - mPadding.right;
    int contentH = getHeight() - mPadding.top - mPadding.bottom;

    int innerW = (contentW - 2 * mCfg.blockW) / (mCfg.horCount - 2);
    int innerH = (contentH - 2 * mCfg.blockH) / (mCfg.verCount - 2);

    assert(innerW > 0 && innerH > 0);

    int x = mPadding.left;
    int y = mPadding.top;

    // 上横
    mBlocks.push_back({ x,y,mCfg.blockW,mCfg.blockH });
    for (int i = 0;i < mCfg.horCount - 2;i++) {
        x += (i == 0 ? mCfg.blockW : innerW);
        mBlocks.push_back({ x,y,innerW,mCfg.blockH });
    }
    x += innerW;
    mBlocks.push_back({ x,y,mCfg.blockW,mCfg.blockH });

    // 右竖
    for (int i = 0;i < mCfg.verCount - 2;i++) {
        y += (i == 0 ? mCfg.blockH : innerH);
        mBlocks.push_back({ x,y,mCfg.blockW,innerH });
    }
    y += innerH;
    mBlocks.push_back({ x,y,mCfg.blockW,mCfg.blockH });

    // 下横
    for (int i = 0;i < mCfg.horCount - 2;i++) {
        x -= innerW;
        mBlocks.push_back({ x,y,innerW,mCfg.blockH });
    }
    x -= mCfg.blockW;
    mBlocks.push_back({ x,y,mCfg.blockW,mCfg.blockH });

    // 左竖
    for (int i = 0;i < mCfg.verCount - 2;i++) {
        y -= innerH;
        mBlocks.push_back({ x,y,mCfg.blockW,innerH });
    }

    // 中横
    {
        int midRow = (mCfg.verCount - 3) / 2;
        int yMid = mPadding.top + mCfg.blockH + midRow * innerH;
        int x = mPadding.left + mCfg.blockW;

        for (int i = 0;i < mCfg.horCount - 2;i++) {
            mBlocks.push_back({ x,yMid,innerW,mCfg.blockH });
            x += innerW;
        }
    }

    // 中竖
    {
        int midCol = (mCfg.horCount - 3) / 2;
        int xMid = mPadding.left + mCfg.blockW + midCol * innerW;
        int y = mPadding.top + mCfg.blockH;

        for (int i = 0;i < mCfg.verCount - 2;i++) {
            if (i == (mCfg.verCount - 3) / 2) {
                y += innerH;
                continue;
            }
            mBlocks.push_back({ xMid,y,mCfg.blockW,innerH });
            y += innerH;
        }
    }

    mActiveCount = 0;
    mCompleted = false;

    mLayoutDirty = false;

    LOGV("blocks=%d", (int)mBlocks.size());
}

int TouchTestView::hitTest(int x, int y) const {
    for (int i = 0;i < (int)mBlocks.size();++i) {
        const auto& b = mBlocks[i];
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) {
            LOGV("hit block=%d", i);
            return i;
        }
    }
    return -1;
}

void TouchTestView::activateBlock(int idx) {
    assert(idx >= 0 && idx < (int)mBlocks.size());

    auto& b = mBlocks[idx];
    if (b.active) return;

    b.active = true;
    mActiveCount++;

    LOGV("activate idx=%d count=%d", idx, mActiveCount);

    invalidateBlock(idx);

    if (!mCompleted) {
        if (mActiveCount == (int)mBlocks.size()) {
            mCompleted = true;
            LOGV("ALL BLOCKS ACTIVATED");
            if (mOnAllBlocksActivated) mOnAllBlocksActivated();
        }
    }
}

void TouchTestView::resetBlocks() {
    if (mActiveCount == 0) return;

    LOGV("resetBlocks");

    for (auto& b : mBlocks)
        b.active = false;

    mActiveCount = 0;
    mCompleted = false;

    invalidate();
}

void TouchTestView::invalidateBlock(int idx) {
    const auto& b = mBlocks[idx];
    invalidate(b.x, b.y, b.w, b.h);
}

bool TouchTestView::onTouchEvent(cdroid::MotionEvent& e) {
    if (mLayoutDirty) rebuildLayout();

    int x = e.getX();
    int y = e.getY();

    LOGV("touch (%d,%d)", x, y);

    int idx = hitTest(x, y);

    if (idx < 0) {
        resetBlocks();
    } else {
        activateBlock(idx);
    }

    // 差异触发回调
    if (!mHasTouch || mPt.x != x || mPt.y != y) {
        if (mOnTouchPointChanged) {
            mOnTouchPointChanged(x, y);
        }
    }
    mHasTouch = true;

#if 0
    int oldX = mPt.x;
    int oldY = mPt.y;
    mPt.x = x;
    mPt.y = y;
    int left = std::min(oldX, x) - mPt.r;
    int top = std::min(oldY, y) - mPt.r;
    int right = std::max(oldX, x) + mPt.r;
    int bottom = std::max(oldY, y) + mPt.r;
    invalidate(left, top, right - left, bottom - top);
#else
    invalidate(mPt.x - mPt.r, mPt.y - mPt.r, 2 * mPt.r, 2 * mPt.r);
    mPt.x = x;
    mPt.y = y;
    invalidate(mPt.x - mPt.r, mPt.y - mPt.r, 2 * mPt.r, 2 * mPt.r);
#endif

    return true;
}

void TouchTestView::onDraw(cdroid::Canvas& c) {
    if (mLayoutDirty) rebuildLayout();

    for (const auto& b : mBlocks) {
        c.rectangle(b.x, b.y, b.w, b.h);

        if (b.active) {
            c.set_source_rgb(mCfg.r, mCfg.g, mCfg.b);
            c.fill_preserve();
        }

        c.set_source_rgb(mCfg.r, mCfg.g, mCfg.b);
        c.set_line_width(2);
        c.stroke();
    }

#if 1
    const auto& b = mBlocks[0];
    c.rectangle(b.x, b.y, b.w, b.h);
    if (b.active) {
        c.set_source_rgb(1.f - mCfg.r, 1.f - mCfg.g, 1.f - mCfg.b);
        c.fill_preserve();
    }
    c.set_source_rgb(1.f - mCfg.r, 1.f - mCfg.g, 1.f - mCfg.b);
    c.set_line_width(2);
    c.stroke();
#endif

    c.set_source_rgb(mPt.rC, mPt.gC, mPt.bC);
    c.arc(mPt.x, mPt.y, mPt.r, 0, M_PI * 2);
    c.fill();
}