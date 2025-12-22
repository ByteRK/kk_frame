/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-12-02 16:31:37
 * @FilePath: /kk_frame/src/viewlibs/pickerLayoutManager.h
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#ifndef __PICKERLAYOUTMANAGER_H__
#define __PICKERLAYOUTMANAGER_H__

#include <core/context.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>
#include <widgetEx/recyclerview/snaphelper.h>
#include <widgetEx/recyclerview/recyclerview.h>
#include <view/view.h>

class PickerLayoutManager :public cdroid::LinearLayoutManager {
public:
    /// @brief 计算模式
    enum {
        MEASURE_BY_COUND,    // 根据显示数量计算recycleview Padding
        MEASURE_BY_CENTER,   // 根据中间项宽高以及recycleview宽高进行计算 （viewflipper不适用）
    };
    /// @brief 停止时，显示在中间的View的监听
    typedef std::function<void(View* view, int position)> OnSelectedViewListener;
protected:
    const std::string       TAG;
    float                   mScale = 0.5f;
    bool                    mIsAlpha = true;
    SnapHelper*             mSnapHelper;
    OnSelectedViewListener  mOnSelectedViewListener;
    int                     mItemViewWidth;
    int                     mItemViewHeight;
    int                     mItemCount = -1;
    RecyclerView*           mRecyclerView;
    int                     mOrientation;
    int                     mMeasureMode;

    int                     mCenterPosition;

    bool                    mCanScrollVertically = true;
    bool                    mCanScrollHorizontally = true;

public:
    PickerLayoutManager(Context* context, int orientation, bool reverseLayout);
    PickerLayoutManager(Context* context, RecyclerView* recyclerView, int orientation, bool reverseLayout, int itemCount, float scale, bool isAlpha, int measureMode = MEASURE_BY_CENTER);

    /// @brief 添加SnapHelper
    /// @param view
    void onAttachedToWindow(RecyclerView& view) override;

    /// @brief 没有指定显示条目的数量时，RecyclerView的宽高由自身确定
    /// @brief 指定显示条目的数量时，根据MeasureMode计算RecyclerView的宽高
    /// @param recycler
    /// @param state
    /// @param widthSpec
    /// @param heightSpec
    void onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec)override;

    void onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state)override;

    int scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;

    int scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;

    void setCanScrollVertically(bool canScrollVertically);

    void setCanScrollHorizontally(bool canScrollHorizontally);

    void setSnapHelper(SnapHelper* snapHelper);

    SnapHelper* getSnapHelper();

private:
    /// @brief 横向情况下的缩放
    virtual void scaleHorizontalChildView();

    /// @brief 竖向方向上的缩放
    virtual void scaleVerticalChildView();

public:
    /// @brief 当滑动停止时触发回调
    /// @param state
    void onScrollStateChanged(int state)override;

    void setOnSelectedViewListener(OnSelectedViewListener listener);
};

#endif