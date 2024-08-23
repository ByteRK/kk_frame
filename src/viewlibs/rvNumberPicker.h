/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-22 15:55:07
 * @LastEditTime: 2024-07-12 01:14:19
 * @FilePath: /kk_frame/src/viewlibs/rvNumberPicker.h
 * @Description: 使用RecycleView实现数字选择器
 * @BugList: 1、暂时不要使用SmoothscrolltoPosition
 *           2、layout_width以及layout_height必须指定数值
 *           3、textColor全透颜色请使用#01000000,暂不支持全0透明度
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */


#ifndef _RV_NUMBERPICKER_H_
#define _RV_NUMBERPICKER_H_

 // #include "pickerLayoutManager.h"
#include <core/context.h>
#include <core/typeface.h>
#include <widgetEx/recyclerview/linearlayoutmanager.h>
#include <widgetEx/recyclerview/snaphelper.h>
#include <widgetEx/recyclerview/recyclerview.h>
#include <view/view.h>

class RVNumberPicker :public cdroid::RecyclerView {
public:
    DECLARE_UIEVENT(std::string, TextFormatter, int);
    DECLARE_UIEVENT(void, OnItemClickListener, RVNumberPicker&, View&, int);
    DECLARE_UIEVENT(bool, OnItemLongClickListener, RVNumberPicker&, View&, int);
    DECLARE_UIEVENT(void, OnValueChangeListener, RVNumberPicker&, int, int);
    DECLARE_UIEVENT(void, OnCenterViewChangeListener, RVNumberPicker&, int, int);

    // XY轴偏移类型（相对于Position）
    enum {
        TRANSFER_NEGATE = -1,       // 负值
        TRANSFER_ABS = 0,           // 绝对值
        TRANSFER_RELATIVE = 1,      // 相对值
    };
    // 位置变换结构体
    typedef struct Convert_Struct {
        float       position;       // 位置
        int         transX;         // X轴偏移量
        int         transXType;     // X轴偏移类型
        int         transY;         // Y轴偏移量
        int         transYType;     // Y轴偏移类型
        Convert_Struct(float p, int tx, int ty) : position(p),
            transX(tx), transXType(TRANSFER_ABS), transY(ty), transYType(TRANSFER_ABS) {
        };
        Convert_Struct(float p,
            int tx, int txt, int ty, int tyt) : position(p),
            transX(tx), transXType(txt), transY(ty), transYType(tyt) {
        }
    }ConvertStruct;

private:

    /// @brief RVNumberPicker适配器
    class RVNumberPickerAdapter :public cdroid::RecyclerView::Adapter {
    private:
        RVNumberPicker* mFriend;     // RVNumberPicker指针
        enum {
            PICKER_TYPE_TEXT,        // 文字模式
            PICKER_TYPE_IMAGE,       // 图片模式
        };
    public:
        RVNumberPickerAdapter(RVNumberPicker* wheelView);
        ~RVNumberPickerAdapter();
        ViewHolder* onCreateViewHolder(ViewGroup* parent, int viewType)override;
        void onBindViewHolder(RecyclerView::ViewHolder& holder, int position)override;
        int  getItemCount()override;
        int  getItemViewType(int position)override;
    };

    /// @brief RVNumberPicker布局管理器
    class RVNumberPickerManage :public LinearLayoutManager {
    private:
        RVNumberPicker* mFriend;     // RVNumberPicker指针
        View* mCenterViewCache;      // 中间项页面指针缓存
        int   mCenterPositionCache;  // 中间项Position缓存
    public:
        RVNumberPickerManage(Context* context, RVNumberPicker* pickerView, int orientation, bool reverseLayout);
        ~RVNumberPickerManage();
        void onScrollStateChanged(int state)override;
        void onAttachedToWindow(RecyclerView* view) override;
    private:
        void init();
        void onMeasure(RecyclerView::Recycler& recycler, RecyclerView::State& state, int widthSpec, int heightSpec)override;
        void onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
        int  scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
        int  scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
        void adjustHorizontalChildView();
        void adjustVerticalChildView();

        int  calculateColorValue(float abs);
        int  calculateTextSize(float abs);
        void calculateConvertValue(View* child, const float& position);
    };

    /// @brief RVNumberPicker滑动辅助
    class RVNumberPickerSnapHelper :public SnapHelper {
    private:
        static constexpr float INVALID_DISTANCE = 1.f;
        OrientationHelper* mVerticalHelper;               // 垂直方向
        OrientationHelper* mHorizontalHelper;             // 水平方向
    public:
        RVNumberPickerSnapHelper();
        ~RVNumberPickerSnapHelper()override;
        void calculateDistanceToFinalSnap(RecyclerView::LayoutManager& layoutManager, View& targetView, int distance[2])override;
        int findTargetSnapPosition(RecyclerView::LayoutManager& layoutManager, int velocityX, int velocityY)override;
        View* findSnapView(RecyclerView::LayoutManager& layoutManager)override;
    private:
        int distanceToCenter(RecyclerView::LayoutManager& layoutManager, View& targetView, OrientationHelper& helper);
        int estimateNextPositionDiffForFling(RecyclerView::LayoutManager& layoutManager,
            OrientationHelper& helper, int velocityX, int velocityY);
        View* findCenterView(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
        float computeDistancePerChild(RecyclerView::LayoutManager& layoutManager, OrientationHelper& helper);
        OrientationHelper& getVerticalHelper(RecyclerView::LayoutManager& layoutManager);
        OrientationHelper& getHorizontalHelper(RecyclerView::LayoutManager& layoutManager);
    };

public:
    friend RVNumberPickerAdapter;
    friend RVNumberPickerManage;
private:
    bool        mReverse;                                              // 是否反向布局
    int         mOrientation;                                          // 布局方向
    int         mMinNum, mMaxNum;                                      // 最小值和最大值
    int         mDisplayCount, mRealCount;                             // 显示数量和实际数量
    int         mCenterSize, mTextSize, mTextSize2, mActiveSize;       // 文字大小属性
    int         mCenterColor, mTextColor, mTextColor2, mActiveColor;   // 文字颜色属性
    int         mTextStyle;                                            // 文字Style
    int         mGravity;                                              // 文字对齐方式
    std::string mItemBackground;                                       // 背景
    int         mXMLWidth, mXMLHeight;                                 // xml宽高
    std::string mFontFamily;                                           // 字体
    Typeface* mFontTypeface;                                         // 字体面
    std::vector<std::string>   mImageList;                             // 图片列表
    std::vector<ConvertStruct> mConvertList;                           // 转换信息列表
private:
    Context* mContext;                                                 // 上下文
    RVNumberPickerAdapter* mAdapter;                                   // 适配器
    RVNumberPickerManage* mLayoutManage;                               // 布局管理器
    SnapHelper* mSnapHelper;                                           // 滑动辅助类
    TextFormatter              mNumberFormatter;                       // 数字格式化

    int                        mPosition = 0;                          // 当前项位置
    OnItemClickListener        mOnItemClickListener = nullptr;         // 点击事件
    OnItemLongClickListener    mOnItemLongClickListener = nullptr;     // 长按事件
    OnValueChangeListener      mOnValueChangeListener = nullptr;       // 值改变事件,滑动结束触发
    OnCenterViewChangeListener mOnCenterViewChangeListener = nullptr;  // 中间项改变事件,滑动高频触发
public:
    RVNumberPicker(int w, int h);
    RVNumberPicker(Context* context, const AttributeSet& attr);
    ~RVNumberPicker();
    void init();
    int  getValue();
    void setValue(int value, bool smooth = false, bool callBack = false);
    void nextValue(bool smooth = false, bool cycle = false);
    void prevValue(bool smooth = false, bool cycle = false);
    void setMaxValue(int maxvalue);
    void setMinValue(int minvalue);
    void notifyUpdate();
    void updateStruct(int min, int max);
    void updateStruct(int min, int max, int value);
    void setFormatter(TextFormatter l);
    void setImageList(std::vector<std::string> list, bool update = false, int newValue = -1);
    void setConvertList(std::vector<ConvertStruct> list);
    void setOnItemClickListener(OnItemClickListener l);
    void setOnItemLongClickListener(OnItemLongClickListener l);
    void setOnValueChangedListener(OnValueChangeListener l);
    void setOnCenterViewChangeListener(OnCenterViewChangeListener l);
protected:
    void onItemClick(View& v, int position);
    void onItemLongClick(View& v, int position);
    void onValueChanged(int n);
    void onCenterViewChanged(int o, int n);
};



#endif // _RV_NUMBERPICKER_H_
