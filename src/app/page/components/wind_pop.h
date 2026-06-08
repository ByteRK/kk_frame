/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-02-08 02:48:19
 * @LastEditTime: 2026-06-09 01:26:07
 * @FilePath: /kk_frame/src/app/page/components/wind_pop.h
 * @Description: 弹窗组件
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __WIND_POP_H__
#define __WIND_POP_H__

#include "pop.h"

class WindPop {
private:
    PopBase*   mPop{ nullptr };       // 弹窗指针
    ViewGroup* mPopBox{ nullptr };    // 弹窗容器

public:
    WindPop();
    virtual ~WindPop();

    void       init(ViewGroup* parent);

    PopBase*   getPop();
    int8_t     getPopType();
    int8_t     showPop(PopBase* pop, LoadMsgBase* initData = nullptr);
    void       removePop();
    void       hidePopBox();
    void       showPopBox();
    bool       onKey(KeyEvent& evt);
    
};

#endif // !__WIND_POP_H__