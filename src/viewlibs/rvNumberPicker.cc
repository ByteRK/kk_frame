/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2025-09-25 14:05:20
 * @FilePath: /hana_frame/src/viewlibs/rvNumberPicker.cc
 * @Description: 使用RecycleView实现数字选择器
 * 
 * @BugList: 1、暂时不要使用SmoothscrolltoPosition
 *           2、layout_width以及layout_height必须指定数值
 *           3、textColor全透颜色请使用#01000000,暂不支持全0透明度
 * 
 * @Todo:    1、notifyUpdate默认值为false,使用true可以对性能进行优化,但需测试
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#include "rvNumberPicker.h"
#include <widgetEx/recyclerview/orientationhelper.h>
#include <view/layoutinflater.h>
#include <widget/textview.h>
#include <widget/imageview.h>
#include <widget/relativelayout.h>

DECLARE_WIDGET(RVNumberPicker)


/*****************************************适配器***********************************************/

/// @brief 
/// @param pickerView 
RVNumberPicker::RVNumberPickerAdapter::RVNumberPickerAdapter(RVNumberPicker* pickerView) :mFriend(pickerView) {
}

/// @brief 
RVNumberPicker::RVNumberPickerAdapter::~RVNumberPickerAdapter() {
}

/// @brief 
/// @param parent 
/// @param viewType 
/// @return 
RecyclerView::ViewHolder* RVNumberPicker::RVNumberPickerAdapter::onCreateViewHolder(ViewGroup* parent, int viewType) {
    View* view;
    if (viewType == PICKER_TYPE_IMAGE) {
        ImageView* imageView;
        if (mFriend->mOrientation == HORIZONTAL) {
            imageView = new ImageView(LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
            imageView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
            LOGV("[%d]view w:%d", viewType, mFriend->mXMLWidth / mFriend->mDisplayCount);
        } else {
            imageView = new ImageView(LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
            imageView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
            LOGV("[%d]view h:%d", viewType, mFriend->mXMLHeight / mFriend->mDisplayCount);
        }
        // imageView->setBackgroundResource(mFriend->mItemBackground);
        imageView->setScaleType(ScaleType::CENTER_INSIDE);
        imageView->setTag((void*)PICKER_TYPE_IMAGE);
        view = imageView;
    } else {
        TextView* textView;
        if (mFriend->mOrientation == HORIZONTAL) {
            textView = new TextView("", LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
            textView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
            LOGV("[%d]view w:%d", viewType, mFriend->mXMLWidth / mFriend->mDisplayCount);
        } else {
            textView = new TextView("", LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
            textView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
            LOGV("[%d]view h:%d", viewType, mFriend->mXMLHeight / mFriend->mDisplayCount);
        }
        // textView->setBackgroundResource(mFriend->mItemBackground);
        textView->setGravity(mFriend->mGravity);
        textView->setTypeface(mFriend->mFontTypeface);
        textView->setTag((void*)PICKER_TYPE_TEXT);
        view = textView;
        if(!mFriend->mSelectLayout.empty()){
            RelativeLayout *layout;

            TextView *selectView = (TextView *)LayoutInflater::from(parent->getContext())->inflate(mFriend->mSelectLayout,nullptr);
            if(mFriend->mOrientation == HORIZONTAL){
                layout = new RelativeLayout(LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
                layout->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
                selectView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
            }else{
                layout = new RelativeLayout(LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
                layout->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
                selectView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
            }
            selectView->setGravity(mFriend->mGravity);
            selectView->setVisibility(View::INVISIBLE);
            layout->addView(textView);
            layout->addView(selectView);
            layout->setBackgroundColor(0x00FFFFFF);
            view = layout;
        }
       
    }
    return new ViewHolder(view);
}

/// @brief 
/// @param holder 
/// @param position 
void RVNumberPicker::RVNumberPickerAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    View* view;

    int realPosition = position + mFriend->mMinNum;
    if (getItemViewType(position) == PICKER_TYPE_IMAGE) {
        ImageView* imageView = (ImageView*)(holder.itemView);
        if (realPosition < mFriend->mImageList.size())
            imageView->setImageResource(mFriend->mImageList.at(realPosition));
        else
            imageView->setImageResource("#000000");
        view = imageView;
        view->setBackgroundResource(mFriend->mItemBackground);
    } else {
        if(mFriend->mSelectLayout.empty()){
            TextView* textView = (TextView*)(holder.itemView);
            if (mFriend->mNumberFormatter)
                textView->setText(mFriend->mNumberFormatter(realPosition));
            else
                textView->setText(std::to_string(realPosition));
            view = textView;
            view->setBackgroundResource(mFriend->mItemBackground);
        }else{
            ViewGroup *layout = (ViewGroup*)(holder.itemView);
            TextView *textView = (TextView *)layout->getChildAt(0);
            TextView *selectTv = (TextView *)layout->getChildAt(1);
            if (mFriend->mNumberFormatter){
                textView->setText(mFriend->mNumberFormatter(realPosition));
                if(mFriend->mSelectNumberFormatter) selectTv->setText(mFriend->mSelectNumberFormatter(realPosition));
                else                                selectTv->setText(mFriend->mNumberFormatter(realPosition));
            }else{
                textView->setText(std::to_string(realPosition));
                selectTv->setText(std::to_string(realPosition));
            }
            textView->setBackgroundResource(mFriend->mItemBackground);
            view = layout;
        }
    }
    
    if (mFriend->mOnItemClickListener){
        view->setOnClickListener([this, position](View& view) { mFriend->onItemClick(view, position); });
        if(!mFriend->isSoundEffectsEnabled()) view->setSoundEffectsEnabled(false);
    }
    else
        view->setClickable(false);
}

/// @brief 
/// @return 
int RVNumberPicker::RVNumberPickerAdapter::getItemCount() {
    return mFriend->mRealCount;
}

/// @brief 
/// @param position 
/// @return 
int RVNumberPicker::RVNumberPickerAdapter::getItemViewType(int position) {
    return mFriend->mImageList.size() > 0 && position < mFriend->mImageList.size() ? PICKER_TYPE_IMAGE : PICKER_TYPE_TEXT;
}


/*****************************************SmoothScroller***********************************************/

RVNumberPicker::RVSmoothScroller::RVSmoothScroller(Context* context):LinearSmoothScroller(context){
    mDisplayMetrics = context->getDisplayMetrics();
    mSmoothDuration = 300;
}

void RVNumberPicker::RVSmoothScroller::onTargetFound(View* targetView, RecyclerView::State& state, Action& action){
    const int dx = calculateDxToMakeVisible(targetView, getHorizontalSnapPreference());
    const int dy = calculateDyToMakeVisible(targetView, getVerticalSnapPreference());
    const int distance = (int) std::sqrt(dx * dx + dy * dy);
    const int time = rv_CalculateTimeForDeceleration(distance);
    if (time > 0) {
        action.update(-dx, -dy, time, mDecelerateInterpolator);
    }
}   

void RVNumberPicker::RVSmoothScroller::setDuration(int duration){
    mSmoothDuration = duration;
}

int RVNumberPicker::RVSmoothScroller::rv_CalculateTimeForDeceleration(int dx){
    return std::ceil(std::abs(dx) * ((float)mSmoothDuration/ mDisplayMetrics.densityDpi)) / 0.3356f;
}




/*****************************************管理***********************************************/

/// @brief 
/// @param context 
/// @param pickerView 
/// @param orientation 
/// @param reverseLayout 
RVNumberPicker::RVNumberPickerManage::RVNumberPickerManage(Context* context, RVNumberPicker* pickerView, int orientation, bool reverseLayout)
    :LinearLayoutManager(context, orientation, reverseLayout), mFriend(pickerView) {
    init();
}

/// @brief 
RVNumberPicker::RVNumberPickerManage::~RVNumberPickerManage() {
}

/// @brief 监听滑动停止，取得选中项position
/// @param state 
void RVNumberPicker::RVNumberPickerManage::onScrollStateChanged(int state) {
    LinearLayoutManager::onScrollStateChanged(state);
    if (state == RecyclerView::SCROLL_STATE_IDLE && mFriend->mSnapHelper) {
        View* view = mFriend->mSnapHelper->findSnapView(*this);
        if (view == nullptr) {
            LOGE("Can not found SnapView !!!");
            return;
        }
        mFriend->onValueChanged(LinearLayoutManager::getPosition(view));
    }
}

/// @brief 
/// @param view 
void RVNumberPicker::RVNumberPickerManage::onAttachedToWindow(RecyclerView& view) {
    cdroid::LinearLayoutManager::onAttachedToWindow(view);
    scrollToPosition(mFriend->mPosition);
}

/// @brief 
/// @param view 
void RVNumberPicker::RVNumberPickerManage::smoothScrollToPosition(RecyclerView& recyclerView, RecyclerView::State& state,int position){
    RVSmoothScroller* rvSmoothScroller = new RVSmoothScroller(recyclerView.getContext());
    rvSmoothScroller->setDuration(mFriend->mSmoothDuration);
    rvSmoothScroller->setTargetPosition(position);
    startSmoothScroll(rvSmoothScroller);
}

/// @brief 初始化
void RVNumberPicker::RVNumberPickerManage::init() {
    mCenterPositionCache = 0;
}

/// @brief 计算recyclerView位置关系
/// @param recycler 
/// @param state 
/// @param widthSpec 
/// @param heightSpec 
void RVNumberPicker::RVNumberPickerManage::onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec) {
    if (getItemCount() != 0 && mFriend->mDisplayCount != 0) {
        RecyclerView::ViewHolder* holder = mRecyclerView->getAdapter()->createViewHolder(mRecyclerView, 0);
        View* view = holder->itemView;
        measureChildWithMargins(view, widthSpec, heightSpec);
        int itemViewWidth = view->getMeasuredWidth();
        int itemViewHeight = view->getMeasuredHeight();

        mRecyclerView->setClipToPadding(false);
        if (mOrientation == HORIZONTAL) {
            int paddingHorizontal = (mFriend->mDisplayCount - 1) / 2 * itemViewWidth;
            LOGV("mDisplayCount = %d  mRealCound = %d  itemViewWidth = %d  paddingHorizontal = %d", mFriend->mDisplayCount, mFriend->mRealCount, itemViewWidth, paddingHorizontal);
            mRecyclerView->setPadding(paddingHorizontal, 0, paddingHorizontal, 0);
            setMeasuredDimension(
                itemViewWidth * mFriend->mDisplayCount,
                LayoutManager::chooseSize(heightSpec, getPaddingTop() + getPaddingBottom(), getMinimumHeight())
            );
        } else if (mOrientation == VERTICAL) {
            int paddingVertical = (mFriend->mDisplayCount - 1) / 2 * itemViewHeight;
            LOGV("mDisplayCount = %d  mRealCound = %d  itemViewHeight = %d  paddingVertical = %d", mFriend->mDisplayCount, mFriend->mRealCount, itemViewHeight, paddingVertical);
            mRecyclerView->setPadding(0, paddingVertical, 0, paddingVertical);
            setMeasuredDimension(
                LayoutManager::chooseSize(widthSpec, getPaddingLeft() + getPaddingRight(), getMinimumWidth()),
                itemViewHeight * mFriend->mDisplayCount
            );
        }
        delete holder;
    } else {
        cdroid::LinearLayoutManager::onMeasure(recycler, state, widthSpec, heightSpec);
    }
}

/// @brief 
/// @param recycler 
/// @param state 
void RVNumberPicker::RVNumberPickerManage::onLayoutCompleted(State& state){
    LinearLayoutManager::onLayoutCompleted(state);
    if (mOrientation == HORIZONTAL) {
        adjustHorizontalChildView();
    } else if (mOrientation == VERTICAL) {
        adjustVerticalChildView();
    }
}

/// @brief 
/// @param dx 
/// @param recycler 
/// @param state 
/// @return 
int RVNumberPicker::RVNumberPickerManage::scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    const int scrolled = cdroid::LinearLayoutManager::scrollHorizontallyBy(dx, recycler, state);
    adjustHorizontalChildView();
    return scrolled;
}

/// @brief 
/// @param dy 
/// @param recycler 
/// @param state 
/// @return 
int RVNumberPicker::RVNumberPickerManage::scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    const int scrolled = cdroid::LinearLayoutManager::scrollVerticallyBy(dy, recycler, state);
    adjustVerticalChildView();
    return scrolled;
}

/// @brief 横向调节
void RVNumberPicker::RVNumberPickerManage::adjustHorizontalChildView() {
    float boxCenterX = getWidth() / 2.0f; // 容器中心线
    bool selectlayoutIsEmpty = mFriend->mSelectLayout.empty();
    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);

        float decoratedLeft = getDecoratedLeft(child);             // 左边界
        float decoratedRight = getDecoratedRight(child);           // 右边界

        float centerX = (decoratedLeft + decoratedRight) / 2.0f;   // ITEM中心线
        float offsetX = centerX - boxCenterX;                      // X轴差值
        float position = offsetX / getWidth();                     // 所在位置
        float absX = std::abs(position);

        // 判断滑动中间项
        bool isCenterView = decoratedLeft < boxCenterX && decoratedRight > boxCenterX;
        
        if(!isCenterView && !selectlayoutIsEmpty){
            ((ViewGroup*)child)->getChildAt(0)->setVisibility(View::VISIBLE);
            ((ViewGroup*)child)->getChildAt(1)->setVisibility(View::INVISIBLE);
        }else if(isCenterView && !selectlayoutIsEmpty) {
            ((ViewGroup*)child)->getChildAt(0)->setVisibility(View::INVISIBLE);
            ((ViewGroup*)child)->getChildAt(1)->setVisibility(View::VISIBLE);
        }
        
        int centerPosition = LinearLayoutManager::getPosition(child);
        if (isCenterView && mCenterPositionCache != centerPosition) {
            if (mFriend->mOnCenterViewChangeListener)mFriend->onCenterViewChanged(mCenterPositionCache, centerPosition);
            mCenterPositionCache = centerPosition;
        }

        // 文本类型则进行文字缩放
        TextView* text;
        if(selectlayoutIsEmpty) text = dynamic_cast<TextView*>(child);
        else                    text = dynamic_cast<TextView*>(((ViewGroup*)child)->getChildAt(0));
        if (text && child->getTag() == (void*)PICKER_TYPE_TEXT ) {
            if (isCenterView && selectlayoutIsEmpty) {
                text->setTextSize(mFriend->isActivated() ? mFriend->mActiveSize : mFriend->mCenterSize);
                text->setTextColor(mFriend->isActivated() ? mFriend->mActiveColor : mFriend->mCenterColor);
            } else if(!isCenterView){
                text->setTextSize(calculateTextSize(absX));
                text->setTextColor(calculateColorValue(absX));
            }
            text->setSelected(isCenterView?true:false);
        }

        // 计算位置偏移
        if (mFriend->mConvertList.size()) calculateConvertValue(child, position);
    }
}

/// @brief 垂直调节
// 增加 bool 判断，是否来自 用户的滑动操作
void RVNumberPicker::RVNumberPickerManage::adjustVerticalChildView() {
    float boxCenterY = getHeight() / 2.0f; // 容器中心线
    bool selectlayoutIsEmpty = mFriend->mSelectLayout.empty();

    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);

        float decoratedTop = getDecoratedTop(child);             // 上边界
        float decoratedBottom = getDecoratedBottom(child);       // 下边界

        float centerY = (decoratedTop + decoratedBottom) / 2.0f; // ITEM中心线
        float offsetY = centerY - boxCenterY;                    // Y轴差值
        float position = offsetY / getHeight();                  // 所在位置
        float absY = std::abs(position);

        LOGV("child: %p, position: %f, centerY: %f, boxCenterY: %f, decoratedTop: %f, decoratedBottom: %f",
            child, position, centerY, boxCenterY, decoratedTop, decoratedBottom);


        // 判断滑动中间项
        bool isCenterView = decoratedTop < boxCenterY && decoratedBottom > boxCenterY;
        
        if(!isCenterView && !selectlayoutIsEmpty){
            ((ViewGroup*)child)->getChildAt(0)->setVisibility(View::VISIBLE);
            ((ViewGroup*)child)->getChildAt(1)->setVisibility(View::INVISIBLE);
        }else if(isCenterView && !selectlayoutIsEmpty) {
            ((ViewGroup*)child)->getChildAt(0)->setVisibility(View::INVISIBLE);
            ((ViewGroup*)child)->getChildAt(1)->setVisibility(View::VISIBLE);
        }

        int centerPosition = LinearLayoutManager::getPosition(child);
        if (isCenterView && mCenterPositionCache != centerPosition) {
            if (mFriend->mOnCenterViewChangeListener)mFriend->onCenterViewChanged(mCenterPositionCache, centerPosition);
            mCenterPositionCache = centerPosition;
        }

        // 文本类型则进行文字缩放
        TextView* text;
        if(selectlayoutIsEmpty) text = dynamic_cast<TextView*>(child);
        else                    text = dynamic_cast<TextView*>(((ViewGroup*)child)->getChildAt(0));
        if (text && child->getTag() == (void*)PICKER_TYPE_TEXT) {
            if (isCenterView && selectlayoutIsEmpty) {
                text->setTextSize(mFriend->isActivated() ? mFriend->mActiveSize : mFriend->mCenterSize);
                text->setTextColor(mFriend->isActivated() ? mFriend->mActiveColor : mFriend->mCenterColor);
            } else if(!isCenterView){
                text->setTextSize(calculateTextSize(absY));
                text->setTextColor(calculateColorValue(absY));
            }
            text->setSelected(isCenterView?true:false);
        }

        // 计算位置偏移
        if (mFriend->mConvertList.size()) calculateConvertValue(child, position);
    }
}

/// @brief 计算颜色过渡
/// @param abs 
/// @return 
int RVNumberPicker::RVNumberPickerManage::calculateColorValue(float abs) {
    if (mFriend->mTextColor == mFriend->mTextColor2)return mFriend->mTextColor;
    if (abs > .490f)return mFriend->mTextColor2;
    else if (abs < .01f)return mFriend->mTextColor;

    int proportion = (int)(abs * 200);

    // 分解起始颜色值
    unsigned int startAlpha = (mFriend->mTextColor >> 24) & 0xFF;
    unsigned int startRed = (mFriend->mTextColor >> 16) & 0xFF;
    unsigned int startGreen = (mFriend->mTextColor >> 8) & 0xFF;
    unsigned int startBlue = mFriend->mTextColor & 0xFF;

    // 分解结束颜色值
    unsigned int endAlpha = (mFriend->mTextColor2 >> 24) & 0xFF;
    unsigned int endRed = (mFriend->mTextColor2 >> 16) & 0xFF;
    unsigned int endGreen = (mFriend->mTextColor2 >> 8) & 0xFF;
    unsigned int endBlue = mFriend->mTextColor2 & 0xFF;

    // 计算过渡颜色值的红色分量
    unsigned int transitionAlpha = startAlpha - ((startAlpha - endAlpha) * proportion / 100);
    unsigned int transitionRed = startRed - ((startRed - endRed) * proportion / 100);
    unsigned int transitionGreen = startGreen - ((startGreen - endGreen) * proportion / 100);
    unsigned int transitionBlue = startBlue - ((startBlue - endBlue) * proportion / 100);

    // 组合过渡颜色值
    int transitionColor = (transitionAlpha << 24) | (transitionRed << 16) | (transitionGreen << 8) | transitionBlue;
    return transitionColor;
}

/// @brief 计算文本缩放
/// @param abs 
/// @return 
int RVNumberPicker::RVNumberPickerManage::calculateTextSize(float abs) {
    if (mFriend->mTextSize == mFriend->mTextSize2)return mFriend->mTextSize;
    if (abs > .499f)mFriend->mTextSize2;
    else if (abs < .01f)return mFriend->mTextSize;

    int proportion = (int)(abs * 200);
    int transitionTextSize = mFriend->mTextSize - ((mFriend->mTextSize - mFriend->mTextSize2) * proportion / 100);
    return transitionTextSize;
}

/// @brief 计算位置变换
/// @param child 
/// @param position 
void RVNumberPicker::RVNumberPickerManage::calculateConvertValue(View* child, const float& position) {
    const float abs = std::abs(position);
    float indentX = 0.f, indentY = 0.f;

    // 判断变化参数
    int index = 0;
    for (int count = mFriend->mConvertList.size(); index < count; index++) {
        if (index == count - 1 || abs < mFriend->mConvertList[index + 1].position)
            break;
    }

    // 计算变化值
    if (index != mFriend->mConvertList.size()) {
        const RVNumberPicker::ConvertStruct& transform = mFriend->mConvertList.at(index);
        if (index < mFriend->mConvertList.size() - 1) {
            const RVNumberPicker::ConvertStruct& transform_next = mFriend->mConvertList.at(index + 1);
            // 计算比例
            while (transform.position == transform_next.position)
                LOGE("position is same");
            float proportion = (abs - transform.position) / (transform.position - transform_next.position);
            indentX = proportion * (transform.transX - transform_next.transX) + transform.transX;
            indentY = proportion * (transform.transY - transform_next.transY) + transform.transY;

            // 根据位移类型修改位移参数
            indentX = transform.transXType == TRANSFER_ABS ? indentX
                : indentX * (position > 0 ? 1 : -1) * transform.transXType;
            indentY = transform.transYType == TRANSFER_ABS ? indentY
                : indentY * (position > 0 ? 1 : -1) * transform.transYType;
        } else {
            indentX = transform.transXType == TRANSFER_ABS ? transform.transX
                : transform.transX * (position > 0 ? 1 : -1) * transform.transXType;
            indentY = transform.transYType == TRANSFER_ABS ? transform.transY
                : transform.transY * (position > 0 ? 1 : -1) * transform.transYType;
        }
    }
    child->setTranslationX(indentX);
    child->setTranslationY(indentY);
}



/*****************************************SnapHelper***********************************************/

RVNumberPicker::RVNumberPickerSnapHelper::RVNumberPickerSnapHelper() {
    mVerticalHelper = nullptr;
    mHorizontalHelper = nullptr;
}

RVNumberPicker::RVNumberPickerSnapHelper::~RVNumberPickerSnapHelper() {
    delete mVerticalHelper;
    delete mHorizontalHelper;
}

void RVNumberPicker::RVNumberPickerSnapHelper::calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int out[2]) {
    if (layoutManager.canScrollHorizontally()) {
        out[0] = distanceToCenter(layoutManager, targetView, getHorizontalHelper(layoutManager));
    } else {
        out[0] = 0;
    }

    if (layoutManager.canScrollVertically()) {
        out[1] = distanceToCenter(layoutManager, targetView, getVerticalHelper(layoutManager));
    } else {
        out[1] = 0;
    }
}

int RVNumberPicker::RVNumberPickerSnapHelper::findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY) {
    return RecyclerView::NO_POSITION;
}

View* RVNumberPicker::RVNumberPickerSnapHelper::findSnapView(RecyclerView::LayoutManager& layoutManager) {
    if (layoutManager.canScrollVertically()) {
        return findCenterView(layoutManager, getVerticalHelper(layoutManager));
    } else if (layoutManager.canScrollHorizontally()) {
        return findCenterView(layoutManager, getHorizontalHelper(layoutManager));
    }
    return nullptr;
}

int RVNumberPicker::RVNumberPickerSnapHelper::distanceToCenter(RecyclerView::LayoutManager& layoutManager, View& targetView, OrientationHelper& helper) {
    const int childCenter = helper.getDecoratedStart(&targetView)
        + (helper.getDecoratedMeasurement(&targetView) / 2);
    int containerCenter;
    if (layoutManager.getClipToPadding()) {
        containerCenter = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        containerCenter = helper.getEnd() / 2;
    }
    return childCenter - containerCenter;
}

int RVNumberPicker::RVNumberPickerSnapHelper::estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper, int velocityX, int velocityY) {
    int distances[2];
    calculateScrollDistance(velocityX, velocityY, distances);
    float distancePerChild = computeDistancePerChild(layoutManager, helper);
    if (distancePerChild <= 0) {
        return 0;
    }
    const int distance =
        std::abs(distances[0]) > std::abs(distances[1]) ? distances[0] : distances[1];
    return (int)std::round(distance / distancePerChild);
}

View* RVNumberPicker::RVNumberPickerSnapHelper::findCenterView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper) {
    int childCount = layoutManager.getChildCount();
    if (childCount == 0) {
        return nullptr;
    }

    View* closestChild = nullptr;
    int center;
    if (layoutManager.getClipToPadding()) {
        center = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        center = helper.getEnd() / 2;
    }
    int absClosest = INT_MAX;//Integer.MAX_VALUE;

    for (int i = 0; i < childCount; i++) {
        View* child = layoutManager.getChildAt(i);
        int childCenter = helper.getDecoratedStart(child)
            + (helper.getDecoratedMeasurement(child) / 2);
        int absDistance = std::abs(childCenter - center);

        /** if child center is closer than previous closest, set it as closest  **/
        if (absDistance < absClosest) {
            absClosest = absDistance;
            closestChild = child;
        }
    }
    return closestChild;
}

float RVNumberPicker::RVNumberPickerSnapHelper::computeDistancePerChild(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper) {
    View* minPosView = nullptr;
    View* maxPosView = nullptr;
    int minPos = INT_MAX;//Integer.MAX_VALUE;
    int maxPos = INT_MIN;//Integer.MIN_VALUE;
    int childCount = layoutManager.getChildCount();
    if (childCount == 0) {
        return INVALID_DISTANCE;
    }

    for (int i = 0; i < childCount; i++) {
        View* child = layoutManager.getChildAt(i);
        const int pos = layoutManager.getPosition(child);
        if (pos == RecyclerView::NO_POSITION) {
            continue;
        }
        if (pos < minPos) {
            minPos = pos;
            minPosView = child;
        }
        if (pos > maxPos) {
            maxPos = pos;
            maxPosView = child;
        }
    }
    if (minPosView == nullptr || maxPosView == nullptr) {
        return INVALID_DISTANCE;
    }
    int start = std::min(helper.getDecoratedStart(minPosView),
        helper.getDecoratedStart(maxPosView));
    int end = std::max(helper.getDecoratedEnd(minPosView),
        helper.getDecoratedEnd(maxPosView));
    int distance = end - start;
    if (distance == 0) {
        return INVALID_DISTANCE;
    }
    return 1.f * distance / ((maxPos - minPos) + 1);
}

OrientationHelper& RVNumberPicker::RVNumberPickerSnapHelper::getVerticalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mVerticalHelper == nullptr || mVerticalHelper->getLayoutManager() != &layoutManager) {
        mVerticalHelper = OrientationHelper::createVerticalHelper(&layoutManager);
    }
    return *mVerticalHelper;
}

OrientationHelper& RVNumberPicker::RVNumberPickerSnapHelper::getHorizontalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mHorizontalHelper == nullptr || mHorizontalHelper->getLayoutManager() != &layoutManager) {
        mHorizontalHelper = OrientationHelper::createHorizontalHelper(&layoutManager);
    }
    return *mHorizontalHelper;
}



/*****************************************NumberPicker***********************************************/

/// @brief 
/// @param w 
/// @param h 
RVNumberPicker::RVNumberPicker(int w, int h) :RecyclerView(w, h) {
    init();
}

/// @brief 
/// @param context 
/// @param attr 
RVNumberPicker::RVNumberPicker(Context* context, const AttributeSet& attr)
    :RecyclerView(context, attr), mContext(context) {
    mOrientation = attr.getInt("orientation", std::unordered_map<std::string, int>{
        { "horizontal", (int)HORIZONTAL},
        { "vertical",(int)VERTICAL }
    }, DEFAULT_ORIENTATION);
    mReverse = attr.getBoolean("reverseLayout", false);
    mDisplayCount = attr.getInt("wheelItemCount", 3);
    mSmoothDuration = attr.getInt("smoothDuration", 300);
    mMinNum = attr.getInt("min", 0);
    mMaxNum = attr.getInt("max", 5);
    mRealCount = mMaxNum - mMinNum + 1;

    mGravity = attr.getGravity("gravity", Gravity::CENTER);
    mTextSize = attr.getDimensionPixelSize("textSize", 20);
    mTextColor = attr.getColor("textColor");
    mTextSize2 = attr.getDimensionPixelSize("textSize2", mTextSize);
    mTextColor2 = attr.getColor("textColor2", mTextColor);
    mCenterSize = attr.getDimensionPixelSize("centerSize", mTextSize);
    mCenterColor = attr.getColor("centerColor", mTextColor);
    mActiveSize = attr.getDimensionPixelSize("activeSize", mCenterSize);
    mActiveColor = attr.getColor("activeColor", mCenterColor);
    mItemBackground = attr.getString("itemBackground", "#00FFFFFF");

    mSelectLayout = attr.getString("internalLayout", "");

    mTextStyle = attr.getInt("textStyle", std::unordered_map<std::string, int>{
        { "normal", (int)Typeface::NORMAL },
        { "bold"  , (int)Typeface::BOLD },
        { "italic", (int)Typeface::ITALIC }
    }, Typeface::NORMAL);
    mFontFamily = attr.getString("fontFamily", "");
    mFontTypeface = Typeface::create(mFontFamily, mTextStyle);

    mXMLWidth = attr.getLayoutDimension("layout_width", LayoutParams::WRAP_CONTENT);
    mXMLHeight = attr.getLayoutDimension("layout_height", LayoutParams::WRAP_CONTENT);
    init();
}

/// @brief 析构
RVNumberPicker::~RVNumberPicker() {
    delete mAdapter;
    delete mSnapHelper;
}

/// @brief 初始化
void RVNumberPicker::init() {
    mPosition = 0;
    mNumberFormatter = nullptr;
    mOnItemClickListener = nullptr;
    mOnItemLongClickListener = nullptr;
    mOnValueChangeListener = nullptr;
    mOnCenterViewChangeListener = nullptr;

    mAdapter = new RVNumberPickerAdapter(this);
    mLayoutManage = new RVNumberPickerManage(mContext, this, mOrientation, mReverse);
    mSnapHelper = new RVNumberPickerSnapHelper();

    setAdapter(mAdapter);
    setLayoutManager(mLayoutManage);
    mSnapHelper->attachToRecyclerView(this);
    getRecycledViewPool().setMaxRecycledViews(0, 5 * mDisplayCount);
}

/// @brief 获取当前值
/// @return 
int RVNumberPicker::getValue() {
    return mPosition + mMinNum;
}

/// @brief 获取最大值
/// @return 
int  RVNumberPicker::getMaxValue(){
    return mMaxNum;
}

/// @brief 获取最小值
/// @return 
int  RVNumberPicker::getMinValue(){
    return mMinNum;
}

/// @brief 设置当前值
/// @param value 设置值
/// @param smooth 是否平滑切换
/// @param callBack 是否触发回调（仅在非smooth模式下生效，smooth模式下自动会触发valuechange回调）
void RVNumberPicker::setValue(int value, bool smooth, bool callBack) {
    mPosition = (value<mMinNum ? mMinNum : value>mMaxNum ? mMaxNum : value) - mMinNum;
    if (smooth) {
        smoothScrollToPosition(mPosition);
    } else {
        scrollToPosition(mPosition);
        if (callBack) onValueChanged(mPosition);
    }
}

/// @brief 跳转到下一项
/// @param smooth 是否平滑切换
/// @param cycle 是否循环
void RVNumberPicker::nextValue(bool smooth, bool cycle) {
    if (mPosition >= mRealCount - 1) {
        if (cycle && mPosition != 0) {
            scrollToPosition(0);
            onValueChanged(0);
        } else {
            return;
        }
    } else {
        if (smooth) {
            smoothScrollToPosition(mPosition + 1);
        } else {
            scrollToPosition(mPosition + 1);
            onValueChanged(mPosition + 1);
        }
    }
}

/// @brief 跳转到上一项
/// @param smooth 是否平滑切换
/// @param cycle 是否循环
void RVNumberPicker::prevValue(bool smooth, bool cycle) {
    if (mPosition <= 0) {
        if (cycle && mPosition != mRealCount - 1) {
            scrollToPosition(mRealCount - 1);
            onValueChanged(mRealCount - 1);
        } else {
            return;
        }
    } else {
        if (smooth) {
            smoothScrollToPosition(mPosition - 1);
        } else {
            scrollToPosition(mPosition - 1);
            onValueChanged(mPosition - 1);
        }
    }
}

/// @brief 设置最大值
/// @param maxvalue 
void RVNumberPicker::setMaxValue(int maxvalue) {
    mMaxNum = maxvalue;
    mRealCount = mMaxNum - mMinNum + 1;
}

/// @brief 设置最小值
/// @param minvalue 
void RVNumberPicker::setMinValue(int minvalue) {
    mMinNum = minvalue;
    mRealCount = mMaxNum - mMinNum + 1;
}

/// @brief 通知更新
/// @param isItemChange （ture），会优化notify时所需时间，但是否全部场景都适用，还待测试
void RVNumberPicker::notifyUpdate(bool isItemChange) {
    if(isItemChange) mAdapter->notifyItemRangeChanged(0,mMaxNum-mMinNum);
    else             mAdapter->notifyDataSetChanged();
}

/// @brief 通知更新
void RVNumberPicker::notifyUpdatePostion(int Pos) {
    mAdapter->notifyItemChanged(Pos);
}

/// @brief 更新整个picker数据
/// @param min 最小值
/// @param max 最大值
void RVNumberPicker::updateStruct(int min, int max) {
    updateStruct(min, max, getValue());
}

/// @brief 更新整个picker数据
/// @param min 最小值
/// @param max 最大值
/// @param value 当前值
void RVNumberPicker::updateStruct(int min, int max, int value) {
    setMinValue(min);
    setMaxValue(max);
    notifyUpdate();
    setValue(value);
}

/// @brief 设置文本格式化
/// @param l 
void RVNumberPicker::setFormatter(TextFormatter l) {
    mNumberFormatter = l;
    notifyUpdate();
}

/// @brief 设置 选中项 文本格式化
/// @param l 
void RVNumberPicker::setSelectFormatter(TextFormatter l) {
    mSelectNumberFormatter = l;
    notifyUpdate();
}

/// @brief 设置smooth时 duration
/// @param duration
void RVNumberPicker::setSmoothScrollerDuration(int duration){
    mSmoothDuration = duration;
}

/// @brief 设置图片资源列表，当数量为非0时，启用图像模式
/// @param list 图片资源
/// @param update 是否更新最小值/最大值
/// @param newValue update更新后的值，-1为当前值
void RVNumberPicker::setImageList(std::vector<std::string> list, bool update, int newValue) {
    mImageList = list;
    if (update)
        updateStruct(0, list.size() - 1, newValue >= 0 ? newValue : getValue());
    notifyUpdate();
}

/// @brief 设置位置偏移列表
/// @param list 
void RVNumberPicker::setConvertList(std::vector<ConvertStruct> list) {
    std::sort(list.begin(), list.end(),
        [ ](ConvertStruct& a, ConvertStruct& b) {return a.position < b.position;}
    );
    mConvertList = list;
}

/// @brief 设置点击回调
/// @param l   
void RVNumberPicker::setOnItemClickListener(OnItemClickListener l) {
    mOnItemClickListener = l;
}

/// @brief 设置长按回调
/// @param l 
void RVNumberPicker::setOnItemLongClickListener(OnItemLongClickListener l) {
    mOnItemLongClickListener = l;
}

/// @brief 设置选中项回调
/// @param l 
void RVNumberPicker::setOnValueChangedListener(OnValueChangeListener l) {
    mOnValueChangeListener = l;
}

/// @brief 设置滑动过程中中间项改变回调
/// @param l 
void RVNumberPicker::setOnCenterViewChangeListener(OnCenterViewChangeListener l) {
    mOnCenterViewChangeListener = l;
}

/// @brief 点击回调
/// @param v 
/// @param position 
void RVNumberPicker::onItemClick(View& v, int position) {
    if (mOnItemClickListener)mOnItemClickListener(*this, v, position);
}

/// @brief 长按回调
/// @param v 
/// @param position 
void RVNumberPicker::onItemLongClick(View& v, int position) {
    if (mOnItemLongClickListener)mOnItemLongClickListener(*this, v, position);
}

/// @brief 选中项改变回调
/// @param n 新值
void RVNumberPicker::onValueChanged(int n) {
    int o = mPosition;
    mPosition = n;
    if (mOnValueChangeListener)mOnValueChangeListener(*this, o + mMinNum, n + mMinNum);
}

/// @brief 滑动过程中间项变化回调
/// @param o 旧值
/// @param n 新值
void RVNumberPicker::onCenterViewChanged(int o, int n) {
    if (mOnCenterViewChangeListener)mOnCenterViewChangeListener(*this, o + mMinNum, n + mMinNum);
}
