/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2026-08-07 09:59:04
 * @FilePath: /kk_frame/src/widgets/rvNumberPicker.h
 * @Description: 使用RecycleView实现数字选择器
 *
 * @BugList: 1、暂时不要使用SmoothscrolltoPosition
 *           2、textColor全透颜色请使用#01000000,暂不支持全0透明度
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __RV_NUMBERPICKER_H__
#define __RV_NUMBERPICKER_H__

#include <view/view.h>
#include <core/context.h>
#include <core/typeface.h>
#include <widget/textview.h>
#include <widget/imageview.h>
#include <widgetEx/recyclerview/recyclerview.h>
#include <widgetEx/recyclerview/snaphelper.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>
#include <widgetEx/recyclerview/linearsmoothscroller.h>

class RVNumberPicker :public cdroid::RecyclerView {
public:
    DECLARE_UIEVENT(std::string, TextFormatter, int);
    DECLARE_UIEVENT(void, OverlayFormatter, int, View&);
    DECLARE_UIEVENT(void, OnItemClickListener, RVNumberPicker&, View&, int);
    DECLARE_UIEVENT(bool, OnItemLongClickListener, RVNumberPicker&, View&, int);
    DECLARE_UIEVENT(void, OnValueChangeListener, RVNumberPicker&, int, int);
    DECLARE_UIEVENT(void, OnCenterViewChangeListener, RVNumberPicker&, int, int);

public:
    /// @brief Picker显示类型
    typedef enum : int8_t {
        PICKER_TYPE_TEXT,        // 文字模式
        PICKER_TYPE_IMAGE,       // 图片模式
    } PICKER_TYPE;

    /// @brief XY轴偏移方式（相对于Position）
    typedef enum : int8_t {
        TRANSFER_NEGATE = -1,    // 负值
        TRANSFER_ABS = 0,        // 绝对值
        TRANSFER_RELATIVE = 1,   // 相对值
    } TRANSFER_TYPE;

    /// @brief 位置变换结构体
    struct ConvertStruct {
        float         position;                       // 位置
        int           transX;                         // X轴偏移量
        TRANSFER_TYPE transXType{ TRANSFER_ABS };     // X轴偏移方式
        int           transY;                         // Y轴偏移量
        TRANSFER_TYPE transYType{ TRANSFER_ABS };     // Y轴偏移方式
        ConvertStruct(float p, int tx, int ty) :
            position(p), transX(tx), transY(ty) { };
        ConvertStruct(float p, int tx, TRANSFER_TYPE txt, int ty, TRANSFER_TYPE tyt) :
            position(p), transX(tx), transXType(txt), transY(ty), transYType(tyt) { }
    };

public:
    /// @brief RVNumberPicker适配器
    class PickerAdapter :public cdroid::RecyclerView::Adapter {
    private:
        RVNumberPicker* mFriend{ nullptr };               // Picker指针
        LinearLayout*   mOverlayInflateParent{ nullptr }; // overlay inflate 复用容器
    public:
        PickerAdapter(RVNumberPicker* wheelView);
        ~PickerAdapter();
        ViewHolder* onCreateViewHolder(ViewGroup* parent, int viewType) override;
        void        onBindViewHolder(RecyclerView::ViewHolder& holder, int position) override;
        int         getItemCount() override;
        int         getItemViewType(int position) override;
    private:
        View* createImageItem(ViewGroup* parent);
        View* createSimpleTextItem(ViewGroup* parent);
        View* createSelectTextItem(ViewGroup* parent);
        void  bindImageItem(ImageView* imageView, int position, int realPosition);
        void  bindSimpleTextItem(TextView* textView, int realPosition);
        void  bindSelectTextItem(ViewGroup* layout, int position, int realPosition);
    };

    class PickerScroller :public LinearSmoothScroller {
    private:
        int                           mSmoothDuration{ 300 }; // 滑动时间
        const cdroid::DisplayMetrics& mDisplayMetrics;        // 显示参数
    public:
        PickerScroller(Context* context);
        void setDuration(int duration);
        void onTargetFound(View* targetView, RecyclerView::State& state, Action& action) override;
    private:
        int  calculateTimeForDeceleration(int dx);
    };

    /// @brief RVNumberPicker布局管理器
    class PickerManager :public LinearLayoutManager {
    private:
        RVNumberPicker* mFriend{ nullptr };        // Picker指针
        int             mCenterPositionCache{ 0 }; // 中间项Position缓存
    public:
        PickerManager(Context* context, RVNumberPicker* pickerView, int orientation, bool reverseLayout);
        void onAttachedToWindow(RecyclerView& view) override;
        void onLayoutCompleted(State& state) override;
        void onScrollStateChanged(int state) override;
        void smoothScrollToPosition(RecyclerView& recyclerView, RecyclerView::State& state, int position) override;
    private:
        void onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec) override;
        int  scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state) override;
        int  scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state) override;
        template<bool IsHorizontal> void adjustChildViewImpl();

        int  calculateColorValue(float abs, bool activated);
        int  calculateTextSize(float abs, bool activated);
        void calculateConvertValue(View* child, const float& position);
    };

    /// @brief RVNumberPicker滑动辅助
    class PickerSnapHelper :public SnapHelper {
    private:
        static constexpr float INVALID_DISTANCE = 1.f;
        OrientationHelper* mVerticalHelper{ nullptr };     // 垂直方向
        OrientationHelper* mHorizontalHelper{ nullptr };   // 水平方向
    public:
        PickerSnapHelper() = default;
        ~PickerSnapHelper() override;
        void  calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int distance[2]) override;
        int   findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY) override;
        View* findSnapView(RecyclerView::LayoutManager& layoutManager) override;
    private:
        int   distanceToCenter(RecyclerView::LayoutManager& layoutManager, View& targetView, OrientationHelper& helper);
        int   estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper, int velocityX, int velocityY);
        View* findCenterView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
        float computeDistancePerChild(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
        OrientationHelper& getVerticalHelper(RecyclerView::LayoutManager& layoutManager);
        OrientationHelper& getHorizontalHelper(RecyclerView::LayoutManager& layoutManager);
    };

private:
    friend PickerAdapter;
    friend PickerManager;

private:
    /// @brief 文字主题结构体
    struct TextTheme {
        int size{ 20 };              // 文字大小
        int color{ 0xFFFFFF };       // 文字颜色
        int activeSize{ 20 };        // 活跃时文字大小
        int activeColor{ 0xFFFFFF }; // 活跃时文字颜色
    };

private:
    int         mPickerWidth{ 0 };                           // 宽
    int         mPickerHeight{ 0 };                          // 高

    int         mMinNum{ 0 };                                // 最小值
    int         mMaxNum{ 5 };                                // 最大值
    int         mRealCount{ 6 };                             // 实际数量
    int         mDisplayCount{ 3 };                          // 显示数量
    int         mPosition{ 0 };                              // 当前项位置
    std::string mItemBackground{ "@null" };                  // 子项背景
    int         mTextStyle{ Typeface::NORMAL };              // 字体
    std::string mFontFamily{ "" };                           // 字体
    Typeface*   mFontTypeface{ nullptr };                    // 字体
    int         mGravity{ Gravity::CENTER };                 // 文字对齐方式
    std::string mSelectLayout{ "" };                         // 选中项布局（与picker的editeText类似）
    int         mSelectVisibility{ View::VISIBLE };          // 选中项的可见性
    std::string mOverlayLayout{ "" };                        // 叠层绘制
    std::string mSelectOverlayLayout{ "" };                  // 选中项叠层绘制
    int         mOrientation{ LinearLayout::VERTICAL };      // 布局方向
    bool        mReverse{ false };                           // 是否反向布局
    int         mSmoothDuration{ 300 };                      // 滚动时间

    TextTheme   mTextTheme;                                  // 默认文字主题
    TextTheme   mTextTheme2;                                 // 默认文字主题2(两边使用)
    TextTheme   mCenterTextTheme;                            // 中间项文字主题

    std::vector<std::string>   mImageList;                   // 图片列表
    std::vector<ConvertStruct> mConvertList;                 // 转换信息列表

private:
    PickerAdapter*             mAdapter{ nullptr };                     // 适配器
    PickerManager*             mLayoutManager{ nullptr };               // 布局管理器
    SnapHelper*                mSnapHelper{ nullptr };                  // 滑动辅助类
    TextFormatter              mNumberFormatter{ nullptr };             // 数字格式化
    TextFormatter              mSelectNumberFormatter{ nullptr };       // 选中项数字格式化
    OverlayFormatter           mOverlayFormatter{ nullptr };            // 叠层回调（外抛Overlay View）


    OnItemClickListener        mOnItemClickListener{ nullptr };         // 点击事件
    OnItemLongClickListener    mOnItemLongClickListener{ nullptr };     // 长按事件
    OnValueChangeListener      mOnValueChangeListener{ nullptr };       // 值改变事件,滑动结束触发
    OnCenterViewChangeListener mOnCenterViewChangeListener{ nullptr };  // 中间项改变事件,滑动高频触发

public:
    RVNumberPicker(int w, int h);
    RVNumberPicker(Context* context, const AttributeSet& attr);
    ~RVNumberPicker();

private:
    void init();

public:
    int  getValue();
    int  getMaxValue();
    int  getMinValue();
    void setValue(int value, bool smooth = false, bool callBack = false);
    void nextValue(bool smooth = false, bool cycle = false);
    void prevValue(bool smooth = false, bool cycle = false);
    void offsetValue(int delta, bool smooth, bool cycle);
    void setMaxValue(int value);
    void setMinValue(int value);
    void notifyUpdate(bool isItemChange = true);
    void notifyUpdatePosition(int position);
    void updateStruct(int min, int max);
    void updateStruct(int min, int max, int value);
    void setSelectVisibility(int visibility);
    void setFormatter(TextFormatter l);
    void setSelectFormatter(TextFormatter l);
    void setOverlayFormatter(OverlayFormatter l);
    void setSmoothScrollerDuration(int duration);
    void setImageList(const std::vector<std::string>& list, bool update = false, int newValue = -1);
    void setConvertList(std::vector<ConvertStruct> list);
    void setOnItemClickListener(OnItemClickListener l);
    void setOnItemLongClickListener(OnItemLongClickListener l);
    void setOnValueChangedListener(OnValueChangeListener l);
    void setOnCenterViewChangeListener(OnCenterViewChangeListener l);

protected:
    void onItemClick(View& v, int position);
    void onItemLongClick(View& v, int position);
    void onSizeChanged(int w, int h, int oldw, int oldh) override;
    void onValueChanged(int n);
    void onCenterViewChanged(int o, int n);
};

#endif // __RV_NUMBERPICKER_H__
