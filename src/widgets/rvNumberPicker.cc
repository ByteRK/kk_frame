/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2026-08-04 18:44:29
 * @FilePath: /kk_frame/src/widgets/rvNumberPicker.cc
 * @Description: 使用RecycleView实现数字选择器
 *
 * @BugList: 1、暂时不要使用SmoothscrolltoPosition
 *           2、layout_width以及layout_height必须指定数值
 *           3、textColor全透颜色请使用#01000000,暂不支持全0透明度
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
**/

#include "rvNumberPicker.h"
#include <view/layoutinflater.h>
#include <view/viewoverlay.h>
#include <widget/relativelayout.h>
#include <widgetEx/recyclerview/orientationhelper.h>

DECLARE_WIDGET(RVNumberPicker)

// ============ 渐变 / 滚动 内部常量 ============
static constexpr float  GRADIENT_MAX = 0.49f;          // 变换最大阈值（视同两边）
static constexpr float  GRADIENT_MIN = 0.01f;          // 变换最小阈值（视同中心）
static constexpr float  DECEL_COEFFICIENT = 0.3356f;   // 滚动减速度系数
static constexpr int    RECYCLED_VIEW_MULT = 5;        // 回收池容量倍数

// overlay 布局坐标（需与 XML 中 overlay 布局尺寸一致）
static constexpr int    OVERLAY_LAYOUT_L = 153, OVERLAY_LAYOUT_T = 27, OVERLAY_LAYOUT_R = 10, OVERLAY_LAYOUT_B = 10;
static constexpr int    SEL_OVERLAY_L = 165, SEL_OVERLAY_T = 24, SEL_OVERLAY_R = 34, SEL_OVERLAY_B = 34;

/*****************************************适配器***********************************************/

RVNumberPicker::PickerAdapter::PickerAdapter(RVNumberPicker* pickerView) :mFriend(pickerView) { }

RecyclerView::ViewHolder* RVNumberPicker::PickerAdapter::onCreateViewHolder(ViewGroup* parent, int viewType) {
    View* view = (viewType == PICKER_TYPE_IMAGE) ? createImageItem(parent)
        : mFriend->mSelectLayout.empty() ? createSimpleTextItem(parent)
        : createSelectTextItem(parent);
    return new ViewHolder(view);
}

void RVNumberPicker::PickerAdapter::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    int realPosition = position + mFriend->mMinNum;
    View* view;

    if (getItemViewType(position) == PICKER_TYPE_IMAGE) {
        ImageView* imageView = static_cast<ImageView*>(holder.itemView);
        bindImageItem(imageView, position, realPosition);
        view = imageView;
    } else {
        if (mFriend->mSelectLayout.empty()) {
            TextView* textView = static_cast<TextView*>(holder.itemView);
            bindSimpleTextItem(textView, realPosition);
            view = textView;
        } else {
            ViewGroup* layout = static_cast<ViewGroup*>(holder.itemView);
            bindSelectTextItem(layout, position, realPosition);
            view = layout;
        }
    }

    if (mFriend->mOnItemClickListener) {
        view->setOnClickListener([this, position](View& v) { mFriend->onItemClick(v, position); });
        if (!mFriend->isSoundEffectsEnabled()) view->setSoundEffectsEnabled(false);
    } else {
        view->setClickable(false);
    }
}

int RVNumberPicker::PickerAdapter::getItemCount() {
    return mFriend->mRealCount;
}

int RVNumberPicker::PickerAdapter::getItemViewType(int position) {
    return (!mFriend->mImageList.empty() && position < (int)mFriend->mImageList.size())
        ? PICKER_TYPE_IMAGE : PICKER_TYPE_TEXT;
}

View* RVNumberPicker::PickerAdapter::createImageItem(ViewGroup* parent) {
    ImageView* imageView;
    if (mFriend->mOrientation == HORIZONTAL) {
        imageView = new ImageView(LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
        imageView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
        LOGV("[IMAGE]view w:%d", mFriend->mXMLWidth / mFriend->mDisplayCount);
    } else {
        imageView = new ImageView(LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
        imageView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
        LOGV("[IMAGE]view h:%d", mFriend->mXMLHeight / mFriend->mDisplayCount);
    }
    imageView->setScaleType(ScaleType::CENTER_INSIDE);
    imageView->setTag((void*)PICKER_TYPE_IMAGE);
    return imageView;
}

View* RVNumberPicker::PickerAdapter::createSimpleTextItem(ViewGroup* parent) {
    TextView* textView;
    if (mFriend->mOrientation == HORIZONTAL) {
        textView = new TextView("", LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
        textView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
        LOGV("[TEXT]view w:%d", mFriend->mXMLWidth / mFriend->mDisplayCount);
    } else {
        textView = new TextView("", LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
        textView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
        LOGV("[TEXT]view h:%d", mFriend->mXMLHeight / mFriend->mDisplayCount);
    }
    textView->setBreakStrategy(Layout::BREAK_STRATEGY_SIMPLE);
    textView->setGravity(mFriend->mGravity);
    textView->setTypeface(mFriend->mFontTypeface);
    textView->setTag((void*)PICKER_TYPE_TEXT);
    if (!mFriend->mOverlayLayout.empty()) {
        View *overlayView = LayoutInflater::from(parent->getContext())->inflate(mFriend->mOverlayLayout, nullptr);
        overlayView->layout(OVERLAY_LAYOUT_L, OVERLAY_LAYOUT_T, OVERLAY_LAYOUT_R, OVERLAY_LAYOUT_B);
        textView->getOverlay()->getOverlayView()->addView(overlayView);
    }
    return textView;
}

View* RVNumberPicker::PickerAdapter::createSelectTextItem(ViewGroup* parent) {
    TextView* textView = static_cast<TextView*>(createSimpleTextItem(parent));

    TextView* selectView = static_cast<TextView*>(LayoutInflater::from(parent->getContext())->inflate(mFriend->mSelectLayout, nullptr));
    if (!selectView) {
        mFriend->mSelectLayout.clear();
        return textView;
    }

    RelativeLayout* layout;
    if (mFriend->mOrientation == HORIZONTAL) {
        layout = new RelativeLayout(LayoutParams::WRAP_CONTENT, LayoutParams::MATCH_PARENT);
        layout->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
        selectView->setLayoutParams(new LayoutParams(mFriend->mXMLWidth / mFriend->mDisplayCount, LayoutParams::MATCH_PARENT));
    } else {
        layout = new RelativeLayout(LayoutParams::MATCH_PARENT, LayoutParams::WRAP_CONTENT);
        layout->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
        selectView->setLayoutParams(new LayoutParams(LayoutParams::MATCH_PARENT, mFriend->mXMLHeight / mFriend->mDisplayCount));
    }
    selectView->setGravity(mFriend->mGravity);
    selectView->setVisibility(View::INVISIBLE);
    if (!mFriend->mSelectOverlayLayout.empty()) {
        View* selOverlay = LayoutInflater::from(parent->getContext())->inflate(mFriend->mSelectOverlayLayout, nullptr);
        selOverlay->layout(SEL_OVERLAY_L, SEL_OVERLAY_T, SEL_OVERLAY_R, SEL_OVERLAY_B);
        selectView->getOverlay()->getOverlayView()->addView(selOverlay);
    }
    layout->addView(textView);
    layout->addView(selectView);
    layout->setBackgroundColor(0x00FFFFFF);
    return layout;
}

void RVNumberPicker::PickerAdapter::bindImageItem(ImageView* imageView, int position, int realPosition) {
    if (realPosition < (int)mFriend->mImageList.size())
        imageView->setImageResource(mFriend->mImageList.at(realPosition));
    else
        imageView->setImageResource("#000000");
    imageView->setBackgroundResource(mFriend->mItemBackground);
}

void RVNumberPicker::PickerAdapter::bindSimpleTextItem(TextView* textView, int realPosition) {
    if (mFriend->mNumberFormatter)
        textView->setText(mFriend->mNumberFormatter(realPosition));
    else
        textView->setText(std::to_string(realPosition));
    textView->setBackgroundResource(mFriend->mItemBackground);
    // 检查 overlay 可见性
    if (!mFriend->mOverlayLayout.empty()) {
        View* overlayView = textView->getOverlay()->getOverlayView()->getChildAt(0);
        bool visible = mFriend->mOverlayFormatter && mFriend->mOverlayFormatter(realPosition) != "0";
        overlayView->setVisibility(visible ? View::VISIBLE : View::GONE);
    }
}

void RVNumberPicker::PickerAdapter::bindSelectTextItem(ViewGroup* layout, int position, int realPosition) {
    TextView* textView = static_cast<TextView*>(layout->getChildAt(0));
    TextView* selectTv = static_cast<TextView*>(layout->getChildAt(1));

    bindSimpleTextItem(textView, realPosition);

    if (mFriend->mNumberFormatter) {
        if (mFriend->mSelectNumberFormatter)
            selectTv->setText(mFriend->mSelectNumberFormatter(realPosition));
        else
            selectTv->setText(mFriend->mNumberFormatter(realPosition));
    } else {
        selectTv->setText(std::to_string(realPosition));
    }

    if (!mFriend->mSelectOverlayLayout.empty()) {
        View* selOverlay = selectTv->getOverlay()->getOverlayView()->getChildAt(0);
        if (dynamic_cast<TextView*>(selOverlay)) {
            if (mFriend->mOverlayFormatter && mFriend->mOverlayFormatter(realPosition) != "0") {
                static_cast<TextView*>(selOverlay)->setText(mFriend->mOverlayFormatter(realPosition));
                selOverlay->setVisibility(View::VISIBLE);
            } else {
                selOverlay->setVisibility(View::GONE);
            }
        }
    }
}


/*****************************************滚动器***********************************************/

RVNumberPicker::PickerScroller::PickerScroller(Context* context) :
    LinearSmoothScroller(context), mDisplayMetrics(context->getDisplayMetrics()) { }

void RVNumberPicker::PickerScroller::setDuration(int duration) {
    mSmoothDuration = duration;
}

void RVNumberPicker::PickerScroller::onTargetFound(View* targetView, RecyclerView::State& state, Action& action) {
    const int dx = calculateDxToMakeVisible(targetView, getHorizontalSnapPreference());
    const int dy = calculateDyToMakeVisible(targetView, getVerticalSnapPreference());
    const int distance = (int)std::sqrt(dx * dx + dy * dy);
    const int time = calculateTimeForDeceleration(distance);
    if (time > 0) action.update(-dx, -dy, time, mDecelerateInterpolator);
}

int RVNumberPicker::PickerScroller::calculateTimeForDeceleration(int dx) {
    return std::ceil(std::abs(dx) * ((float)mSmoothDuration / mDisplayMetrics.densityDpi)) / DECEL_COEFFICIENT;
}


/*****************************************管理器***********************************************/

RVNumberPicker::PickerManager::PickerManager(Context* context, RVNumberPicker* pickerView, int orientation, bool reverseLayout)
    :LinearLayoutManager(context, orientation, reverseLayout), mFriend(pickerView) { }

void RVNumberPicker::PickerManager::onAttachedToWindow(RecyclerView& view) {
    cdroid::LinearLayoutManager::onAttachedToWindow(view);
    scrollToPosition(mFriend->mPosition);
}

void RVNumberPicker::PickerManager::onLayoutCompleted(State& state) {
    LinearLayoutManager::onLayoutCompleted(state);
    if (mOrientation == HORIZONTAL) adjustChildViewImpl<true>();
    else adjustChildViewImpl<false>();
}

void RVNumberPicker::PickerManager::onScrollStateChanged(int state) {
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

void RVNumberPicker::PickerManager::smoothScrollToPosition(RecyclerView& recyclerView, RecyclerView::State& state, int position) {
    PickerScroller* rvSmoothScroller = new PickerScroller(recyclerView.getContext());
    rvSmoothScroller->setDuration(mFriend->mSmoothDuration);
    rvSmoothScroller->setTargetPosition(position);
    startSmoothScroll(rvSmoothScroller);
}

void RVNumberPicker::PickerManager::onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec) {
    if (getItemCount() != 0 && mFriend->mDisplayCount != 0) {
        int itemViewWidth = mRecyclerView->getLayoutParams()->width / mFriend->mDisplayCount;
        int itemViewHeight = mRecyclerView->getLayoutParams()->height / mFriend->mDisplayCount;

        mRecyclerView->setClipToPadding(false);
        if (mOrientation == HORIZONTAL) {
            int paddingHorizontal = (mFriend->mDisplayCount - 1) / 2 * itemViewWidth;
            LOGV("mDisplayCount = %d  mRealCount = %d  itemViewWidth = %d  paddingHorizontal = %d", mFriend->mDisplayCount, mFriend->mRealCount, itemViewWidth, paddingHorizontal);
            mRecyclerView->setPadding(paddingHorizontal, 0, paddingHorizontal, 0);
            setMeasuredDimension(
                itemViewWidth * mFriend->mDisplayCount,
                LayoutManager::chooseSize(heightSpec, getPaddingTop() + getPaddingBottom(), getMinimumHeight())
            );
        } else if (mOrientation == VERTICAL) {
            int paddingVertical = (mFriend->mDisplayCount - 1) / 2 * itemViewHeight;
            LOGV("mDisplayCount = %d  mRealCount = %d  itemViewHeight = %d  paddingVertical = %d", mFriend->mDisplayCount, mFriend->mRealCount, itemViewHeight, paddingVertical);
            mRecyclerView->setPadding(0, paddingVertical, 0, paddingVertical);
            setMeasuredDimension(
                LayoutManager::chooseSize(widthSpec, getPaddingLeft() + getPaddingRight(), getMinimumWidth()),
                itemViewHeight * mFriend->mDisplayCount
            );
        }
    } else {
        cdroid::LinearLayoutManager::onMeasure(recycler, state, widthSpec, heightSpec);
    }
}

int RVNumberPicker::PickerManager::scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    const int scrolled = cdroid::LinearLayoutManager::scrollHorizontallyBy(dx, recycler, state);
    adjustChildViewImpl<true>();
    return scrolled;
}

int RVNumberPicker::PickerManager::scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state) {
    const int scrolled = cdroid::LinearLayoutManager::scrollVerticallyBy(dy, recycler, state);
    adjustChildViewImpl<false>();
    return scrolled;
}

// ============ 轴无关的 child 调节模板 ============

struct AxisHelperH {
    static float containerSize(RVNumberPicker::PickerManager* mgr) { return (float)mgr->getWidth(); }
    static float start(View* child, RVNumberPicker::PickerManager* mgr) { return (float)mgr->getDecoratedLeft(child); }
    static float end(View* child, RVNumberPicker::PickerManager* mgr) { return (float)mgr->getDecoratedRight(child); }
};
struct AxisHelperV {
    static float containerSize(RVNumberPicker::PickerManager* mgr) { return (float)mgr->getHeight(); }
    static float start(View* child, RVNumberPicker::PickerManager* mgr) { return (float)mgr->getDecoratedTop(child); }
    static float end(View* child, RVNumberPicker::PickerManager* mgr) { return (float)mgr->getDecoratedBottom(child); }
};

template<bool IsHorizontal>
void RVNumberPicker::PickerManager::adjustChildViewImpl() {
    using A = typename std::conditional<IsHorizontal, AxisHelperH, AxisHelperV>::type;

    float boxCenter = A::containerSize(this) / 2.0f;            // 容器中心点坐标
    bool  selectLayoutIsEmpty = mFriend->mSelectLayout.empty(); // 选中布局是否为空(无选中布局)

    for (int i = 0; i < getChildCount(); i++) {
        View* child = getChildAt(i);

        float decStart = A::start(child, this);
        float decEnd = A::end(child, this);
        float center = (decStart + decEnd) / 2.0f;
        float offset = center - boxCenter;
        float position = offset / A::containerSize(this);
        float absDist = std::abs(position);

        bool isCenterView = decStart < boxCenter && decEnd > boxCenter;

        // 选中布局的可见性切换
        if (!selectLayoutIsEmpty) {
            ViewGroup* vg = static_cast<ViewGroup*>(child);
            if (mFriend->mSelectVisibility == View::VISIBLE) {
                vg->getChildAt(0)->setVisibility(isCenterView ? View::GONE : View::VISIBLE);
                vg->getChildAt(1)->setVisibility(isCenterView ? View::VISIBLE : View::GONE);
            } else {
                vg->getChildAt(0)->setVisibility(View::VISIBLE);
                vg->getChildAt(1)->setVisibility(View::GONE);
            }
        }

        // 中间项变化回调
        if (isCenterView) {
            int centerPosition = LinearLayoutManager::getPosition(child);
            if (mCenterPositionCache != centerPosition && mFriend->mOnCenterViewChangeListener) {
                mFriend->onCenterViewChanged(mCenterPositionCache, centerPosition);
                mCenterPositionCache = centerPosition;
            }
        }

        // 文字缩放 / 颜色渐变 (仅 PICKER_TYPE_TEXT，跳过图片 item)
        if (child->getTag() == (void*)PICKER_TYPE_TEXT) {
            TextView* text = selectLayoutIsEmpty
                ? static_cast<TextView*>(child)
                : static_cast<TextView*>(static_cast<ViewGroup*>(child)->getChildAt(0));

            if (isCenterView) {
                if (!selectLayoutIsEmpty && mFriend->mSelectVisibility == View::VISIBLE) {
                    text->setTextSize(mFriend->isActivated() ? mFriend->mCenterTextTheme.activeSize : mFriend->mCenterTextTheme.size);
                    text->setTextColor(mFriend->isActivated() ? mFriend->mCenterTextTheme.activeColor : mFriend->mCenterTextTheme.color);
                }
            } else {
                text->setTextSize(calculateTextSize(absDist, mFriend->isActivated()));
                text->setTextColor(calculateColorValue(absDist, mFriend->isActivated()));
            }

            text->setSelected(isCenterView);
        }

        // 位置偏移变换
        if (!mFriend->mConvertList.empty()) calculateConvertValue(child, position);
    }
}

// 显式实例化（避免链接错误）
template void RVNumberPicker::PickerManager::adjustChildViewImpl<true>();
template void RVNumberPicker::PickerManager::adjustChildViewImpl<false>();

// ============ 渐变计算 ============

/// @brief ARGB 线性插值
static inline int lerpARGB(int c1, int c2, int t) {
    auto comp = [=](int shift) {
        int a = ((c1 >> shift) & 0xFF) - ((((c1 >> shift) & 0xFF) - ((c2 >> shift) & 0xFF)) * t / 100);
        return a << shift;
    };
    return comp(24) | comp(16) | comp(8) | comp(0);
}

int RVNumberPicker::PickerManager::calculateColorValue(float abs, bool activated) {
    int& textColor = activated ? mFriend->mTextTheme.activeColor : mFriend->mTextTheme.color;
    int& textColor2 = activated ? mFriend->mTextTheme2.activeColor : mFriend->mTextTheme2.color;

    if (textColor == textColor2)  return textColor;
    if (abs > GRADIENT_MAX)       return textColor2;
    if (abs < GRADIENT_MIN)       return textColor;

    int proportion = (int)(abs * 200); // 将 abs 映射到 0~100 的比例范围
    return lerpARGB(textColor, textColor2, proportion);
}

int RVNumberPicker::PickerManager::calculateTextSize(float abs, bool activated) {
    int& textSize = activated ? mFriend->mTextTheme.activeSize : mFriend->mTextTheme.size;
    int& textSize2 = activated ? mFriend->mTextTheme2.activeSize : mFriend->mTextTheme2.size;

    if (textSize == textSize2)  return textSize;
    if (abs > GRADIENT_MAX)     return textSize2;
    if (abs < GRADIENT_MIN)     return textSize;

    int proportion = (int)(abs * 200); // 将 abs 映射到 0~100 的比例范围
    return textSize - ((textSize - textSize2) * proportion / 100);
}

void RVNumberPicker::PickerManager::calculateConvertValue(View* child, const float& position) {
    const float abs = std::abs(position);
    float indentX = 0.f, indentY = 0.f;

    const auto& list = mFriend->mConvertList;
    if (list.empty()) { child->setTranslationX(0); child->setTranslationY(0); return; }

    // 二分查找：找到 abs 所在的区间
    auto it = std::lower_bound(list.begin(), list.end(), abs,
        [](const RVNumberPicker::ConvertStruct& v, float a) { return v.position < a; });
    int index = (it == list.begin()) ? 0 : (int)(it - list.begin()) - 1;

    const RVNumberPicker::ConvertStruct& transform = list[index];
    if (index < (int)list.size() - 1) {
        const RVNumberPicker::ConvertStruct& next = list[index + 1];
        if (transform.position == next.position) {
            LOGE("Duplicate position in ConvertList, skipping");
            child->setTranslationX(indentX);
            child->setTranslationY(indentY);
            return;
        }
        float proportion = (abs - transform.position) / (next.position - transform.position);
        indentX = proportion * (transform.transX - next.transX) + transform.transX;
        indentY = proportion * (transform.transY - next.transY) + transform.transY;

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
    child->setTranslationX(indentX);
    child->setTranslationY(indentY);
}


/*****************************************SnapHelper***********************************************/

RVNumberPicker::PickerSnapHelper::~PickerSnapHelper() {
    if (mVerticalHelper) {
        delete mVerticalHelper;
        mVerticalHelper = nullptr;
    }
    if (mHorizontalHelper) {
        delete mHorizontalHelper;
        mHorizontalHelper = nullptr;
    }
}

void RVNumberPicker::PickerSnapHelper::calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int out[2]) {
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

int RVNumberPicker::PickerSnapHelper::findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY) {
    int centerPosition = layoutManager.getPosition(findSnapView(layoutManager));
    if (centerPosition == RecyclerView::NO_POSITION) return RecyclerView::NO_POSITION;

    int posDiff;
    if (layoutManager.canScrollVertically()) {
        posDiff = estimateNextPositionDiffForFling(layoutManager, getVerticalHelper(layoutManager), velocityX, velocityY);
    } else if (layoutManager.canScrollHorizontally()) {
        posDiff = estimateNextPositionDiffForFling(layoutManager, getHorizontalHelper(layoutManager), velocityX, velocityY);
    } else {
        return RecyclerView::NO_POSITION;
    }
    int target = centerPosition + posDiff;
    // 限制在有效范围内
    int itemCount = layoutManager.getItemCount();
    if (itemCount > 0) target = std::max(0, std::min(target, itemCount - 1));
    return target;
}

View* RVNumberPicker::PickerSnapHelper::findSnapView(RecyclerView::LayoutManager& layoutManager) {
    if (layoutManager.canScrollVertically()) {
        return findCenterView(layoutManager, getVerticalHelper(layoutManager));
    } else if (layoutManager.canScrollHorizontally()) {
        return findCenterView(layoutManager, getHorizontalHelper(layoutManager));
    }
    return nullptr;
}

int RVNumberPicker::PickerSnapHelper::distanceToCenter(RecyclerView::LayoutManager& layoutManager, View& targetView, OrientationHelper& helper) {
    const int childCenter = helper.getDecoratedStart(&targetView) + (helper.getDecoratedMeasurement(&targetView) / 2);
    int containerCenter;
    if (layoutManager.getClipToPadding()) {
        containerCenter = helper.getStartAfterPadding() + helper.getTotalSpace() / 2;
    } else {
        containerCenter = helper.getEnd() / 2;
    }
    return childCenter - containerCenter;
}

int RVNumberPicker::PickerSnapHelper::estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper, int velocityX, int velocityY) {
    int distances[2];
    calculateScrollDistance(velocityX, velocityY, distances);
    float distancePerChild = computeDistancePerChild(layoutManager, helper);
    if (distancePerChild <= 0) return 0;
    const int distance = std::abs(distances[0]) > std::abs(distances[1]) ? distances[0] : distances[1];
    return (int)std::round(distance / distancePerChild);
}

View* RVNumberPicker::PickerSnapHelper::findCenterView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper) {
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
    int absClosest = INT_MAX;

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

float RVNumberPicker::PickerSnapHelper::computeDistancePerChild(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper) {
    View* minPosView = nullptr;
    View* maxPosView = nullptr;
    int minPos = INT_MAX;
    int maxPos = INT_MIN;
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

OrientationHelper& RVNumberPicker::PickerSnapHelper::getVerticalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mVerticalHelper == nullptr || mVerticalHelper->getLayoutManager() != &layoutManager) {
        mVerticalHelper = OrientationHelper::createVerticalHelper(&layoutManager);
    }
    return *mVerticalHelper;
}

OrientationHelper& RVNumberPicker::PickerSnapHelper::getHorizontalHelper(RecyclerView::LayoutManager& layoutManager) {
    if (mHorizontalHelper == nullptr || mHorizontalHelper->getLayoutManager() != &layoutManager) {
        mHorizontalHelper = OrientationHelper::createHorizontalHelper(&layoutManager);
    }
    return *mHorizontalHelper;
}


/*****************************************NumberPicker***********************************************/

RVNumberPicker::RVNumberPicker(int w, int h) :RecyclerView(w, h) {
    init();
}

RVNumberPicker::RVNumberPicker(Context* context, const AttributeSet& attr) :RecyclerView(context, attr) {
    mReverse = attr.getBoolean("reverseLayout", mReverse);
    mOrientation = attr.getInt("orientation", std::unordered_map<std::string, int>{
        { "horizontal", LinearLayout::HORIZONTAL },
        { "vertical", LinearLayout::VERTICAL }
    }, mOrientation);
    mDisplayCount = attr.getInt("wheelItemCount", mDisplayCount);
    mSmoothDuration = attr.getInt("smoothDuration", mSmoothDuration);
    mMinNum = attr.getInt("min", mMinNum);
    mMaxNum = attr.getInt("max", mMaxNum);

    mSelectVisibility = attr.getInt("selectVisibility", std::unordered_map<std::string, int>{
        { "gone", (int)View::GONE },
        { "invisible", (int)View::INVISIBLE },
        { "visible", (int)View::VISIBLE }
    }, mSelectVisibility);
    mGravity = attr.getGravity("gravity", mGravity);

    mTextTheme.size = attr.getDimensionPixelSize("textSize", mTextTheme.size);
    mTextTheme.color = attr.getColor("textColor", mTextTheme.color);
    mTextTheme.activeSize = attr.getDimensionPixelSize("activeTextSize", mTextTheme.activeSize);
    mTextTheme.activeColor = attr.getColor("activeTextColor", mTextTheme.activeColor);
    mTextTheme2.size = attr.getDimensionPixelSize("textSize2", mTextTheme.size);
    mTextTheme2.color = attr.getColor("textColor2", mTextTheme.color);
    mTextTheme2.activeSize = attr.getDimensionPixelSize("activeTextSize2", mTextTheme2.size);
    mTextTheme2.activeColor = attr.getColor("activeTextColor2", mTextTheme2.color);
    mCenterTextTheme.size = attr.getDimensionPixelSize("centerTextSize", mTextTheme.size);
    mCenterTextTheme.color = attr.getColor("centerTextColor", mTextTheme.color);
    mCenterTextTheme.activeSize = attr.getDimensionPixelSize("activeCenterTextSize", mCenterTextTheme.size);
    mCenterTextTheme.activeColor = attr.getColor("activeCenterTextColor", mCenterTextTheme.color);

    mItemBackground = attr.getString("itemBackground", mItemBackground);

    mSelectLayout = attr.getString("internalLayout", mSelectLayout);
    mOverlayLayout = attr.getString("overlayLayout", mOverlayLayout);
    mSelectOverlayLayout = attr.getString("selectOverlayLayout", mOverlayLayout);

    mTextStyle = attr.getInt("textStyle", std::unordered_map<std::string, int>{
        { "normal", (int)Typeface::NORMAL },
        { "bold"  , (int)Typeface::BOLD },
        { "italic", (int)Typeface::ITALIC }
    }, mTextStyle);
    mFontFamily = attr.getString("fontFamily", mFontFamily);
    mFontTypeface = Typeface::create(mFontFamily, mTextStyle);

    mXMLWidth = attr.getLayoutDimension("layout_width", LayoutParams::WRAP_CONTENT);
    mXMLHeight = attr.getLayoutDimension("layout_height", LayoutParams::WRAP_CONTENT);
    init();
}

RVNumberPicker::~RVNumberPicker() {
    if (mAdapter) {
        delete mAdapter;
        mAdapter = nullptr;
    }
    if (mSnapHelper) {
        delete mSnapHelper;
        mSnapHelper = nullptr;
    }
}

void RVNumberPicker::init() {
    mRealCount = mMaxNum - mMinNum + 1;

    mAdapter = new PickerAdapter(this);
    mLayoutManager = new PickerManager(mContext, this, mOrientation, mReverse);
    mSnapHelper = new PickerSnapHelper();

    setAdapter(mAdapter);
    setLayoutManager(mLayoutManager);
    mSnapHelper->attachToRecyclerView(this);
    getRecycledViewPool().setMaxRecycledViews(0, RECYCLED_VIEW_MULT * mDisplayCount);
}

int RVNumberPicker::getValue() {
    return mPosition + mMinNum;
}

int  RVNumberPicker::getMaxValue() {
    return mMaxNum;
}

int  RVNumberPicker::getMinValue() {
    return mMinNum;
}

/// @brief 设置当前值
/// @param value 设置值
/// @param smooth 是否平滑切换
/// @param callBack 是否触发回调（仅在非smooth模式下生效，smooth模式下自动会触发valuechange回调）
void RVNumberPicker::setValue(int value, bool smooth, bool callBack) {
    mPosition = std::max(mMinNum, std::min(value, mMaxNum)) - mMinNum;
    if (smooth) {
        smoothScrollToPosition(mPosition);
    } else {
        scrollToPosition(mPosition);
        if (callBack) onValueChanged(mPosition);
    }
}

void RVNumberPicker::nextValue(bool smooth, bool cycle) {
    offsetValue(1, smooth, cycle);
}

void RVNumberPicker::prevValue(bool smooth, bool cycle) {
    offsetValue(-1, smooth, cycle);
}

/// @brief 通用偏移跳转
/// @param delta 偏移量 (+1 下一项, -1 上一项)
/// @param smooth 是否平滑切换
/// @param cycle 是否循环
void RVNumberPicker::offsetValue(int delta, bool smooth, bool cycle) {
    int target = mPosition + delta;
    if (cycle) {
        target = (target % mRealCount + mRealCount) % mRealCount;
    } else if (target < 0 || target >= mRealCount) {
        return;
    }
    if (smooth) {
        smoothScrollToPosition(target);
    } else {
        scrollToPosition(target);
        onValueChanged(target);
    }
}

void RVNumberPicker::setMaxValue(int value) {
    mMaxNum = value;
    mRealCount = mMaxNum - mMinNum + 1;
}

void RVNumberPicker::setMinValue(int value) {
    mMinNum = value;
    mRealCount = mMaxNum - mMinNum + 1;
}

/// @brief 通知更新
/// @param isItemChange （ture），会优化notify时所需时间，但是否全部场景都适用，还待测试
void RVNumberPicker::notifyUpdate(bool isItemChange) {
    if (isItemChange) mAdapter->notifyItemRangeChanged(0, mRealCount);
    else              mAdapter->notifyDataSetChanged();
}

/// @brief 通知更新
void RVNumberPicker::notifyUpdatePosition(int position) {
    mAdapter->notifyItemChanged(position);
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
    notifyUpdate(false);
    setValue(value);
}

/// @brief 更新选中项
/// @param visibility 
void RVNumberPicker::setSelectVisibility(int visibility) {
    mSelectVisibility = visibility;
    notifyUpdate(true);
}

/// @brief 设置文本格式化
/// @param l 
void RVNumberPicker::setFormatter(TextFormatter l) {
    mNumberFormatter = l;
    notifyUpdate(false);
}

/// @brief 设置 选中项 文本格式化
/// @param l 
void RVNumberPicker::setSelectFormatter(TextFormatter l) {
    mSelectNumberFormatter = l;
    notifyUpdate(false);
}

void RVNumberPicker::setOverlayFormatter(TextFormatter l) {
    mOverlayFormatter = l;
    notifyUpdate(false);
}

/// @brief 设置smooth时 duration
/// @param duration
void RVNumberPicker::setSmoothScrollerDuration(int duration) {
    mSmoothDuration = duration;
}

/// @brief 设置图片资源列表，当数量为非0时，启用图像模式
/// @param list 图片资源
/// @param update 是否更新最小值/最大值
/// @param newValue update更新后的值，-1为当前值
void RVNumberPicker::setImageList(const std::vector<std::string>& list, bool update, int newValue) {
    mImageList = list;
    if (update)
        updateStruct(0, (int)list.size() - 1, newValue >= 0 ? newValue : getValue());
    notifyUpdate(false);
}

/// @brief 设置位置偏移列表
/// @param list 
void RVNumberPicker::setConvertList(std::vector<ConvertStruct> list) {
    std::sort(list.begin(), list.end(),
        [](const ConvertStruct& a, const ConvertStruct& b) { return a.position < b.position; });
    mConvertList = std::move(list);
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
