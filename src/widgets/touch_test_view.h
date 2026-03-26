/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-26 18:06:25
 * @LastEditTime: 2026-03-26 18:06:51
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

class TouchTestView :public cdroid::View {
protected:
    struct {
        int left{ 20 };         // 左边距
        int top{ 20 };          // 上边距
        int right{ 20 };        // 右边距
        int bottom{ 20 };       // 下边距
    } mExcludePadding; // 测试边距

    struct {
        double red{ 0.8f };     // 测试块红
        double green{ 0.5f };   // 测试块绿
        double blue{ 0.3f };    // 测试块蓝
        int width{ 40 };        // 测试块宽度
        int height{ 40 };       // 测试块高度
        int verCount{ 11 };     // 垂直测试块数量(必须为奇数)
        int horCount{ 19 };     // 水平测试块数量(必须为奇数)
    } mSetBlocks; // 设置的测试块信息

private:
    struct {
        int x{ 0 };             // X坐标
        int y{ 0 };             // Y坐标
        int r{ 6 };             // 半径
        double red{ 0.3f };     // 红
        double green{ 0.3f };   // 绿
        double blue{ 0.3f };    // 蓝
    } mTouchPoints; // 触摸点

    struct {
        int width{ 0 };         // 实际测试块宽度
        int height{ 0 };        // 实际测试块高度
        bool init{ false };     // 是否已初始化
    } mRealBlocks; // 实际测试块信息
    std::vector<bool> mBlockStatus;      // 测试块状态

    int  mGridSize;

public:
    TouchTestView(int w, int h);
    TouchTestView(cdroid::Context* ctx, const cdroid::AttributeSet& attrs);

    bool onTouchEvent(cdroid::MotionEvent& evt) override;
    void onDraw(cdroid::Canvas& canvas) override;

    void setPadding(int left, int top, int right, int bottom) override;

    void setBlockSize(int width, int height);
    void setBlockCount(int verCount, int horCount);
    void setBlockColor(double red, double green, double blue);

    void setPointColor(double red, double green, double blue);
    void setPointRadius(int r);

private:
    void init();
    void drawGrid(cdroid::Canvas& canvas);
    void drawTestBlock(cdroid::Canvas& canvas);
    void drawTestBlock(cdroid::Canvas& c, int i, int x, int y, int w, int h, double r, double g, double b);

    void checkTouchPoint();
};

#endif // __TOUCH_TEST_VIEW_H__