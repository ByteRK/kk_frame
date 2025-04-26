/*
 * @Author: cy
 * @Email: 964028708@qq.com
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2025-02-18 19:28:55
 * @FilePath: /cy_frame/src/viewlibs/rkLayoutManage.cc
 * @Description: 
 * @BugList: 
 * 
 * Copyright (c) 2024 by Cy, All Rights Reserved. 
 * 
 */

#include "rkLayoutManage.h"
#include "pickerSnapHelper.h"

#include <widgetEx/recyclerview/linearsnaphelper.h>
#include <widgetEx/recyclerview/pagersnaphelper.h>

RKLayoutManage::RKLayoutManage(Context* context, int orientation, bool reverseLayout)
    :PickerLayoutManager(context, orientation, reverseLayout) {
    init();
}

RKLayoutManage::RKLayoutManage(Context* context, RecyclerView* recyclerView, int orientation, bool reverseLayout, int itemCount, bool isAlpha)
    :PickerLayoutManager(context, recyclerView, orientation, reverseLayout, itemCount, 1.f, isAlpha) {
    init();
}

RKLayoutManage::~RKLayoutManage() {
    delete mSnapHelper;
}

void RKLayoutManage::runExpandAnim() {
    setEnableExpandAnim(true);
    mExpandAnimFinish = false;
    mRecyclerView->requestLayout();
}

void RKLayoutManage::setEnableExpandAnim(bool enable) {
    mEnableExpandAnim = enable;
}

void RKLayoutManage::setExpandAnimDuration(long duration) {
    if (duration < 100) {
        LOGE("ExpandAnimDuration must be greater than 100ms");
        return;
    }
    mExpandAnimDuration = duration;
}

void RKLayoutManage::setSnapHelper(int snapHelperType) {
    if (mSnapHelper)delete mSnapHelper;
    mSnapHelper = nullptr;
    switch (snapHelperType) {
    case SNAPHELPER_PAGER:
        mSnapHelper = new PagerSnapHelper();
        break;
    case SNAPHELPER_LINEAR:
        mSnapHelper = new LinearSnapHelper();
        break;
    default:
        mSnapHelper = new PickerSnapHelper();
        break;
    }
    if (mIsAttachedToWindow)
        mSnapHelper->attachToRecyclerView(mRecyclerView);
}

void RKLayoutManage::setTransformList(const std::vector<TransformStruct>& transformList) {
    mTransformList = transformList;
    // if (mRecyclerView)
    mRecyclerView->requestLayout();
}

void RKLayoutManage::setOnCenterViewChangeListener(OnCenterViewChangeListener listener) {
    mOnCenterViewChangeListener = listener;
}

void RKLayoutManage::init() {
    // 设置定位类
    setSnapHelper(SNAPHELPER_FAST);

    mOldCenterPosition = 0;
    mOldCenterView = nullptr;

    mExpandAnimDuration = 1200;
    mEnableExpandAnim = false;
    mExpandAnimFinish = false;

    View* recyclerView = mRecyclerView;
    mAnimatorListener.onAnimationStart = [recyclerView](Animator& anim, bool reverse) {
        recyclerView->setOnTouchListener([ ](View&, MotionEvent&) {return true;});
        };
    mAnimatorListener.onAnimationCancel = [recyclerView](Animator& anim) {
        recyclerView->setOnTouchListener(nullptr);
        };
}

void RKLayoutManage::scaleHorizontalChildView() {
    float boxCenterX = getWidth() / 2.0f;  // 容器中心线

    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);

        float decoratedLeft = getDecoratedLeft(child);    // 左边界
        float decoratedRight = getDecoratedRight(child);  // 右边界

        float centerX = (decoratedLeft + decoratedRight) / 2.0f; // ITEM中心线
        float offsetX = centerX - boxCenterX;                    // X轴差值
        float position = offsetX / getWidth();                   // 所在位置
        float absX = std::abs(position);                       // 差值绝对值

        // 判断滑动中间项
        if (decoratedLeft < boxCenterX && decoratedRight > boxCenterX && mOldCenterView != child) {
            int centerPosition = getPosition(child);
            if (mOnCenterViewChangeListener)mOnCenterViewChangeListener(mOldCenterPosition, centerPosition);
            mOldCenterPosition = centerPosition;
            mOldCenterView = child;
        }

        // 计算变化值
        float scaleFactor = 1.f, alphaFactor = 1.f, indentX = 0.f, indentY = 0.f;
        calculateScaleValue(position, scaleFactor, alphaFactor, indentX, indentY);

        // 开始变换
        startChildScale(child, absX, offsetX, scaleFactor, alphaFactor, indentX, indentY);
    }
}

void RKLayoutManage::scaleVerticalChildView() {
    float boxCenterY = getHeight() / 2.0f;  // 容器中心线

    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);

        float decoratedTop = getDecoratedTop(child);        // 上边界
        float decoratedBottom = getDecoratedBottom(child);  // 下边界

        float centerY = (decoratedTop + decoratedBottom) / 2.0f; // ITEM中心线
        float offsetY = centerY - boxCenterY;  // Y轴差值
        float position = offsetY / getHeight(); // 所在位置
        float absY = std::abs(position);

        // 判断滑动中间项
        if (decoratedTop < boxCenterY && decoratedBottom > boxCenterY && mOldCenterView != child) {
            int centerPosition = getPosition(child);
            if (mOnCenterViewChangeListener)mOnCenterViewChangeListener(mOldCenterPosition, centerPosition);
            mOldCenterPosition = centerPosition;
            mOldCenterView = child;
        }

        // 计算变化值
        float scaleFactor = 1.f, alphaFactor = 1.f, indentX = 0.f, indentY = 0.f;
        calculateScaleValue(position, scaleFactor, alphaFactor, indentX, indentY);

        // 开始变换
        startChildScale(child, absY, offsetY, scaleFactor, alphaFactor, indentX, indentY);
    }
}

void RKLayoutManage::calculateScaleValue(const float& position, float& scaleFactor, float& alphaFactor, float& indentX, float& indentY) {
    const float abs = std::abs(position);
    // 判断变化参数
    int index = 0;
    for (int count = mTransformList.size(); index < count; index++) {
        if (index == count - 1 || abs < mTransformList[index + 1].position)
            break;
    }

    // 计算变化值
    if (index != mTransformList.size()) {
        const TransformStruct& transform = mTransformList.at(index);
        if (index < mTransformList.size() - 1) {
            const TransformStruct& transform_next = mTransformList.at(index + 1);
            // 计算比例
            while (transform.position == transform_next.position)
                LOGE("position is same");
            float proportion = (abs - transform.position) / (transform.position - transform_next.position);
            scaleFactor = proportion * (transform.scale - transform_next.scale) + transform.scale;
            if (mIsAlpha)alphaFactor = proportion * (transform.alpha - transform_next.alpha) + transform.alpha;
            indentX = proportion * (transform.transX - transform_next.transX) + transform.transX;
            indentY = proportion * (transform.transY - transform_next.transY) + transform.transY;

            // 根据位移类型修改位移参数
            indentX = transform.transXType == TRANSFER_ABS ? indentX
                : indentX * (position > 0 ? 1 : -1) * transform.transXType;
            indentY = transform.transYType == TRANSFER_ABS ? indentY
                : indentY * (position > 0 ? 1 : -1) * transform.transYType;
        } else {
            scaleFactor = transform.scale;
            indentX = transform.transX;
            indentY = transform.transY;
        }
    }
}

void RKLayoutManage::startChildScale(View* child, const float& abs, const float& offset,
    const float& scaleFactor, const float& alphaFactor, const float& indentX, const float& indentY) {
    bool isHorizontal = mOrientation == LinearLayout::HORIZONTAL;

    child->setScaleX(scaleFactor);
    child->setScaleY(scaleFactor);
    child->setZ(10 - abs * 10);
    if (mEnableExpandAnim && !mExpandAnimFinish && abs <= 0.5 && abs >= 0.1) { // 展开动画
        child->animate().cancel();
        child->setAlpha(.0f);
        child->setLayerType(View::LAYER_TYPE_SOFTWARE);
        View* recyclerView = mRecyclerView;
        mAnimatorListener.onAnimationEnd = [child, alphaFactor, recyclerView](Animator& anim, bool reverse) {
            if (alphaFactor >= 1.f) child->setLayerType(View::LAYER_TYPE_NONE); // 动画播放结束后恢复LayerType
            recyclerView->setOnTouchListener(nullptr);
            };
        if (isHorizontal) {
            child->setTranslationX(-offset);
            child->animate().translationX(indentX).alpha(alphaFactor).\
                setDuration(mExpandAnimDuration).setListener(mAnimatorListener).start();
        } else {
            child->setTranslationY(-offset);
            child->animate().translationY(indentY).alpha(alphaFactor).\
                setDuration(mExpandAnimDuration).setListener(mAnimatorListener).start();
        }
    } else {
        child->setAlpha(alphaFactor);
        if (alphaFactor >= 1.f) child->setLayerType(View::LAYER_TYPE_NONE);
        if (isHorizontal) child->setTranslationX(indentX);
        else child->setTranslationY(indentY);
    }

    if (isHorizontal) child->setTranslationY(indentY);
    else child->setTranslationX(indentX);
}

void RKLayoutManage::onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    PickerLayoutManager::onLayoutChildren(recycler, state);
    mExpandAnimFinish = true;
}

