/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-26 18:06:25
 * @LastEditTime: 2026-03-26 22:13:51
 * @FilePath: /kk_frame/src/widgets/touch_test_view.h
 * @Description: 触摸测试组件
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __TOUCH_TEST_VIEW_H__
#define __TOUCH_TEST_VIEW_H__

#include <view/view.h>
#include <vector>
#include <functional>

class TouchTestView : public cdroid::View {
public:
    DECLARE_UIEVENT(void,OnAllBlocksActivated);
    DECLARE_UIEVENT(void,OnTouchPointChange,int, int);

public:
    TouchTestView(int w, int h);
    TouchTestView(cdroid::Context* ctx, const cdroid::AttributeSet& attrs);

    bool onTouchEvent(cdroid::MotionEvent& evt) override;
    void onDraw(cdroid::Canvas& canvas) override;

    void setTestPadding(int left, int top, int right, int bottom);
    void setBlockSize(int width, int height);
    void setBlockCount(int verCount, int horCount);
    void setBlockColor(double red, double green, double blue);

    void setPointColor(double red, double green, double blue);
    void setPointRadius(int r);

    void setOnAllBlocksActivated(OnAllBlocksActivated cb);
    void setOnTouchPointChanged(OnTouchPointChange cb);

private:
    struct Padding {
        int left{ 20 }, top{ 20 }, right{ 20 }, bottom{ 20 };
    } mPadding;

    struct Config {
        double r{ 0.8 }, g{ 0.5 }, b{ 0.3 };
        int blockW{ 40 };
        int blockH{ 40 };
        int verCount{ 11 };
        int horCount{ 19 };
    } mCfg;

    struct TouchPoint {
        int x{ 0 }, y{ 0 }, r{ 6 };
        double rC{ 0.3 }, gC{ 0.3 }, bC{ 0.3 };
    } mPt;

    struct Block {
        int x, y, w, h;
        bool active{ false };

        Block(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {
            active = false;
        }
    };

private:
    std::vector<Block> mBlocks;

    int  mActiveCount{ 0 };
    bool mCompleted{ false };

    OnAllBlocksActivated  mOnAllBlocksActivated;
    OnTouchPointChange    mOnTouchPointChanged;

    bool mLayoutDirty{ true };
    bool mHasTouch{ false };

private:
    void validateConfig();
    void rebuildLayout();

    int  hitTest(int x, int y) const;
    void activateBlock(int idx);
    void resetBlocks();

    void invalidateBlock(int idx);
};

#endif