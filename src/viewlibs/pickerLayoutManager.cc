/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-06-03 17:51:16
 * @FilePath: /kk_frame/src/viewlibs/pickerLayoutManager.cc
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Ricken, All Rights Reserved. 
 * 
 */

#include <cdlog.h>
#include <algorithm>

#include "pickerLayoutManager.h"

PickerLayoutManager::PickerLayoutManager(Context* context, int orientation, bool reverseLayout)
    :cdroid::LinearLayoutManager(context, orientation, reverseLayout), TAG("PickerLayoutManager") {
    mOnSelectedViewListener = nullptr;
    mSnapHelper = nullptr;
    mOrientation = orientation;
    mCenterPosition = 0;
}


PickerLayoutManager::PickerLayoutManager(Context* context, RecyclerView* recyclerView, int orientation, bool reverseLayout, int itemCount, float scale, bool isAlpha, int measureMode)
    :cdroid::LinearLayoutManager(context, orientation, reverseLayout), TAG("PickerLayoutManager") {
    mOnSelectedViewListener = nullptr;
    mSnapHelper = nullptr;
    mItemCount = itemCount;
    mOrientation = orientation;
    mRecyclerView = recyclerView;
    mIsAlpha = isAlpha;
    mScale = scale;
    mMeasureMode = measureMode;
    mCenterPosition = 0;
    if (mItemCount != 0) setAutoMeasureEnabled(false);
}

/// @brief 添加SnapHelper
/// @param view 
void PickerLayoutManager::onAttachedToWindow(RecyclerView* view) {
    cdroid::LinearLayoutManager::onAttachedToWindow(view);
    if (mSnapHelper)
        mSnapHelper->attachToRecyclerView(view);
}


/// @brief 没有指定显示条目的数量时，RecyclerView的宽高由自身确定
/// @brief 指定显示条目的数量时，根据方向分别计算RecyclerView的宽高
/// @brief (item数量乘上每一项宽度必须小于recycleview宽度)
/// @param recycler
/// @param state
/// @param widthSpec 
/// @param heightSpec 
void PickerLayoutManager::onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec) {
#define USERECYCLESELF_HEIGHT true
    if (getItemCount() != 0 && mItemCount != 0) {
        RecyclerView::ViewHolder* holder = mRecyclerView->getAdapter()->createViewHolder(mRecyclerView, 0);
        View* view = holder->itemView;
        measureChildWithMargins(view, widthSpec, heightSpec);
        mItemViewWidth = view->getMeasuredWidth();
        mItemViewHeight = view->getMeasuredHeight();

        if (mOrientation == HORIZONTAL) {
            int paddingHorizontal = 0;
            if (mMeasureMode == MEASURE_BY_COUND)
                paddingHorizontal = (mItemCount - 1) / 2 * mItemViewWidth;
            else
                paddingHorizontal = (getWidth() - mItemViewWidth) / 2;
            mRecyclerView->setClipToPadding(false);
            mRecyclerView->setPadding(paddingHorizontal, 0, paddingHorizontal, 0);
#if USERECYCLESELF_HEIGHT
            setMeasuredDimension(
                mItemViewWidth * mItemCount,
                LayoutManager::chooseSize(heightSpec, getPaddingTop() + getPaddingBottom(), getMinimumHeight())
            );
#else
            setMeasuredDimension(mItemViewWidth * mItemCount, mItemViewHeight);
#endif
        } else if (mOrientation == VERTICAL) {
            int paddingVertical = 0;
            if (mMeasureMode == MEASURE_BY_COUND)
                paddingVertical = (mItemCount - 1) / 2 * mItemViewHeight;
            else
                paddingVertical = (getHeight() - mItemViewHeight) / 2;
            mRecyclerView->setClipToPadding(false);
            mRecyclerView->setPadding(0, paddingVertical, 0, paddingVertical);
#if USERECYCLESELF_HEIGHT
            setMeasuredDimension(
                LayoutManager::chooseSize(widthSpec, getPaddingLeft() + getPaddingRight(), getMinimumWidth()),
                mItemViewHeight * mItemCount
            );
#else
            setMeasuredDimension(mItemViewWidth, mItemViewHeight * mItemCount);
#endif
        }
        delete holder;
    } else {
        cdroid::LinearLayoutManager::onMeasure(recycler, state, widthSpec, heightSpec);
    }
}


/// @brief 
/// @param recycler 
/// @param state 
void PickerLayoutManager::onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    cdroid::LinearLayoutManager::onLayoutChildren(recycler, state);
    if (getItemCount() < 0 || state.isPreLayout()) return;
    scrollToPosition(mCenterPosition);
    if (mOrientation == HORIZONTAL) {
        scaleHorizontalChildView();
    } else if (mOrientation == VERTICAL) {
        scaleVerticalChildView();
    }
}

/// @brief 
/// @param dx 
/// @param recycler 
/// @param state 
/// @return 
int PickerLayoutManager::scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    if (!mCanScrollHorizontally)return 0;
    scaleHorizontalChildView();
    return cdroid::LinearLayoutManager::scrollHorizontallyBy(dx, recycler, state);
}


/// @brief 
/// @param dy 
/// @param recycler 
/// @param stat 
/// @return 
int PickerLayoutManager::scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    if (!mCanScrollVertically)return 0;
    scaleVerticalChildView();
    return cdroid::LinearLayoutManager::scrollVerticallyBy(dy, recycler, state);
}

void PickerLayoutManager::setCanScrollVertically(bool canScrollVertically) {
    mCanScrollVertically = canScrollVertically;
}

void PickerLayoutManager::setCanScrollHorizontally(bool canScrollHorizontally) {
    mCanScrollHorizontally = canScrollHorizontally;
}

void PickerLayoutManager::setSnapHelper(SnapHelper* snapHelper) {
    if (mSnapHelper == snapHelper)return;
    if (mSnapHelper)delete mSnapHelper;
    mSnapHelper = snapHelper;
    if (mIsAttachedToWindow)mSnapHelper->attachToRecyclerView(mRecyclerView);
}

SnapHelper* PickerLayoutManager::getSnapHelper() {
    return mSnapHelper;
}


/// @brief 横向情况下的缩放
void PickerLayoutManager::scaleHorizontalChildView() {
    float mid = getWidth() / 2.0f;
    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);
        float childMid = (getDecoratedLeft(child) + getDecoratedRight(child)) / 2.0f;
        float scale = 1.0f + (-1 * (1 - mScale)) * (std::min(mid, std::abs(mid - childMid))) / mid;
        child->setScaleX(scale);
        child->setScaleY(scale);
        if (mIsAlpha) {
            child->setAlpha(scale);
        }
    }
}

/// @brief 竖向方向上的缩放
void PickerLayoutManager::scaleVerticalChildView() {
    float mid = getHeight() / 2.0f;
    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);
        float childMid = (getDecoratedTop(child) + getDecoratedBottom(child)) / 2.0f;
        float scale = 1.0f + (-1 * (1 - mScale)) * (std::min(mid, std::abs(mid - childMid))) / mid;
        child->setScaleX(scale);
        child->setScaleY(scale);
        if (mIsAlpha) {
            child->setAlpha(scale);
        }
    }
}

/// @brief 当滑动停止时触发回调
/// @param state 
void PickerLayoutManager::onScrollStateChanged(int state) {
    LinearLayoutManager::onScrollStateChanged(state);
    if (state == RecyclerView::SCROLL_STATE_IDLE) {
        if (mOnSelectedViewListener && mSnapHelper) {
            View* view = mSnapHelper->findSnapView(*this);
            if (view == nullptr) {
                LOGE("Can not found SnapView !!!");
                return;
            }
            mCenterPosition = getPosition(view);
            mOnSelectedViewListener(view, mCenterPosition);
        }
    }
}

/// @brief 设置选中项归位监听
/// @brief 滑动完全停止后响应
/// @param listener 
void PickerLayoutManager::setOnSelectedViewListener(OnSelectedViewListener listener) {
    mOnSelectedViewListener = listener;
}