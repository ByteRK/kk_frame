/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-08 17:11:53
 * @FilePath: /hana_frame/src/windows/page_standBy.h
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#ifndef _PAGE_STANDBY_H_
#define _PAGE_STANDBY_H_

#include "base.h"

class StandByPage :public PageBase {
private:
public:
    StandByPage();
    ~StandByPage();
    uint8_t getType() const override;
protected:
    void setView() override;

    void onTick() override;
};

#endif // !_PAGE_STANDBY_H_