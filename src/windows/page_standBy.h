/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2024-08-23 10:50:57
 * @FilePath: /kk_frame/src/windows/page_standBy.h
 * @Description: 待机页面
 * @BugList:
 *
 * Copyright (c) 2024 by Ricken, All Rights Reserved.
 *
 */

#ifndef _PAGE_STANDBY_H_
#define _PAGE_STANDBY_H_

#include "base.h"
#include <widget/textview.h>
#include <widget/imageview.h>

class StandByPage :public PageBase {
private:
public:
    StandByPage();
    ~StandByPage();

    void reload() override;
    void onTick() override;

    int8_t getPageType()override;
    void   checkLight(uint8_t* left, uint8_t* right) override;
protected:
    void getView() override;
    void setAnim() override;
    void setView() override;
    void loadData() override;

    bool onKey(uint16_t keyCode, uint8_t status) override;
};

#endif // !_PAGE_STANDBY_H_