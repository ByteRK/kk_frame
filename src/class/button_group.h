/*
 * @Author: Ricken
 * @Email:
 * @Date: 2026-01-14 16:09:30
 * @LastEditTime: 2026-01-23 00:08:36
 * @FilePath: /kk_frame/src/class/button_group.h
 * @Description: 按键组(灵感来源于xialc)
 *               SingleChoice:单选  MultiChoice:多选
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BUTTON_GROUP_H__
#define __BUTTON_GROUP_H__

#include <map>
#include <set>
#include <view/view.h>

/// @brief 单选按键组
class SingleChoiceG {
public:
    typedef std::map<int, View*>             ButtonViews;
    typedef std::map<int, const std::string> ButtonSTRValues;
    typedef std::map<int, int>               ButtonINTValues;

    DECLARE_UIEVENT(bool, OnItemCLickListener, int oldID, int newID);

public:
    SingleChoiceG();
    ~SingleChoiceG();
    bool addView(View* view);
    void setStrMap(const ButtonSTRValues& values);
    void setIntMap(const ButtonINTValues& values);
    void setOnClickListener(OnItemCLickListener l);
    bool clickView(int id);

    int          getID();
    std::string  getString();
    int          getInt();

    View*        getView(int id);
    ButtonViews* getButtons();

protected:
    virtual void onButtonClick(View& v);

protected:
    int                     mLastID;
    std::map<int, View*>    mButtons;
    ButtonSTRValues         mSTRValues;
    ButtonINTValues         mINTValues;
    OnItemCLickListener     mOnClickListener;
};

/// @brief 多选按键组 TODO
class MultiChoiceG {
public:
    typedef std::map<int, View*>             ButtonViews;
    typedef std::map<int, const std::string> ButtonSTRValues;
    typedef std::map<int, int>               ButtonINTValues;
    typedef std::set<int>                    SelectedSet;

    DECLARE_UIEVENT(bool, OnItemCLickListener, int id, bool selected);

public:
    MultiChoiceG();
    ~MultiChoiceG();

    bool addView(View* view);
    void setStrMap(const ButtonSTRValues& values);
    void setIntMap(const ButtonINTValues& values);
    void setOnClickListener(OnItemCLickListener l);

    bool selectView(int id, bool selected = true);
    bool toggleView(int id);
    void clearSelection();

    bool                     isSelected(int id) const;
    SelectedSet              getSelectedIDs() const;
    std::vector<std::string> getSelectedStrings() const;
    std::vector<int>         getSelectedInts() const;

    View*        getView(int id);
    ButtonViews* getButtons();
    size_t       getSelectedCount() const;

protected:
    virtual void onButtonClick(View& v);

protected:
    SelectedSet             mSelectedIDs;
    std::map<int, View*>    mButtons;
    ButtonSTRValues         mSTRValues;
    ButtonINTValues         mINTValues;
    OnItemCLickListener     mOnClickListener;
};

#endif // !__BUTTON_GROUP_H__
