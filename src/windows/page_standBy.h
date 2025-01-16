/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2024-05-23 00:04:23
 * @LastEditTime: 2025-01-17 01:16:13
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
    uint8_t getType() const override;
protected:
    void getView() override;
    void setAnim() override;
    void setView() override;
    void loadData() override;

    void onTick() override;
    void onReload() override;
    bool onKey(uint16_t keyCode, uint8_t status) override;
};

#endif // !_PAGE_STANDBY_H_