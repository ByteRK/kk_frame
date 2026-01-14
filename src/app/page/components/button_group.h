/*
 * @Author: xialc
 * @Email:
 * @Date: 2026-01-14 16:09:30
 * @LastEditTime: 2026-01-14 16:56:40
 * @FilePath: /kk_frame/src/app/page/components/button_group.h
 * @Description: 按键组
 *               SingleChoice:单选  MultiChoice:多选
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __BUTTON_GROUP_H__
#define __BUTTON_GROUP_H__

#include <map>
#include <view/view.h>

/// @brief 单选按键组
class SingleChoiceG {
public:
    typedef std::map<int, View*>             ButtonViews;
    typedef std::map<int, const std::string> ButtonSTRValues;
    typedef std::map<int, int>               ButtonINTValues;

public:
    SingleChoiceG();
    ~SingleChoiceG();
    bool addView(View* view);
    void setStrMap(const ButtonSTRValues& values);
    void setIntMap(const ButtonINTValues& values);
    void setOnClickListener(View::OnClickListener l);
    bool clickView(int id);
    int          getID();
    std::string  getString();
    int          getInt();
    View*        get(int id);
    ButtonViews* getButtons();

protected:
    virtual void onButtonClick(View& v);

protected:
    int                     mLastID;
    std::map<int, View*>    mButtons;
    ButtonSTRValues         mSTRValues;
    ButtonINTValues         mINTValues;
    View::OnClickListener   mOnClickListener;
};

/// @brief 多选按键组 TODO
class MultiChoiceG {
public:
    MultiChoiceG() = default;
    ~MultiChoiceG() = default;
};

#endif // !__BUTTON_GROUP_H__
